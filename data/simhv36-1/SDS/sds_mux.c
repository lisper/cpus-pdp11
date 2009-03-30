/* sds_mux.c: SDS 940 terminal multiplexor simulator

   Copyright (c) 2001-2005, Robert M Supnik

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

   mux          terminal multiplexor

   29-Jun-05    RMS     Added SET MUXLn DISCONNECT
   21-Jun-05    RMS     Fixed bug in SHOW CONN/STATS
   05-Jan-04    RMS     Revised for tmxr library changes
   09-May-03    RMS     Added network device flag

   This module implements up to 32 individual serial interfaces, representing
   either the project Genie terminal multiplexor or the SDS 940 CTE option.
*/

#include "sds_defs.h"
#include "sim_sock.h"
#include "sim_tmxr.h"
#include <ctype.h>

#define PROJ_GENIE      (cpu_unit.flags & UNIT_GENIE)
#define MUX_NUMLIN      mux_desc.lines

#define MUX_LINES       32                              /* lines */
#define MUX_FLAGS       4                               /* intr per line */
#define MUX_FLAGMASK    (MUX_FLAGS - 1)
#define MUX_SCANMAX     (MUX_LINES * MUX_FLAGS)         /* flags to scan */
#define MUX_SCANMASK    (MUX_SCANMAX - 1)
#define UNIT_V_UC       (UNIT_V_UF + 0)                 /* UC only */
#define UNIT_UC         (1 << UNIT_V_UC)
#define MUX_INIT_POLL   8000
#define MUXL_WAIT       500
#define MUX_SETFLG(l,x) mux_flags[((l) * MUX_FLAGS) + (x)] = 1
#define MUX_SETINT(x)   int_req = int_req | (INT_MUXR >> (x))
#define MUX_CLRINT(x)   int_req = int_req & ~(INT_MUXR >> (x))

/* PIN/POT */

#define P_V_CHAR        16                              /* char */
#define P_M_CHAR        0377
#define P_CHAR(x)       (((x) >> P_V_CHAR) & P_M_CHAR)
#define PIN_OVR         000100000                       /* overrun */
#define POT_NOX         000100000                       /* no xmit */
#define POT_XMI         000040000                       /* xmit int */
#define POT_GLNE        000020000                       /* Genie: enable */
#define POT_SCDT        000020000                       /* 940: clr DTR */
#define P_V_CHAN        0                               /* channel */
#define P_M_CHAN        (MUX_LINES - 1)
#define P_CHAN(x)       (((x) >> P_V_CHAN) & P_M_CHAN)

/* SKS 940 */

#define SKS_XBE         000001000                       /* xmt buf empty */
#define SKS_CRO         000000400                       /* carrier on */
#define SKS_DSR         000000200                       /* data set ready */
#define SKS_CHAN(x)     P_CHAN(x)

/* SKS Genie */

#define SKG_V_CHAN      7
#define SKG_M_CHAN      (MUX_LINES - 1)
#define SKG_CHAN(x)     (((x) >> SKG_V_CHAN) & SKG_M_CHAN)

/* Flags */

#define MUX_FRCV        0                               /* receive */
#define MUX_FXMT        1                               /* transmit */
#define MUX_FCRN        2                               /* carrier on */
#define MUX_FCRF        3                               /* carrier off */

/* Line status */

#define MUX_SCHP        001                             /* char pending */
#define MUX_SOVR        002                             /* overrun */
#define MUX_SLNE        004                             /* line enabled */
#define MUX_SXIE        010                             /* xmt int enab */
#define MUX_SCRO        020                             /* carrier on */
#define MUX_SDSR        040                             /* data set ready */

/* Data */

extern uint32 alert, int_req;
extern int32 stop_invins, stop_invdev, stop_inviop;
extern UNIT cpu_unit;

uint8 mux_rbuf[MUX_LINES];                              /* rcv buf */
uint8 mux_xbuf[MUX_LINES];                              /* xmt buf */
uint8 mux_sta[MUX_LINES];                               /* status */
uint8 mux_flags[MUX_SCANMAX];                           /* flags */
uint32 mux_tps = 100;                                   /* polls/second */
uint32 mux_scan = 0;                                    /* scanner */
uint32 mux_slck = 0;                                    /* scanner locked */

TMLN mux_ldsc[MUX_LINES] = { 0 };                       /* line descriptors */
TMXR mux_desc = { MUX_LINES, 0, 0, mux_ldsc };          /* mux descriptor */

t_stat mux (uint32 fnc, uint32 inst, uint32 *dat);
t_stat muxi_svc (UNIT *uptr);
t_stat muxo_svc (UNIT *uptr);
t_stat mux_reset (DEVICE *dptr);
t_stat mux_attach (UNIT *uptr, char *cptr);
t_stat mux_detach (UNIT *uptr);
t_stat mux_summ (FILE *st, UNIT *uptr, int32 val, void *desc);
t_stat mux_show (FILE *st, UNIT *uptr, int32 val, void *desc);
t_stat mux_vlines (UNIT *uptr, int32 val, char *cptr, void *desc);
void mux_reset_ln (int32 ln);
void mux_scan_next (void);

/* MUX data structures

   mux_dev      MUX device descriptor
   mux_unit     MUX unit descriptor
   mux_reg      MUX register list
   mux_mod      MUX modifiers list
*/

DIB mux_dib = { -1, DEV3_GMUX, 0, NULL, &mux };

REG mux_nlreg = { DRDATA (NLINES, MUX_NUMLIN, 6), PV_LEFT };

UNIT mux_unit = { UDATA (&muxi_svc, UNIT_ATTABLE, 0), MUX_INIT_POLL };

REG mux_reg[] = {
    { BRDATA (STA, mux_sta, 8, 6, MUX_LINES) },
    { BRDATA (RBUF, mux_rbuf, 8, 8, MUX_LINES) },
    { BRDATA (XBUF, mux_xbuf, 8, 8, MUX_LINES) },
    { BRDATA (INT, mux_flags, 8, 1, MUX_SCANMAX) },
    { ORDATA (SCAN, mux_scan, 7) },
    { FLDATA (SLCK, mux_slck, 0) },
    { DRDATA (TPS, mux_tps, 8), REG_NZ + PV_LEFT },
    { NULL }
    };

MTAB mux_mod[] = {
    { MTAB_XTD | MTAB_VDV | MTAB_VAL, 0, "lines", "LINES",
      &mux_vlines, NULL, &mux_nlreg },
    { MTAB_XTD | MTAB_VDV, 1, NULL, "DISCONNECT",
      &tmxr_dscln, NULL, &mux_desc },
    { UNIT_ATT, UNIT_ATT, "connections", NULL, NULL, &mux_summ },
    { MTAB_XTD | MTAB_VDV | MTAB_NMO, 1, "CONNECTIONS", NULL,
      NULL, &mux_show, NULL },
    { MTAB_XTD | MTAB_VDV | MTAB_NMO, 0, "STATISTICS", NULL,
      NULL, &mux_show, NULL },
    { 0 }
    };

DEVICE mux_dev = {
    "MUX", &mux_unit, mux_reg, mux_mod,
    1, 10, 31, 1, 8, 8,
    &tmxr_ex, &tmxr_dep, &mux_reset,
    NULL, &mux_attach, &mux_detach,
    &mux_dib, DEV_NET | DEV_DISABLE
    };

/* MUXL data structures

   muxl_dev     MUXL device descriptor
   muxl_unit    MUXL unit descriptor
   muxl_reg     MUXL register list
   muxl_mod     MUXL modifiers list
*/

UNIT muxl_unit[] = {
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT },
    { UDATA (&muxo_svc, UNIT_UC, 0), MUXL_WAIT }
    };

MTAB muxl_mod[] = {
    { UNIT_UC, 0, "lower case", "LC", NULL },
    { UNIT_UC, UNIT_UC, "upper case", "UC", NULL },
    { MTAB_XTD|MTAB_VUN, 0, NULL, "DISCONNECT",
      &tmxr_dscln, NULL, &mux_desc },
    { MTAB_XTD|MTAB_VUN|MTAB_NC, 0, "LOG", "LOG",
      &tmxr_set_log, &tmxr_show_log, &mux_desc },
    { MTAB_XTD|MTAB_VUN|MTAB_NC, 0, NULL, "NOLOG",
      &tmxr_set_nolog, NULL, &mux_desc },
    { 0 }
    };

REG muxl_reg[] = {
    { URDATA (TIME, muxl_unit[0].wait, 10, 24, 0,
              MUX_LINES, REG_NZ + PV_LEFT) },
    { NULL }
    };

DEVICE muxl_dev = {
    "MUXL", muxl_unit, muxl_reg, muxl_mod,
    MUX_LINES, 10, 31, 1, 8, 8,
    NULL, NULL, &mux_reset,
    NULL, NULL, NULL,
    NULL, 0
    };

/* MUX: IO routine */

/* Mux routine -  EOM 30001 or EOM 77777,2 */

t_stat mux (uint32 fnc, uint32 inst, uint32 *dat)
{
uint32 ln;

switch (fnc) {

    case IO_CONN:                                       /* connect */
        if ((PROJ_GENIE && (inst == 000230001)) ||      /* set alert */
           (!PROJ_GENIE && (inst == 020277777))) alert = POT_MUX;
        else CRETINS;
        break;

    case IO_SKS:                                        /* skip */
        if (PROJ_GENIE && ((inst & 077770077) == 004030001)) {
            ln = SKG_CHAN (inst);                       /* get line */
            if (!sim_is_active (&muxl_unit[ln])) *dat = 1;
            }
        else if (!PROJ_GENIE && ((inst & 077776000) == 024076000)) {
            ln = SKS_CHAN (inst);                       /* get line */
            if (inst & (SKS_XBE|SKS_CRO|SKS_DSR)) *dat = 1;
            if (((inst & SKS_XBE) && sim_is_active (&muxl_unit[ln])) ||
                ((inst & SKS_CRO) && !(mux_sta[ln] & MUX_SCRO)) ||
                ((inst & SKS_DSR) && !(mux_sta[ln] & MUX_SDSR)))
                *dat = 0;                               /* no skip if fail */
            }
        else CRETINS;   

    default:
        return SCPE_IERR;
        }                                               /* end case */

return SCPE_OK;
}

/* PIN routine */

t_stat pin_mux (uint32 num, uint32 *dat)
{
uint32 ln = mux_scan >> 2;
uint32 flag = mux_scan & MUX_FLAGMASK;

mux_scan = mux_scan & MUX_SCANMASK;                     /* mask scan */
mux_flags[mux_scan] = 0;                                /* clear flag */
if (flag == MUX_FRCV) {                                 /* rcv event? */
    *dat = ln | ((uint32) mux_rbuf[ln] << P_V_CHAR) |   /* line + char + */
        ((mux_sta[ln] & MUX_SOVR)? PIN_OVR: 0);         /* overrun */
    mux_sta[ln] = mux_sta[ln] & ~(MUX_SCHP | MUX_SOVR);
    }
else *dat = ln;                                         /* just line */
mux_slck = 0;                                           /* unlock scanner */
mux_scan_next ();                                       /* kick scanner */
return SCPE_OK;
}

t_stat pot_mux (uint32 num, uint32 *dat)
{
uint32 ln = P_CHAN (*dat);
uint32 chr = P_CHAR (*dat);

if (PROJ_GENIE && ((*dat & POT_GLNE) == 0)) {           /* Genie disable? */
    mux_sta[ln] = mux_sta[ln] & ~MUX_SLNE;              /* clear status */
    mux_ldsc[ln].rcve = 0;
    }
else if (!PROJ_GENIE && (*dat & POT_SCDT)) {            /* SDS disable? */
    if (mux_ldsc[ln].conn) {                            /* connected? */
        tmxr_linemsg (&mux_ldsc[ln], "\r\nLine hangup\r\n");
        tmxr_reset_ln (&mux_ldsc[ln]);                  /* reset line */
        mux_reset_ln (ln);                              /* reset state */
        MUX_SETFLG (ln, MUX_FCRF);                      /* set carrier off */
        mux_scan_next ();                               /* kick scanner */
        }
    mux_sta[ln] = mux_sta[ln] & ~MUX_SLNE;              /* clear status */
    mux_ldsc[ln].rcve = 0;
    }
else {                                                  /* enabled */
    if ((*dat & POT_NOX) == 0) {                        /* output char? */
        mux_xbuf[ln] = chr;                             /* store char */
        sim_activate (&muxl_unit[ln], muxl_unit[ln].wait);
        }
    if (*dat & POT_XMI) mux_sta[ln] = mux_sta[ln] | MUX_SXIE;
    else mux_sta[ln] = mux_sta[ln] & ~MUX_SXIE;
    mux_sta[ln] = mux_sta[ln] | MUX_SLNE;               /* line is enabled */
    mux_ldsc[ln].rcve = 1;
    }
return SCPE_OK;
}

/* Unit service - receive side

   Poll all active lines for input
   Poll for new connections
*/

t_stat muxi_svc (UNIT *uptr)
{
int32 ln, c, t;

if ((uptr->flags & UNIT_ATT) == 0) return SCPE_OK;      /* attached? */
t = sim_rtcn_calb (mux_tps, TMR_MUX);                   /* calibrate */
sim_activate (uptr, t);                                 /* continue poll */
ln = tmxr_poll_conn (&mux_desc);                        /* look for connect */
if (ln >= 0) {                                          /* got one? */
    if (!PROJ_GENIE && (mux_sta[ln] & MUX_SLNE)) {      /* modem & DTR? */
        mux_sta[ln] = mux_sta[ln] | (MUX_SCRO|MUX_SDSR);/* carrier on */
        MUX_SETFLG (ln, MUX_FCRN);                      /* set carr on flag */
        mux_scan_next ();                               /* kick scanner */
        }
    mux_ldsc[ln].rcve = 1;                              /* set rcv enable */
    }
tmxr_poll_rx (&mux_desc);                               /* poll for input */
for (ln = 0; ln < MUX_NUMLIN; ln++) {                   /* loop thru lines */
    if (mux_ldsc[ln].conn) {                            /* connected? */
        if (c = tmxr_getc_ln (&mux_ldsc[ln])) {         /* get char */
            if (mux_sta[ln] & MUX_SCHP)                 /* already got one? */
                mux_sta[ln] = mux_sta[ln] | MUX_SOVR;   /* overrun */
            else mux_sta[ln] = mux_sta[ln] | MUX_SCHP;  /* char pending */
            c = c & 0177;                               /* mask to 7b */
            if ((muxl_unit[ln].flags & UNIT_UC) &&      /* cvt to UC? */
                 islower (c & 0x7F)) c = toupper (c);
            mux_rbuf[ln] = c;                           /* save char */
            MUX_SETFLG (ln, MUX_FRCV);                  /* set rcv flag */
            mux_scan_next ();                           /* kick scanner */
            }
        }
    else mux_sta[ln] = 0;                               /* disconnected */
    }                                                   /* end for */
return SCPE_OK;
}

/* Unit service - transmit side */

t_stat muxo_svc (UNIT *uptr)
{
int32 c;
uint32 ln = uptr - muxl_unit;                           /* line # */

if (mux_ldsc[ln].conn) {                                /* connected? */
    if (mux_ldsc[ln].xmte) {                            /* xmt enabled? */
        c = mux_xbuf[ln] & 0177;                        /* get char */
        if ((muxl_unit[ln].flags & UNIT_UC) && islower (c))
            c = toupper (c);                            /* cvt to UC? */
        tmxr_putc_ln (&mux_ldsc[ln], c);                /* output char */
        tmxr_poll_tx (&mux_desc);                       /* poll xmt */
        }
    else {                                              /* buf full */
        tmxr_poll_tx (&mux_desc);                       /* poll xmt */
        sim_activate (uptr, muxl_unit[ln].wait);        /* wait */
        return SCPE_OK;
        }
    }
if (mux_sta[ln] & MUX_SXIE) {
    MUX_SETFLG (ln, MUX_FXMT);                          /* set flag */
    mux_scan_next ();                                   /* kick scanner */
    }
return SCPE_OK;
}

/* Kick scanner */

void mux_scan_next (void)
{
int32 i;

if (mux_slck) return;                                   /* locked? */
for (i = 0; i < MUX_SCANMAX; i++) {                     /* scan flags */
    mux_scan = (mux_scan + 1) & MUX_SCANMASK;           /* next flag */
    if (mux_flags[mux_scan]) {                          /* flag set? */
        mux_slck = 1;                                   /* lock scanner */
        MUX_SETINT (mux_scan & MUX_FLAGMASK);           /* request int */
        return;
        }
    }
return;
}

/* Reset routine */

t_stat mux_reset (DEVICE *dptr)
{
int32 i, t;

if (mux_dev.flags & DEV_DIS)                            /* master disabled? */
    muxl_dev.flags = muxl_dev.flags | DEV_DIS;          /* disable lines */
else muxl_dev.flags = muxl_dev.flags & ~DEV_DIS;
if (mux_unit.flags & UNIT_ATT) {                        /* master att? */
    if (!sim_is_active (&mux_unit)) {
        t = sim_rtcn_init (mux_unit.wait, TMR_MUX);
        sim_activate (&mux_unit, t);                    /* activate */
        }
    }
else sim_cancel (&mux_unit);                            /* else stop */
for (i = 0; i < MUX_LINES; i++) mux_reset_ln (i);
for (i = 0; i < MUX_FLAGS; i++) MUX_CLRINT (i);         /* clear all ints */
return SCPE_OK;
}

/* Attach master unit */

t_stat mux_attach (UNIT *uptr, char *cptr)
{
t_stat r;
int32 t;

r = tmxr_attach (&mux_desc, uptr, cptr);                /* attach */
if (r != SCPE_OK) return r;                             /* error */
t = sim_rtcn_init (mux_unit.wait, TMR_MUX);
sim_activate (uptr, t);                                 /* start poll */
return SCPE_OK;
}

/* Detach master unit */

t_stat mux_detach (UNIT *uptr)
{
int32 i;
t_stat r;

r = tmxr_detach (&mux_desc, uptr);                      /* detach */
for (i = 0; i < MUX_LINES; i++) mux_ldsc[i].rcve = 0;   /* disable rcv */
sim_cancel (uptr);                                      /* stop poll */
return r;
}

/* Show summary processor */

t_stat mux_summ (FILE *st, UNIT *uptr, int32 val, void *desc)
{
int32 i, t;

for (i = t = 0; i < MUX_LINES; i++) t = t + (mux_ldsc[i].conn != 0);
if (t == 1) fprintf (st, "1 connection");
else fprintf (st, "%d connections", t);
return SCPE_OK;
}

/* SHOW CONN/STAT processor */

t_stat mux_show (FILE *st, UNIT *uptr, int32 val, void *desc)
{
int32 i, t;

for (i = t = 0; i < MUX_LINES; i++) t = t + (mux_ldsc[i].conn != 0);
if (t) {
    for (i = 0; i < MUX_LINES; i++) {
        if (mux_ldsc[i].conn) {
            if (val) tmxr_fconns (st, &mux_ldsc[i], i);
            else tmxr_fstats (st, &mux_ldsc[i], i);
            }
        }
    }
else fprintf (st, "all disconnected\n");
return SCPE_OK;
}

/* Change number of lines */

t_stat mux_vlines (UNIT *uptr, int32 val, char *cptr, void *desc)
{
int32 newln, i, t;
t_stat r;

if (cptr == NULL) return SCPE_ARG;
newln = get_uint (cptr, 10, MUX_LINES, &r);
if ((r != SCPE_OK) || (newln == MUX_NUMLIN)) return r;
if (newln == 0) return SCPE_ARG;
if (newln < MUX_NUMLIN) {
    for (i = newln, t = 0; i < MUX_NUMLIN; i++) t = t | mux_ldsc[i].conn;
    if (t && !get_yn ("This will disconnect users; proceed [N]?", FALSE))
            return SCPE_OK;
    for (i = newln; i < MUX_NUMLIN; i++) {
        if (mux_ldsc[i].conn) {
            tmxr_linemsg (&mux_ldsc[i], "\r\nOperator disconnected line\r\n");
            tmxr_reset_ln (&mux_ldsc[i]);               /* reset line */
            }
        muxl_unit[i].flags = muxl_unit[i].flags | UNIT_DIS;
        mux_reset_ln (i);
        }
    }
else {
    for (i = MUX_NUMLIN; i < newln; i++) {
        muxl_unit[i].flags = muxl_unit[i].flags & ~UNIT_DIS;
        mux_reset_ln (i);
        }
    }
MUX_NUMLIN = newln;
return SCPE_OK;
}

/* Reset an individual line */

void mux_reset_ln (int32 ln)
{
int32 flg = ln * MUX_FLAGS;

if (mux_ldsc[ln].conn) mux_sta[ln] = MUX_SCRO | MUX_SDSR;
else mux_sta[ln] = 0;
sim_cancel (&muxl_unit[ln]);
mux_flags[flg + MUX_FRCV] = 0;
mux_flags[flg + MUX_FXMT] = 0;
mux_flags[flg + MUX_FCRN] = 0;
mux_flags[flg + MUX_FCRF] = 0;
return;
}
