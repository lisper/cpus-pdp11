/* hp2100_ms.c: HP 2100 13181A/13183A magnetic tape simulator

   Copyright (c) 1993-2006, Robert M. Supnik

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

   ms           13181A 7970B 800bpi nine track magnetic tape
                13183A 7970E 1600bpi nine track magnetic tape

   07-Jul-06    JDB     Added CAPACITY as alternate for REEL
                        Fixed EOT test for unlimited reel size
   16-Feb-06    RMS     Revised for new EOT test
   22-Jul-05    RMS     Fixed compiler warning on Solaris (from Doug Glyn)
   01-Mar-05    JDB     Added SET OFFLINE; rewind/offline now does not detach
   07-Oct-04    JDB     Fixed enable/disable from either device
   14-Aug-04    JDB     Fixed many functional and timing problems (from Dave Bryan)
                        - fixed erroneous execution of rejected command
                        - fixed erroneous execution of select-only command
                        - fixed erroneous execution of clear command
                        - fixed odd byte handling for read
                        - fixed spurious odd byte status on 13183A EOF
                        - modified handling of end of medium
                        - added detailed timing, with fast and realistic modes
                        - added reel sizes to simulate end of tape
                        - added debug printouts
   06-Jul-04    RMS     Fixed spurious timing error after CLC (found by Dave Bryan)
   26-Apr-04    RMS     Fixed SFS x,C and SFC x,C
                        Fixed SR setting in IBL
                        Revised IBL loader
                        Implemented DMA SRQ (follows FLG)
   25-Apr-03    RMS     Revised for extended file support
   28-Mar-03    RMS     Added multiformat support
   28-Feb-03    RMS     Revised for magtape library
   18-Oct-02    RMS     Added BOOT command, added 13183A support
   30-Sep-02    RMS     Revamped error handling
   29-Aug-02    RMS     Added end of medium support
   30-May-02    RMS     Widened POS to 32b
   22-Apr-02    RMS     Added maximum record length test

   Magnetic tapes are represented as a series of variable records
   of the form:

        32b byte count
        byte 0
        byte 1
        :
        byte n-2
        byte n-1
        32b byte count

   If the byte count is odd, the record is padded with an extra byte
   of junk.  File marks are represented by a byte count of 0.

   References:
   - 13181B Digital Magnetic Tape Unit Interface Kit Operating and Service Manual
            (13181-90901, Nov-1982)
   - 13183B Digital Magnetic Tape Unit Interface Kit Operating and Service Manual
            (13183-90901, Nov-1983)
*/

#include "hp2100_defs.h"
#include "sim_tape.h"

#define UNIT_V_OFFLINE  (MTUF_V_UF + 0)                 /* unit offline */
#define UNIT_OFFLINE    (1 << UNIT_V_OFFLINE)

#define MS_NUMDR        4                               /* number of drives */
#define DB_N_SIZE       16                              /* max data buf */
#define DBSIZE          (1 << DB_N_SIZE)                /* max data cmd */
#define FNC             u3                              /* function */
#define UST             u4                              /* unit status */
#define REEL            u5                              /* tape reel size */

#define TCAP            (300 * 12 * 800)                /* 300 ft capacity at 800bpi */

/* Command - msc_fnc */

#define FNC_CLR         00110                           /* clear */
#define FNC_GAP         00015                           /* write gap */
#define FNC_GFM         00215                           /* gap+file mark */
#define FNC_RC          00023                           /* read */
#define FNC_WC          00031                           /* write */
#define FNC_FSR         00003                           /* forward space */
#define FNC_BSR         00041                           /* backward space */
#define FNC_FSF         00203                           /* forward file */
#define FNC_BSF         00241                           /* backward file */
#define FNC_REW         00101                           /* rewind */
#define FNC_RWS         00105                           /* rewind and offline */
#define FNC_WFM         00211                           /* write file mark */
#define FNC_RFF         00223                           /* "read file fwd" */
#define FNC_CMPL        00400                           /* completion state */
#define FNC_V_SEL       9                               /* select */
#define FNC_M_SEL       017
#define FNC_GETSEL(x)   (((x) >> FNC_V_SEL) & FNC_M_SEL)

#define FNF_MOT         00001                           /* motion */
#define FNF_OFL         00004
#define FNF_WRT         00010                           /* write */
#define FNF_REV         00040                           /* reverse */
#define FNF_RWD         00100                           /* rewind */
#define FNF_CHS         00400                           /* change select */

#define FNC_SEL         ((FNC_M_SEL << FNC_V_SEL) | FNF_CHS)

/* Status - stored in msc_sta, unit.UST (u), or dynamic (d) */

#define STA_PE          0100000                         /* 1600 bpi (d) */
#define STA_V_SEL       13                              /* unit sel (d) */
#define STA_M_SEL       03
#define STA_SEL         (STA_M_SEL << STA_V_SEL)
#define STA_ODD         0004000                         /* odd bytes */
#define STA_REW         0002000                         /* rewinding (u) */
#define STA_TBSY        0001000                         /* transport busy (d) */
#define STA_BUSY        0000400                         /* ctrl busy */
#define STA_EOF         0000200                         /* end of file */
#define STA_BOT         0000100                         /* beg of tape (u) */
#define STA_EOT         0000040                         /* end of tape (d) */
#define STA_TIM         0000020                         /* timing error */
#define STA_REJ         0000010                         /* programming error */
#define STA_WLK         0000004                         /* write locked (d) */
#define STA_PAR         0000002                         /* parity error */
#define STA_LOCAL       0000001                         /* local (d) */
#define STA_DYN         (STA_PE|STA_SEL|STA_TBSY|STA_WLK|STA_LOCAL)

extern uint32 PC, SR;
extern uint32 dev_cmd[2], dev_ctl[2], dev_flg[2], dev_fbf[2], dev_srq[2];
extern int32 sim_switches;
extern FILE *sim_deb;

int32 ms_ctype = 0;                                     /* ctrl type */
int32 ms_timing = 1;                                    /* timing type */
int32 msc_sta = 0;                                      /* status */
int32 msc_buf = 0;                                      /* buffer */
int32 msc_usl = 0;                                      /* unit select */
int32 msc_1st = 0;
int32 msc_stopioe = 1;                                  /* stop on error */
int32 msd_buf = 0;                                      /* data buffer */
uint8 msxb[DBSIZE] = { 0 };                             /* data buffer */
t_mtrlnt ms_ptr = 0, ms_max = 0;                        /* buffer ptrs */

/* Hardware timing at 45 IPS                  13181                  13183           
   (based on 1580 instr/msec)          instr   msec    SCP   instr    msec    SCP
                                      --------------------   --------------------
   - BOT start delay        : btime = 161512  102.22   184   252800  160.00   288
   - motion cmd start delay : ctime =  14044    8.89    16    17556   11.11    20
   - GAP traversal time     : gtime = 175553  111.11   200   105333   66.67   120
   - IRG traversal time     : itime =  24885   15.75     -    27387   17.33     -
   - rewind initiation time : rtime =    878    0.56     1      878    0.56     1
   - data xfer time / word  : xtime =     88   55.56us   -       44  27.78us    - 

   NOTE: The 13181-60001 Rev. 1629 tape diagnostic fails test 17B subtest 6 with
         "E116 BYTE TIME SHORT" if the correct data transfer time is used for
          13181A interface.  Set "xtime" to 115 (instructions) to pass that
          diagnostic.  Rev. 2040 of the tape diagnostic fixes this problem and
          passes with the correct data transfer time.
*/

int32 msc_btime = 0;                                    /* BOT start delay */
int32 msc_ctime = 0;                                    /* motion cmd start delay */
int32 msc_gtime = 0;                                    /* GAP traversal time */
int32 msc_itime = 0;                                    /* IRG traversal time */
int32 msc_rtime = 0;                                    /* rewind initiation time */
int32 msc_xtime = 0;                                    /* data xfer time / word */

typedef int32 TIMESET[6];                               /* set of controller times */

int32 *const timers[] = { &msc_btime, &msc_ctime, &msc_gtime,
                          &msc_itime, &msc_rtime, &msc_xtime };

const TIMESET msc_times[3] = {
    { 161512, 14044, 175553, 24885, 878,  88 },         /* 13181A */
    { 252800, 17556, 105333, 27387, 878,  44 },         /* 13183A */
    {      1,  1000,      1,     1, 100,  10 }          /* FAST */
    };

DEVICE msd_dev, msc_dev;
int32 msdio (int32 inst, int32 IR, int32 dat);
int32 mscio (int32 inst, int32 IR, int32 dat);
t_stat msc_svc (UNIT *uptr);
t_stat msc_reset (DEVICE *dptr);
t_stat msc_attach (UNIT *uptr, char *cptr);
t_stat msc_detach (UNIT *uptr);
t_stat msc_online (UNIT *uptr, int32 value, char *cptr, void *desc);
t_stat msc_boot (int32 unitno, DEVICE *dptr);
t_stat ms_map_err (UNIT *uptr, t_stat st);
t_stat ms_settype (UNIT *uptr, int32 val, char *cptr, void *desc);
t_stat ms_showtype (FILE *st, UNIT *uptr, int32 val, void *desc);
t_stat ms_set_timing (UNIT *uptr, int32 val, char *cptr, void *desc);
t_stat ms_show_timing (FILE *st, UNIT *uptr, int32 val, void *desc);
t_stat ms_set_reelsize (UNIT *uptr, int32 val, char *cptr, void *desc);
t_stat ms_show_reelsize (FILE *st, UNIT *uptr, int32 val, void *desc);
void ms_config_timing (void);

/* MSD data structures

   msd_dev      MSD device descriptor
   msd_unit     MSD unit list
   msd_reg      MSD register list
*/

DIB ms_dib[] = {
    { MSD, 0, 0, 0, 0, 0, &msdio },
    { MSC, 0, 0, 0, 0, 0, &mscio }
    };

#define msd_dib ms_dib[0]
#define msc_dib ms_dib[1]

UNIT msd_unit = { UDATA (NULL, 0, 0) };

REG msd_reg[] = {
    { ORDATA (BUF, msd_buf, 16) },
    { FLDATA (CMD, msd_dib.cmd, 0), REG_HRO },
    { FLDATA (CTL, msd_dib.ctl, 0) },
    { FLDATA (FLG, msd_dib.flg, 0) },
    { FLDATA (FBF, msd_dib.fbf, 0) },
    { FLDATA (SRQ, msd_dib.srq, 0) },
    { BRDATA (DBUF, msxb, 8, 8, DBSIZE) },
    { DRDATA (BPTR, ms_ptr, DB_N_SIZE + 1) },
    { DRDATA (BMAX, ms_max, DB_N_SIZE + 1) },
    { ORDATA (DEVNO, msd_dib.devno, 6), REG_HRO },
    { NULL }
    };

MTAB msd_mod[] = {
    { MTAB_XTD | MTAB_VDV, 1, "DEVNO", "DEVNO",
      &hp_setdev, &hp_showdev, &msd_dev },
    { 0 }
    };

DEVICE msd_dev = {
    "MSD", &msd_unit, msd_reg, msd_mod,
    1, 10, DB_N_SIZE, 1, 8, 8,
    NULL, NULL, &msc_reset,
    NULL, NULL, NULL,
    &msd_dib, DEV_DISABLE
    };

/* MSC data structures

   msc_dev      MSC device descriptor
   msc_unit     MSC unit list
   msc_reg      MSC register list
   msc_mod      MSC modifier list
*/

UNIT msc_unit[] = {
    { UDATA (&msc_svc, UNIT_ATTABLE | UNIT_ROABLE |
             UNIT_DISABLE | UNIT_OFFLINE, 0) },
    { UDATA (&msc_svc, UNIT_ATTABLE | UNIT_ROABLE |
             UNIT_DISABLE | UNIT_OFFLINE, 0) },
    { UDATA (&msc_svc, UNIT_ATTABLE | UNIT_ROABLE |
             UNIT_DISABLE | UNIT_OFFLINE, 0) },
    { UDATA (&msc_svc, UNIT_ATTABLE | UNIT_ROABLE |
             UNIT_DISABLE | UNIT_OFFLINE, 0) }
    };

REG msc_reg[] = {
    { ORDATA (STA, msc_sta, 12) },
    { ORDATA (BUF, msc_buf, 16) },
    { ORDATA (USEL, msc_usl, 2) },
    { FLDATA (FSVC, msc_1st, 0) },
    { FLDATA (CMD, msc_dib.cmd, 0), REG_HRO },
    { FLDATA (CTL, msc_dib.ctl, 0) },
    { FLDATA (FLG, msc_dib.flg, 0) },
    { FLDATA (FBF, msc_dib.fbf, 0) },
    { FLDATA (SRQ, msc_dib.srq, 0) },
    { URDATA (POS, msc_unit[0].pos, 10, T_ADDR_W, 0, MS_NUMDR, PV_LEFT) },
    { URDATA (FNC, msc_unit[0].FNC, 8, 8, 0, MS_NUMDR, REG_HRO) },
    { URDATA (UST, msc_unit[0].UST, 8, 12, 0, MS_NUMDR, REG_HRO) },
    { URDATA (REEL, msc_unit[0].REEL, 10, 2, 0, MS_NUMDR, REG_HRO) },
    { DRDATA (BTIME, msc_btime, 24), REG_NZ + PV_LEFT },
    { DRDATA (CTIME, msc_ctime, 24), REG_NZ + PV_LEFT },
    { DRDATA (GTIME, msc_gtime, 24), REG_NZ + PV_LEFT },
    { DRDATA (ITIME, msc_itime, 24), REG_NZ + PV_LEFT },
    { DRDATA (RTIME, msc_rtime, 24), REG_NZ + PV_LEFT },
    { DRDATA (XTIME, msc_xtime, 24), REG_NZ + PV_LEFT },
    { FLDATA (TIMING, ms_timing, 0), REG_HRO },
    { FLDATA (STOP_IOE, msc_stopioe, 0) },
    { FLDATA (CTYPE, ms_ctype, 0), REG_HRO },
    { ORDATA (DEVNO, msc_dib.devno, 6), REG_HRO },
    { NULL }
    };

MTAB msc_mod[] = {
    { UNIT_OFFLINE, UNIT_OFFLINE, "offline", "OFFLINE", NULL },
    { UNIT_OFFLINE, 0, "online", "ONLINE", msc_online },
    { MTUF_WLK, 0, "write enabled", "WRITEENABLED", NULL },
    { MTUF_WLK, MTUF_WLK, "write locked", "LOCKED", NULL }, 
    { MTAB_XTD | MTAB_VUN, 0, "CAPACITY", "CAPACITY",
       &ms_set_reelsize, &ms_show_reelsize, NULL },
    { MTAB_XTD | MTAB_VUN | MTAB_NMO, 1, "REEL", "REEL",
      &ms_set_reelsize, &ms_show_reelsize, NULL },
    { MTAB_XTD | MTAB_VUN, 0, "FORMAT", "FORMAT",
      &sim_tape_set_fmt, &sim_tape_show_fmt, NULL },
    { MTAB_XTD | MTAB_VDV, 0, NULL, "13181A",
      &ms_settype, NULL, NULL },
    { MTAB_XTD | MTAB_VDV, 1, NULL, "13183A",
      &ms_settype, NULL, NULL },
    { MTAB_XTD | MTAB_VDV, 0, "TYPE", NULL,
      NULL, &ms_showtype, NULL },
    { MTAB_XTD | MTAB_VDV, 0, NULL, "REALTIME",
      &ms_set_timing, NULL, NULL },
    { MTAB_XTD | MTAB_VDV, 1, NULL, "FASTTIME",
      &ms_set_timing, NULL, NULL },
    { MTAB_XTD | MTAB_VDV, 0, "TIMING", NULL,
      NULL, &ms_show_timing, NULL },
    { MTAB_XTD | MTAB_VDV, 1, "DEVNO", "DEVNO",
      &hp_setdev, &hp_showdev, &msd_dev },
    { 0 }
    };

DEVICE msc_dev = {
    "MSC", msc_unit, msc_reg, msc_mod,
    MS_NUMDR, 10, 31, 1, 8, 8,
    NULL, NULL, &msc_reset,
    &msc_boot, &msc_attach, &msc_detach,
    &msc_dib, DEV_DISABLE | DEV_DEBUG
    };

/* IO instructions */

int32 msdio (int32 inst, int32 IR, int32 dat)
{
int32 devd;

devd = IR & I_DEVMASK;                                  /* get device no */
switch (inst) {                                         /* case on opcode */

    case ioFLG:                                         /* flag clear/set */
        if ((IR & I_HC) == 0) { setFSR (devd); }        /* STF */
        break;

    case ioSFC:                                         /* skip flag clear */
        if (FLG (devd) == 0) PC = (PC + 1) & VAMASK;
        break;

    case ioSFS:                                         /* skip flag set */
        if (FLG (devd) != 0) PC = (PC + 1) & VAMASK;
        break;

    case ioOTX:                                         /* output */
        msd_buf = dat;                                  /* store data */
        break;

    case ioMIX:                                         /* merge */
        dat = dat | msd_buf;
        break;

    case ioLIX:                                         /* load */
        dat = msd_buf;
        break;

    case ioCTL:                                         /* control clear/set */
        if (IR & I_CTL) {                               /* CLC */
            clrCTL (devd);                              /* clr ctl, cmd */
            clrCMD (devd);
            }
        else {                                          /* STC */
            setCTL (devd);                              /* set ctl, cmd */
            setCMD (devd);
            }
        break;

    case ioEDT:                                         /* DMA end */
        clrFSR (devd);                                  /* same as CLF */
        break;

    default:
        break;
        }

if (IR & I_HC) { clrFSR (devd); }                       /* H/C option */
return dat;
}

int32 mscio (int32 inst, int32 IR, int32 dat)
{
int32 i, devc, devd, sched_time;
t_stat st;
UNIT *uptr = msc_dev.units + msc_usl;
static const uint8 map_sel[16] = {
    0, 0, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3
    };

devc = IR & I_DEVMASK;                                  /* get device no */
devd = devc - 1;
switch (inst) {                                         /* case on opcode */

    case ioFLG:                                         /* flag clear/set */
        if ((IR & I_HC) == 0) { setFSR (devc); }        /* STF */
        break;

    case ioSFC:                                         /* skip flag clear */
        if (FLG (devc) == 0) PC = (PC + 1) & VAMASK;
        break;

    case ioSFS:                                         /* skip flag set */
        if (FLG (devc) != 0) PC = (PC + 1) & VAMASK;
        break;

    case ioOTX:                                         /* output */
        if (DEBUG_PRS (msc_dev))
            fprintf (sim_deb, ">>MSC OTx: Command = %06o\n", dat);
        msc_buf = dat;
        msc_sta = msc_sta & ~STA_REJ;                   /* clear reject */
        if ((dat & 0377) == FNC_CLR) break;             /* clear always ok */
        if (msc_sta & STA_BUSY) {                       /* busy? reject */
            msc_sta = msc_sta | STA_REJ;                /* dont chg select */
            break;
            }
        if (dat & FNF_CHS) {                            /* select change */
            msc_usl = map_sel[FNC_GETSEL (dat)];        /* is immediate */
            uptr = msc_dev.units + msc_usl;
            }
        if (((dat & FNF_MOT) && sim_is_active (uptr)) ||
            ((dat & FNF_REV) && (uptr->UST & STA_BOT)) ||
            ((dat & FNF_WRT) && sim_tape_wrp (uptr)))
            msc_sta = msc_sta | STA_REJ;                /* reject? */
        break;

    case ioLIX:                                         /* load */
        dat = 0;
    case ioMIX:                                         /* merge */
        dat = dat | (msc_sta & ~STA_DYN);               /* get card status */
        if ((uptr->flags & UNIT_OFFLINE) == 0) {        /* online? */
            dat = dat | uptr->UST;                      /* add unit status */
            if (sim_is_active (uptr) &&                 /* TBSY unless RWD at BOT */
                !((uptr->FNC & FNF_RWD) && (uptr->UST & STA_BOT)))
                dat = dat | STA_TBSY;
            if (sim_tape_wrp (uptr))                    /* write prot? */
                dat = dat | STA_WLK;
            if (sim_tape_eot (uptr))
                dat = dat | STA_EOT;
            }
        else dat = dat | STA_TBSY | STA_LOCAL;
        if (ms_ctype) dat = dat | STA_PE |              /* 13183A? */
            (msc_usl << STA_V_SEL);     
        if (DEBUG_PRS (msc_dev))
            fprintf (sim_deb, ">>MSC LIx: Status = %06o\n", dat);
        break;

    case ioCTL:                                         /* control clear/set */
        if (IR & I_CTL) { clrCTL (devc); }              /* CLC */
        else if (!(msc_sta & STA_REJ)) {                /* STC, last cmd rejected? */
            if ((msc_buf & 0377) == FNC_CLR) {          /* clear? */
                for (i = 0; i < MS_NUMDR; i++) {        /* loop thru units */
                    if (sim_is_active (&msc_unit[i]) && /* write in prog? */
                        (msc_unit[i].FNC == FNC_WC) && (ms_ptr > 0)) {
                        if (st = sim_tape_wrrecf (uptr, msxb, ms_ptr | MTR_ERF))
                            ms_map_err (uptr, st);
                        }
                    if ((msc_unit[i].UST & STA_REW) == 0)
                        sim_cancel (&msc_unit[i]);      /* stop if not rew */
                    }
                setCTL (devc);                          /* set CTL for STC */
                setFSR (devc);                          /* set FLG for completion */
                msc_sta = msc_1st = 0;                  /* clr ctlr status */
                if (DEBUG_PRS (msc_dev))
                    fputs (">>MSC STC: Controller cleared\n", sim_deb);
                return SCPE_OK;
                }
            uptr->FNC = msc_buf & 0377;                 /* save function */
            if (uptr->FNC & FNF_RWD) {                  /* rewind? */
                if (!(uptr->UST & STA_BOT))             /* not at BOT? */
                    uptr->UST = STA_REW;                /* set rewinding */
                sched_time = msc_rtime;                 /* set response time */
                }
            else {
                if (uptr-> UST & STA_BOT)               /* at BOT? */
                    sched_time = msc_btime;             /* use BOT start time */
                else if ((uptr->FNC == FNC_GAP) || (uptr->FNC == FNC_GFM))
                    sched_time = msc_gtime;             /* use gap traversal time */
                else sched_time = 0;
                if (uptr->FNC != FNC_GAP)
                    sched_time += msc_ctime;            /* add base command time */
                if (uptr->FNC & FNF_MOT)                /* motion command? */
                    uptr->UST = 0;                      /* clear BOT status */
                }
            if (msc_buf & ~FNC_SEL) {                   /* NOP for unit sel alone */
                sim_activate (uptr, sched_time);        /* else schedule op */
                if (DEBUG_PRS (msc_dev)) fprintf (sim_deb,
                    ">>MSC STC: Unit %d command %03o scheduled, time = %d\n",
                    msc_usl, uptr->FNC, sched_time);
                }
            else if (DEBUG_PRS (msc_dev))
                fputs (">>MSC STC: Unit select (NOP)\n", sim_deb);
            msc_sta = STA_BUSY;                         /* ctrl is busy */
            msc_1st = 1;
            setCTL (devc);                              /* go */
            }
        break;

    case ioEDT:                                         /* DMA end */
        clrFSR (devc);                                  /* same as CLF */
        break;

    default:
        break;
        }

if (IR & I_HC) { clrFSR (devc); }                       /* H/C option */
return dat;
}

/* Unit service

   If rewind done, reposition to start of tape, set status
   else, do operation, set done, interrupt

   Can't be write locked, can only write lock detached unit
*/

t_stat msc_svc (UNIT *uptr)
{
int32 devc, devd, unum;
t_mtrlnt tbc;
t_stat st, r = SCPE_OK;

devc = msc_dib.devno;                                   /* get device nos */
devd = msd_dib.devno;
unum = uptr - msc_dev.units;                            /* get unit number */

if ((uptr->FNC != FNC_RWS) && (uptr->flags & UNIT_OFFLINE)) {  /* offline? */
    msc_sta = (msc_sta | STA_REJ) & ~STA_BUSY;          /* reject */
    setFSR (devc);                                      /* set cch flg */
    return IORETURN (msc_stopioe, SCPE_UNATT);
    }

switch (uptr->FNC) {                                    /* case on function */

    case FNC_RWS:                                       /* rewind offline */
        sim_tape_rewind (uptr);                         /* rewind tape */
        uptr->flags = uptr->flags | UNIT_OFFLINE;       /* set offline */
        uptr->UST = STA_BOT;                            /* BOT when online again */
        break;                                          /* we're done */

    case FNC_REW:                                       /* rewind */
        if (uptr->UST & STA_REW) {                      /* rewind in prog? */
            uptr->FNC |= FNC_CMPL;                      /* set compl state */
            sim_activate (uptr, msc_ctime);             /* sched completion */
            }
        break;                                          /* anyway, ctrl done */

    case FNC_REW | FNC_CMPL:                            /* complete rewind */
        sim_tape_rewind (uptr);                         /* rewind tape */
        uptr->UST = STA_BOT;                            /* set BOT status */
        return SCPE_OK;                                 /* drive is free */

    case FNC_GFM:                                       /* gap file mark */
    case FNC_WFM:                                       /* write file mark */
        if (st = sim_tape_wrtmk (uptr))                 /* write tmk, err? */
            r = ms_map_err (uptr, st);                  /* map error */
        msc_sta = STA_EOF;                              /* set EOF status */
        break;

    case FNC_GAP:                                       /* erase gap */
        break;

    case FNC_FSR:                                       /* space forward */
        if (st = sim_tape_sprecf (uptr, &tbc))          /* space rec fwd, err? */
            r = ms_map_err (uptr, st);                  /* map error */
        if (tbc & 1) msc_sta = msc_sta | STA_ODD;
        else msc_sta = msc_sta & ~STA_ODD;
        break;

    case FNC_BSR:                                       /* space reverse */
        if (st = sim_tape_sprecr (uptr, &tbc))          /* space rec rev, err? */
            r = ms_map_err (uptr, st);                  /* map error */
        if (tbc & 1) msc_sta = msc_sta | STA_ODD;
        else msc_sta = msc_sta & ~STA_ODD;
        break;

    case FNC_FSF:                                       /* space fwd file */
        while ((st = sim_tape_sprecf (uptr, &tbc)) == MTSE_OK) {
            if (sim_tape_eot (uptr)) break;             /* EOT stops */
            }
        r = ms_map_err (uptr, st);                      /* map error */
        break;

    case FNC_BSF:                                       /* space rev file */
        while ((st = sim_tape_sprecr (uptr, &tbc)) == MTSE_OK) ;
        r = ms_map_err (uptr, st);                      /* map error */
        break;

    case FNC_RFF:                                       /* diagnostic read */
    case FNC_RC:                                        /* read */
        if (msc_1st) {                                  /* first svc? */
            msc_1st = ms_ptr = 0;                       /* clr 1st flop */
            st = sim_tape_rdrecf (uptr, msxb, &ms_max, DBSIZE); /* read rec */
            if (st == MTSE_RECE) msc_sta = msc_sta | STA_PAR;   /* rec in err? */
            else if (st != MTSE_OK) {                   /* other error? */
                r = ms_map_err (uptr, st);              /* map error */
                if (r == SCPE_OK) {                     /* recoverable? */
                    sim_activate (uptr, msc_itime);     /* sched IRG */
                    uptr->FNC |= FNC_CMPL;              /* set completion */
                    return SCPE_OK;
                    }
                break;                                  /* err, done */
                }
            if (ms_ctype) msc_sta = msc_sta | STA_ODD;  /* set ODD for 13183A */
            if (DEBUG_PRS (msc_dev)) fprintf (sim_deb,
                ">>MSC svc: Unit %d read %d word record\n", unum, ms_max / 2);
            }
        if (CTL (devd) && (ms_ptr < ms_max)) {          /* DCH on, more data? */
            if (FLG (devd)) msc_sta = msc_sta | STA_TIM | STA_PAR;
            msd_buf = ((uint16) msxb[ms_ptr] << 8) |
                      ((ms_ptr + 1 == ms_max) ? 0 : msxb[ms_ptr + 1]);
            ms_ptr = ms_ptr + 2;
            setFSR (devd);                              /* set dch flg */
            sim_activate (uptr, msc_xtime);             /* re-activate */
            return SCPE_OK;
            }
        if (ms_max & 1) msc_sta = msc_sta | STA_ODD;    /* set ODD by rec len */
        else msc_sta = msc_sta & ~STA_ODD;
        sim_activate (uptr, msc_itime);                 /* sched IRG */
        if (uptr->FNC == FNC_RFF) msc_1st = 1;          /* diagnostic? */
        else uptr->FNC |= FNC_CMPL;                     /* set completion */
        return SCPE_OK;

    case FNC_WC:                                        /* write */
        if (msc_1st) msc_1st = ms_ptr = 0;              /* no xfer on first */
        else {                                          /* not 1st, next char */
            if (ms_ptr < DBSIZE) {                      /* room in buffer? */
                msxb[ms_ptr] = msd_buf >> 8;            /* store 2 char */
                msxb[ms_ptr + 1] = msd_buf & 0377;
                ms_ptr = ms_ptr + 2;
                uptr->UST = 0;
                }
            else msc_sta = msc_sta | STA_PAR;
           }
        if (CTL (devd)) {                               /* xfer flop set? */
            setFSR (devd);                              /* set dch flag */
            sim_activate (uptr, msc_xtime);             /* re-activate */
            return SCPE_OK;
            }
        if (ms_ptr) {                                   /* any data? write */
            if (DEBUG_PRS (msc_dev)) fprintf (sim_deb,
                ">>MSC svc: Unit %d wrote %d word record\n", unum, ms_ptr / 2);
            if (st = sim_tape_wrrecf (uptr, msxb, ms_ptr)) {    /* write, err? */
                r = ms_map_err (uptr, st);              /* map error */
                break;
                }
            }
        sim_activate (uptr, msc_itime);                 /* sched IRG */
        uptr->FNC |= FNC_CMPL;                          /* set completion */
        return SCPE_OK;

    default:                                            /* unknown */
        break;
        }

setFSR (devc);                                          /* set cch flg */
msc_sta = msc_sta & ~STA_BUSY;                          /* update status */
if (DEBUG_PRS (msc_dev)) fprintf (sim_deb,
    ">>MSC svc: Unit %d command %03o complete\n", unum, uptr->FNC & 0377);
return r;
}

/* Map tape error status */

t_stat ms_map_err (UNIT *uptr, t_stat st)
{
int32 unum = uptr - msc_dev.units;                      /* get unit number */

if (DEBUG_PRS (msc_dev)) fprintf (sim_deb,
    ">>MSC err: Unit %d tape library status = %d\n", unum, st);

switch (st) {

    case MTSE_FMT:                                      /* illegal fmt */
    case MTSE_UNATT:                                    /* unattached */
        msc_sta = msc_sta | STA_REJ;                    /* reject */
    case MTSE_OK:                                       /* no error */
        return SCPE_IERR;                               /* never get here! */

    case MTSE_EOM:                                      /* end of medium */
    case MTSE_TMK:                                      /* end of file */
        msc_sta = msc_sta | STA_EOF | (ms_ctype ? 0 : STA_ODD);
        break;                                          /* EOF also sets ODD for 13181A */

    case MTSE_INVRL:                                    /* invalid rec lnt */
        msc_sta = msc_sta | STA_PAR;
        return SCPE_MTRLNT;

    case MTSE_IOERR:                                    /* IO error */
        msc_sta = msc_sta | STA_PAR;                    /* error */
        if (msc_stopioe) return SCPE_IOERR;
        break;

    case MTSE_RECE:                                     /* record in error */
        msc_sta = msc_sta | STA_PAR;                    /* error */
        break;

    case MTSE_BOT:                                      /* reverse into BOT */
        msc_sta = msc_sta | STA_BOT;                    /* set BOT status */
        break;

    case MTSE_WRP:                                      /* write protect */
        msc_sta = msc_sta | STA_REJ;                    /* reject */
        break;
        }

return SCPE_OK;
}

/* Reset routine */

t_stat msc_reset (DEVICE *dptr)
{
int32 i;
UNIT *uptr;

hp_enbdis_pair (dptr,                                   /* make pair cons */
    (dptr == &msd_dev)? &msc_dev: &msd_dev);
msc_buf = msd_buf = 0;
msc_sta = msc_usl = 0;
msc_1st = 0;
msc_dib.cmd = msd_dib.cmd = 0;                          /* clear cmd */
msc_dib.ctl = msd_dib.ctl = 0;                          /* clear ctl */
msc_dib.flg = msd_dib.flg = 1;                          /* set flg */
msc_dib.fbf = msd_dib.fbf = 1;                          /* set fbf */
msc_dib.srq = msd_dib.srq = 1;                          /* srq follows flg */
for (i = 0; i < MS_NUMDR; i++) {
    uptr = msc_dev.units + i;
    sim_tape_reset (uptr);
    sim_cancel (uptr);
    if ((uptr->flags & UNIT_ATT) && sim_tape_bot (uptr))
        uptr->UST = STA_BOT;
    else uptr->UST = 0;
    }
ms_config_timing ();
return SCPE_OK;
}

/* Attach routine */

t_stat msc_attach (UNIT *uptr, char *cptr)
{
t_stat r;

r = sim_tape_attach (uptr, cptr);                       /* attach unit */
if (r == SCPE_OK) {
    uptr->flags = uptr->flags & ~UNIT_OFFLINE;          /* set online */
    uptr->UST = STA_BOT;                                /* tape starts at BOT */
    }
return r;
}

/* Detach routine */

t_stat msc_detach (UNIT* uptr)
{
uptr->UST = 0;                                          /* update status */
uptr->flags = uptr->flags | UNIT_OFFLINE;               /* set offline */
return sim_tape_detach (uptr);                          /* detach unit */
}

/* Online routine */

t_stat msc_online (UNIT *uptr, int32 value, char *cptr, void *desc)
{
if (uptr->flags & UNIT_ATT) return SCPE_OK;
else return SCPE_UNATT;
}

/* Configure timing */

void ms_config_timing (void)
{
int32 i, tset;

tset = (ms_timing << 1) | (ms_timing? 0 : ms_ctype);    /* select timing set */
for (i = 0; i < (sizeof (timers) / sizeof (timers[0])); i++)
    *timers[i] = msc_times[tset][i];                    /* assign times */
}

/* Set controller timing */

t_stat ms_set_timing (UNIT *uptr, int32 val, char *cptr, void *desc)
{
if ((val < 0) || (val > 1) || (cptr != NULL)) return SCPE_ARG;
ms_timing = val;
ms_config_timing ();
return SCPE_OK;
}

/* Show controller timing */

t_stat ms_show_timing (FILE *st, UNIT *uptr, int32 val, void *desc)
{
if (ms_timing) fputs ("fast timing", st);
else fputs ("realistic timing", st);
return SCPE_OK;
}

/* Set controller type */

t_stat ms_settype (UNIT *uptr, int32 val, char *cptr, void *desc)
{
int32 i;

if ((val < 0) || (val > 1) || (cptr != NULL)) return SCPE_ARG;
for (i = 0; i < MS_NUMDR; i++) {
    if (msc_unit[i].flags & UNIT_ATT) return SCPE_ALATT;
    }
ms_ctype = val;
ms_config_timing ();                                    /* update for new type */
return SCPE_OK;
}

/* Show controller type */

t_stat ms_showtype (FILE *st, UNIT *uptr, int32 val, void *desc)
{
if (ms_ctype) fprintf (st, "13183A");
else fprintf (st, "13181A");
return SCPE_OK;
}

/* Set unit reel size

   val = 0 -> SET MSCn CAPACITY=n
   val = 1 -> SET MSCn REEL=n */
 
t_stat ms_set_reelsize (UNIT *uptr, int32 val, char *cptr, void *desc)
{
int32 reel;
t_stat status;
 
if (val == 0) {
    status = sim_tape_set_capac (uptr, val, cptr, desc);
    if (status == SCPE_OK) uptr->REEL = 0;
    return status;
    }

if (cptr == NULL) return SCPE_ARG;
reel = (int32) get_uint (cptr, 10, 2400, &status);
if (status != SCPE_OK) return status;
else switch (reel) {
 
     case 0:
        uptr->REEL = 0;                                 /* type 0 = unlimited/custom */
        break;

    case 600:
        uptr->REEL = 1;                                 /* type 1 = 600 foot */
        break;

    case 1200:
        uptr->REEL = 2;                                 /* type 2 = 1200 foot */
        break;

    case 2400:
        uptr->REEL = 3;                                 /* type 3 = 2400 foot */
        break;

    default:
        return SCPE_ARG;
        }

uptr->capac = uptr->REEL? (TCAP << uptr->REEL) << ms_ctype: 0;
return SCPE_OK;
}

/* Show unit reel size

   val = 0 -> SHOW MSC or SHOW MSCn or SHOW MSCn CAPACITY
   val = 1 -> SHOW MSCn REEL */
 
t_stat ms_show_reelsize (FILE *st, UNIT *uptr, int32 val, void *desc)
{
t_stat status = SCPE_OK;

if (uptr->REEL == 0) status = sim_tape_show_capac (st, uptr, val, desc);
else fprintf (st, "%4d foot reel", 300 << uptr->REEL);
if (val == 1) fputc ('\n', st);                         /* MTAB_NMO omits \n */
return status;
}

/* 7970B/7970E bootstrap routine (HP 12992D ROM) */

const uint16 ms_rom[IBL_LNT] = {
    0106501,                    /*ST LIB 1              ; read sw */
    0006011,                    /*   SLB,RSS            ; bit 0 set? */
    0027714,                    /*   JMP RD             ; no read */
    0003004,                    /*   CMA,INA            ; A is ctr */
    0073775,                    /*   STA WC             ; save */
    0067772,                    /*   LDA SL0RW          ; sel 0, rew */
    0017762,                    /*FF JSB CMD            ; do cmd */
    0102311,                    /*   SFS CC             ; done? */
    0027707,                    /*   JMP *-1            ; wait */
    0067774,                    /*   LDB FFC            ; get file fwd */
    0037775,                    /*   ISZ WC             ; done files? */
    0027706,                    /*   JMP FF             ; no */
    0067773,                    /*RD LDB RDCMD          ; read cmd */
    0017762,                    /*   JSB CMD            ; do cmd */
    0103710,                    /*   STC DC,C           ; start dch */
    0102211,                    /*   SFC CC             ; read done? */
    0027752,                    /*   JMP STAT           ; no, get stat */
    0102310,                    /*   SFS DC             ; any data? */
    0027717,                    /*   JMP *-3            ; wait */
    0107510,                    /*   LIB DC,C           ; get rec cnt */
    0005727,                    /*   BLF,BLF            ; move to lower */
    0007000,                    /*   CMB                ; make neg */
    0077775,                    /*   STA WC             ; save */
    0102211,                    /*   SFC CC             ; read done? */
    0027752,                    /*   JMP STAT           ; no, get stat */
    0102310,                    /*   SFS DC             ; any data? */
    0027727,                    /*   JMP *-3            ; wait */
    0107510,                    /*   LIB DC,C           ; get load addr */
    0074000,                    /*   STB 0              ; start csum */
    0077762,                    /*   STA CMD            ; save address */
    0027742,                    /*   JMP *+4 */
    0177762,                    /*NW STB CMD,I          ; store data */
    0040001,                    /*   ADA 1              ; add to csum */
    0037762,                    /*   ISZ CMD            ; adv addr ptr */
    0102310,                    /*   SFS DC             ; any data? */
    0027742,                    /*   JMP *-1            ; wait */
    0107510,                    /*   LIB DC,C           ; get word */
    0037775,                    /*   ISZ WC             ; done? */
    0027737,                    /*   JMP NW             ; no */
    0054000,                    /*   CPB 0              ; csum ok? */
    0027717,                    /*   JMP RD+3           ; yes, cont */
    0102011,                    /*   HLT 11             ; no, halt */
    0102511,                    /*ST LIA CC             ; get status */
    0001727,                    /*   ALF,ALF            ; get eof bit */
    0002020,                    /*   SSA                ; set? */
    0102077,                    /*   HLT 77             ; done */
    0001727,                    /*   ALF,ALF            ; put status back */
    0001310,                    /*   RAR,SLA            ; read ok? */
    0102000,                    /*   HLT 0              ; no */
    0027714,                    /*   JMP RD             ; read next */
    0000000,                    /*CMD 0 */
    0106611,                    /*   OTB CC             ; output cmd */
    0102511,                    /*   LIA CC             ; check for reject */
    0001323,                    /*   RAR,RAR */
    0001310,                    /*   RAR,SLA */
    0027763,                    /*   JMP CMD+1          ; try again */
    0103711,                    /*   STC CC,C           ; start command */
    0127762,                    /*   JMP CMD,I          ; exit */
    0001501,                    /*SL0RW 001501          ; select 0, rewind */
    0001423,                    /*RDCMD 001423          ; read record */
    0000203,                    /*FFC   000203          ; space forward file */
    0000000,                    /*WC    000000 */
    0000000,
    0000000
    };

t_stat msc_boot (int32 unitno, DEVICE *dptr)
{
int32 dev;
extern uint32 saved_AR;

if (unitno != 0) return SCPE_NOFNC;                     /* only unit 0 */
dev = msd_dib.devno;                                    /* get data chan dev */
if (ibl_copy (ms_rom, dev)) return SCPE_IERR;           /* copy boot to memory */
SR = (SR & IBL_OPT) | IBL_MS | (dev << IBL_V_DEV);      /* set SR */
if ((sim_switches & SWMASK ('S')) && saved_AR) SR = SR | 1;     /* skip? */
return SCPE_OK;
}

