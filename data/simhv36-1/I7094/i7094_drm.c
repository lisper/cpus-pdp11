/* i7094_drm.c: 7289/7320A drum simulator

   Copyright (c) 2005-2006, Robert M Supnik

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   ROBERT M SUPNIK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of Robert M Supnik shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from Robert M Supnik.

   drm          7289/7320A "fast" drum

   Very little is known about this device; the behavior simulated here is
   what is used by CTSS.

   - The drum channel/controller behaves like a hybrid of the 7607 and the 7909.
     It responds to SCD (like the 7909), gets its address from the channel
     program (like the 7909), but responds to IOCD/IOCP (like the 7607) and
     sets channel flags (like the 7607).
   - The drum channel supports at least 2 drums.  The maximum is 8 or less.
     Physical drums are numbered from 0.
   - Each drum has a capacity of 192K 36b words.  This is divided into 6
     "logical" drum of 32KW each.  Each "logical" drum has 16 2048W "sectors".
     Logical drums are numbered from 1.
   - The drum's behavior if a sector boundary is crossed in mid-transfer is
     unknown.  CTSS never does this.
   - The drum's behavior with record operations is unknown.  CTSS only uses
     IOCD and IOCP.
   - The drum's error indicators are unknown.  CTSS regards bits <0:2,13> of
     the returned SCD data as errors, as well as the normal 7607 trap flags.
   - The drum's rotational speed is unknown.

   Assumptions in this simulator:

   - Transfers may not cross a sector boundary.  An attempt to do so sets
     the EOF flag and causes an immediate disconnect.
   - The hardware never sets end of record.

   For speed, the entire drum is buffered in memory.
*/

#include "i7094_defs.h"
#include <math.h>

#define DRM_NUMDR       8                               /* drums/controller */

/* Drum geometry */

#define DRM_NUMWDS      2048                            /* words/sector */
#define DRM_SCMASK      (DRM_NUMWDS - 1)                /* sector mask */
#define DRM_NUMSC       16                              /* sectors/log drum */
#define DRM_NUMWDL      (DRM_NUMWDS * DRM_NUMSC)        /* words/log drum */
#define DRM_NUMLD       6                               /* log drums/phys drum */
#define DRM_SIZE        (DRM_NUMLD * DRM_NUMWDL)        /* words/phys drum */
#define GET_POS(x)      ((int) fmod (sim_gtime() / ((double) (x)), \
                        ((double) DRM_NUMWDS)))

/* Drum address from channel */

#define DRM_V_PHY       30                              /* physical drum sel */
#define DRM_M_PHY       07
#define DRM_V_LOG       18                              /* logical drum sel */
#define DRM_M_LOG       07
#define DRM_V_WDA       0                               /* word address */
#define DRM_M_WDA       (DRM_NUMWDL - 1)
#define DRM_GETPHY(x)   (((uint32) ((x) >> DRM_V_PHY)) & DRM_M_PHY)
#define DRM_GETLOG(x)   ((((uint32) (x)) >> DRM_V_LOG) & DRM_M_LOG)
#define DRM_GETWDA(x)   ((((uint32) (x)) >> DRM_V_WDA) & DRM_M_WDA)
#define DRM_GETDA(x)    (((DRM_GETLOG(x) - 1) * DRM_NUMWDL) + DRM_GETWDA(x))

/* Drum controller states */

#define DRM_IDLE        0
#define DRM_1ST         1
#define DRM_DATA        2
#define DRM_EOS         3

uint32 drm_ch = CH_G;                                   /* drum channel */
uint32 drm_da = 0;                                      /* drum address */
uint32 drm_sta = 0;                                     /* state */
uint32 drm_op = 0;                                      /* operation */
t_uint64 drm_chob = 0;                                  /* output buf */
uint32 drm_chob_v = 0;                                  /* valid */
int32 drm_time = 10;                                    /* inter-word time */

extern uint32 ind_ioc;

t_stat drm_svc (UNIT *uptr);
t_stat drm_reset (DEVICE *dptr);
t_stat drm_chsel (uint32 ch, uint32 sel, uint32 unit);
t_stat drm_chwr (uint32 ch, t_uint64 val, uint32 flags);
t_bool drm_da_incr (void);

/* DRM data structures

   drm_dev      DRM device descriptor
   drm_unit     DRM unit descriptor
   drm_reg      DRM register list
*/

DIB drm_dib = { &drm_chsel, &drm_chwr };

UNIT drm_unit[] = {
    { UDATA (&drm_svc, UNIT_FIX+UNIT_ATTABLE+UNIT_BUFABLE+
             UNIT_MUSTBUF+UNIT_DISABLE, DRM_SIZE) },
    { UDATA (&drm_svc, UNIT_FIX+UNIT_ATTABLE+UNIT_BUFABLE+
             UNIT_MUSTBUF+UNIT_DISABLE, DRM_SIZE) },
    { UDATA (&drm_svc, UNIT_FIX+UNIT_ATTABLE+UNIT_BUFABLE+
             UNIT_MUSTBUF+UNIT_DISABLE+UNIT_DIS, DRM_SIZE) },
    { UDATA (&drm_svc, UNIT_FIX+UNIT_ATTABLE+UNIT_BUFABLE+
             UNIT_MUSTBUF+UNIT_DISABLE+UNIT_DIS, DRM_SIZE) },
    { UDATA (&drm_svc, UNIT_FIX+UNIT_ATTABLE+UNIT_BUFABLE+
             UNIT_MUSTBUF+UNIT_DISABLE+UNIT_DIS, DRM_SIZE) },
    { UDATA (&drm_svc, UNIT_FIX+UNIT_ATTABLE+UNIT_BUFABLE+
             UNIT_MUSTBUF+UNIT_DISABLE+UNIT_DIS, DRM_SIZE) },
    { UDATA (&drm_svc, UNIT_FIX+UNIT_ATTABLE+UNIT_BUFABLE+
             UNIT_MUSTBUF+UNIT_DISABLE+UNIT_DIS, DRM_SIZE) },
    { UDATA (&drm_svc, UNIT_FIX+UNIT_ATTABLE+UNIT_BUFABLE+
             UNIT_MUSTBUF+UNIT_DISABLE+UNIT_DIS, DRM_SIZE) }
    };

REG drm_reg[] = {
    { ORDATA (STATE, drm_sta, 2) },
    { ORDATA (DA, drm_da, 18) },
    { FLDATA (OP, drm_op, 0) },
    { ORDATA (CHOB, drm_chob, 36) },
    { FLDATA (CHOBV, drm_chob_v, 0) },
    { DRDATA (TIME, drm_time, 24), REG_NZ + PV_LEFT },
    { DRDATA (CHAN, drm_ch, 3), REG_HRO },
    { NULL }
    };

MTAB drm_mtab[] = {
    { MTAB_XTD|MTAB_VDV, 0, "CHANNEL", NULL, NULL, &ch_show_chan },
    { 0 }
    };

DEVICE drm_dev = {
    "DRM", drm_unit, drm_reg, drm_mtab,
    DRM_NUMDR, 8, 18, 1, 8, 36,
    NULL, NULL, &drm_reset,
    NULL, NULL, NULL,
    &drm_dib, DEV_DIS
    };

/* Channel select routine */

t_stat drm_chsel (uint32 ch, uint32 sel, uint32 unit)
{
drm_ch = ch;                                            /* save channel */
if (sel & CHSL_NDS) return ch6_end_nds (ch);            /* nds? nop */

switch (sel) {                                          /* case on cmd */

    case CHSL_RDS:                                      /* read */
    case CHSL_WRS:                                      /* write */
        if (drm_sta != DRM_IDLE) return ERR_STALL;      /* busy? */
        drm_sta = DRM_1ST;                              /* initial state */
        if (sel == CHSL_WRS) drm_op = 1;                /* set read/write */
        else drm_op = 0;                                /* LCHx sends addr */
        break;                                          /* wait for addr */

    default:                                            /* other */
        return STOP_ILLIOP;
        }
return SCPE_OK;
}

/* Channel write routine */

t_stat drm_chwr (uint32 ch, t_uint64 val, uint32 flags)
{
uint32 u, l;
int32 cp, dp;

if (drm_sta == DRM_1ST) {
    u = DRM_GETPHY (val);                               /* get unit */
    l = DRM_GETLOG (val);                               /* get logical address */
    if ((u >= DRM_NUMDR) ||                             /* invalid unit? */
        (drm_unit[u].flags & UNIT_DIS) ||               /* disabled unit? */
        (l == 0) || (l > DRM_NUMLD)) {                  /* invalid log drum? */
        ch6_err_disc (ch, U_DRM, CHF_TRC);              /* disconnect */
        drm_sta = DRM_IDLE;
        return SCPE_OK;
        }
    drm_da = DRM_GETDA (val);                           /* get drum addr */
    cp = GET_POS (drm_time);                            /* current pos in sec */
    dp = (drm_da & DRM_SCMASK) - cp;                    /* delta to desired pos */
    if (dp <= 0) dp = dp + DRM_NUMWDS;                  /* if neg, add rev */
    sim_activate (&drm_unit[u], dp * drm_time);         /* schedule */
    if (drm_op) ch6_req_wr (ch, U_DRM);                 /* if write, get word */
    drm_sta = DRM_DATA;
    drm_chob = 0;                                       /* clr, inval buffer */
    drm_chob_v = 0;
    }
else {
    drm_chob = val & DMASK;
    drm_chob_v = 1;
    }
return SCPE_OK;
}

/* Unit service - this code assumes the entire drum is buffered */

t_stat drm_svc (UNIT *uptr)
{
t_uint64 *fbuf = (t_uint64 *) uptr->filebuf;

if ((uptr->flags & UNIT_BUF) == 0) {                    /* not buf? */
    ch6_err_disc (drm_ch, U_DRM, CHF_TRC);              /* set TRC, disc */
    drm_sta = DRM_IDLE;                                 /* drum is idle */
    return SCPE_UNATT;
    }
if (drm_da >= DRM_SIZE) {                               /* nx logical drum? */
    ch6_err_disc (drm_ch, U_DRM, CHF_EOF);              /* set EOF, disc */
    drm_sta = DRM_IDLE;                                 /* drum is idle */
    return SCPE_OK;
    }

switch (drm_sta) {                                      /* case on state */

    case DRM_DATA:                                      /* data */
        if (drm_op) {                                   /* write? */
            if (drm_chob_v) drm_chob_v = 0;             /* valid? clear */
            else if (ch6_qconn (drm_ch, U_DRM))         /* no, chan conn? */
                ind_ioc = 1;                            /* io check */
            fbuf[drm_da] = drm_chob;                    /* get data */
            if (drm_da >= uptr->hwmark) uptr->hwmark = drm_da + 1;
            if (!drm_da_incr ()) ch6_req_wr (drm_ch, U_DRM);
            }
        else{                                           /* read */
            ch6_req_rd (drm_ch, U_DRM, fbuf[drm_da], 0); /* send word to channel */
            drm_da_incr ();
            }
        sim_activate (uptr, drm_time);                  /* next word */
        break;

    case DRM_EOS:                                       /* end sector */
        if (ch6_qconn (drm_ch, U_DRM))                  /* drum still conn? */
            ch6_err_disc (drm_ch, U_DRM, CHF_EOF);      /* set EOF, disc */
        drm_sta = DRM_IDLE;                             /* drum is idle */
        break;
        }                                               /* end case */

return SCPE_OK;
}

/* Increment drum address - return true, set new state if end of sector */

t_bool drm_da_incr (void)
{
drm_da = drm_da + 1;
if (drm_da & DRM_SCMASK) return FALSE;
drm_sta = DRM_EOS;
return TRUE;
}

/* Reset routine */

t_stat drm_reset (DEVICE *dptr)
{
uint32 i;

drm_da = 0;
drm_op = 0;
drm_sta = DRM_IDLE;
drm_chob = 0;
drm_chob_v = 0;
for (i = 0; i < dptr->numunits; i++) sim_cancel (dptr->units + i);
return SCPE_OK;
}
