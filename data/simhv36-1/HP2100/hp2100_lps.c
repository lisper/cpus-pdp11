/* hp2100_lps.c: HP 2100 12653A line printer simulator

   Copyright (c) 1993-2005, Robert M. Supnik

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

   lps          12653A 2767 line printer
                (based on 12566B microcircuit interface)

   19-Nov-04    JDB     Added restart when set online, etc.
                        Fixed col count for non-printing chars
   01-Oct-04    JDB     Added SET OFFLINE/ONLINE, POWEROFF/POWERON
                        Fixed status returns for error conditions
                        Fixed handling of non-printing characters
                        Fixed handling of characters after column 80
                        Improved timing model accuracy for RTE
                        Added fast/realistic timing
                        Added debug printouts
   03-Jun-04    RMS     Fixed timing (found by Dave Bryan)
   26-Apr-04    RMS     Fixed SFS x,C and SFC x,C
                        Implemented DMA SRQ (follows FLG)
   25-Apr-03    RMS     Revised for extended file support
   24-Oct-02    RMS     Added microcircuit test features
   30-May-02    RMS     Widened POS to 32b
   03-Dec-01    RMS     Changed DEVNO to use extended SET/SHOW
   07-Sep-01    RMS     Moved function prototypes
   21-Nov-00    RMS     Fixed flag, fbf power up state
                        Added command flop
   15-Oct-00    RMS     Added variable device number support

   The 2767 impact printer has a rotating drum with 80 columns of 64 raised
   characters.  ASCII codes 32 through 95 (SPACE through "_") form the print
   repertoire.  The printer responds to the control characters FF, LF, and CR.

   The 80 columns are divided into four zones of 20 characters each that are
   addressed sequentially.  Received characters are buffered in a 20-character
   memory.  When the 20th printable character is received, the current zone is
   printed, and the memory is reset.  In the absence of print command
   characters, a zone print operation will commence after each group of 20
   printable characters is transmitted to the printer.

   The print command characters have these actions:

    * CR -- print the characters in the current zone, reset to zone 1, and clear
            the buffer memory.
    * LF -- same as CR, plus advances the paper one line.
    * FF -- same as CR, plus advances the paper to the top of the next form.
   
   The 2767 provides two status bits via the interface:

     bit 15 -- printer not ready
     bit  0 -- printer busy

   The expected status returns are:

     100001 -- power off or cable disconnected
     100001 -- initial power on, then changes to 000001 within sixty
               seconds of initial power on
     000001 -- power on, paper unloaded or printer offline or not idle
     000000 -- power on, paper loaded and printer online and idle

   These simulator commands provide the listed printer states:

     SET LPS POWEROFF --> power off or cable disconnected
     SET LPS POWERON  --> power on
     SET LPS OFFLINE  --> printer offline
     SET LPS ONLINE   --> printer online
     ATT LPS <file>   --> paper loaded
     DET LPS          --> paper out

   References:
   - 2767A Line Printer Operating and Service Manual (02767-90002, Oct-1973)
   - 12566B, 12566B-001, 12566B-002, 12566B-003 Microcircuit Interface Kits
     Operating and Service Manual (12566-90015, Apr-1976)

   The following implemented behaviors have been inferred from secondary sources
   (diagnostics, operating system drivers, etc.), due to absent or contradictory
   authoritative information; future correction may be needed:

     1. Paper out sets BUSY instead of NOT READY.
     2. Print operation in progress sets BUSY instead of NOT READY.
     3. Characters not in the print repertoire are replaced with blanks.
     4. The 81st and succeeding characters overprint the current line.
*/

#include "hp2100_defs.h"

#define LPS_ZONECNT     20                              /* zone char count */
#define LPS_PAGECNT     80                              /* page char count */
#define LPS_PAGELNT     60                              /* page line length */
#define LPS_FORMLNT     66                              /* form line length */

/* Printer power states */

#define LPS_ON          0                               /* power is on */
#define LPS_OFF         1                               /* power is off */
#define LPS_TURNING_ON  2                               /* power is turning on */

#define LPS_BUSY        0000001                         /* busy status */
#define LPS_NRDY        0100000                         /* not ready status */
#define LPS_PWROFF      LPS_BUSY | LPS_NRDY             /* power-off status */

#define UNIT_V_DIAG     (UNIT_V_UF + 0)                 /* diagnostic mode */
#define UNIT_V_POWEROFF (UNIT_V_UF + 1)                 /* unit powered off */
#define UNIT_V_OFFLINE  (UNIT_V_UF + 2)                 /* unit offline */
#define UNIT_DIAG       (1 << UNIT_V_DIAG)
#define UNIT_POWEROFF   (1 << UNIT_V_POWEROFF)
#define UNIT_OFFLINE    (1 << UNIT_V_OFFLINE)

extern uint32 PC;
extern uint32 dev_cmd[2], dev_ctl[2], dev_flg[2], dev_fbf[2], dev_srq[2];
extern FILE *sim_deb;

int32 lps_ccnt = 0;                                     /* character count */
int32 lps_lcnt = 0;                                     /* line count */
int32 lps_stopioe = 0;                                  /* stop on error */
int32 lps_sta = 0;                                      /* printer status */
int32 lps_timing = 1;                                   /* timing type */
uint32 lps_power = LPS_ON;                              /* power state */

/* Hardware timing:
   (based on 1580 instr/msec)             instr   msec   calc msec
                                          ------------------------
   - character transfer time   : ctime =      2    2 us
   - per-zone printing time    : ptime =  55300   35        40
   - per-line paper slew time  : stime =  17380   11        13
   - power-on ready delay time : rtime = 158000  100

  NOTE: the printer acknowledges before the print motion has stopped to allow
        for continuous slew, so the set times are a bit less than the calculated
        operation time from the manual.
*/

int32 lps_ctime = 0;                                    /* char xfer time */
int32 lps_ptime = 0;                                    /* zone printing time */
int32 lps_stime = 0;                                    /* paper slew time */
int32 lps_rtime = 0;                                    /* power-on ready time */

typedef int32 TIMESET[4];                               /* set of controller times */

int32 *const lps_timers[] = { &lps_ctime, &lps_ptime, &lps_stime, &lps_rtime };

const TIMESET lps_times[2] = {
    { 2, 55300, 17380, 158000 },                        /* REALTIME */
    { 2,  1000,   1000,  1000 }                         /* FASTTIME */
    };

DEVICE lps_dev;
int32 lpsio (int32 inst, int32 IR, int32 dat);
t_stat lps_svc (UNIT *uptr);
t_stat lps_reset (DEVICE *dptr);
t_stat lps_restart (UNIT *uptr, int32 value, char *cptr, void *desc);
t_stat lps_poweroff (UNIT *uptr, int32 value, char *cptr, void *desc);
t_stat lps_poweron (UNIT *uptr, int32 value, char *cptr, void *desc);
t_stat lps_attach (UNIT *uptr, char *cptr);
t_stat lps_set_timing (UNIT *uptr, int32 val, char *cptr, void *desc);
t_stat lps_show_timing (FILE *st, UNIT *uptr, int32 val, void *desc);

/* LPS data structures

   lps_dev      LPS device descriptor
   lps_unit     LPS unit descriptor
   lps_reg      LPS register list
*/

DIB lps_dib = { LPS, 0, 0, 0, 0, 0, &lpsio };

UNIT lps_unit = {
    UDATA (&lps_svc, UNIT_SEQ+UNIT_ATTABLE+UNIT_DISABLE, 0)
    };

REG lps_reg[] = {
    { ORDATA (BUF, lps_unit.buf, 16) },
    { ORDATA (STA, lps_sta, 16) },
    { ORDATA (POWER, lps_power, 2), REG_RO },
    { FLDATA (CMD, lps_dib.cmd, 0) },
    { FLDATA (CTL, lps_dib.ctl, 0) },
    { FLDATA (FLG, lps_dib.flg, 0) },
    { FLDATA (FBF, lps_dib.fbf, 0) },
    { FLDATA (SRQ, lps_dib.srq, 0) },
    { DRDATA (CCNT, lps_ccnt, 7), PV_LEFT },
    { DRDATA (LCNT, lps_lcnt, 7), PV_LEFT },
    { DRDATA (POS, lps_unit.pos, T_ADDR_W), PV_LEFT },
    { DRDATA (CTIME, lps_ctime, 24), PV_LEFT },
    { DRDATA (PTIME, lps_ptime, 24), PV_LEFT },
    { DRDATA (STIME, lps_stime, 24), PV_LEFT },
    { DRDATA (RTIME, lps_rtime, 24), PV_LEFT },
    { FLDATA (TIMING, lps_timing, 0), REG_HRO },
    { FLDATA (STOP_IOE, lps_stopioe, 0) },
    { ORDATA (DEVNO, lps_dib.devno, 6), REG_HRO },
    { NULL }
    };

MTAB lps_mod[] = {
    { UNIT_DIAG, UNIT_DIAG, "diagnostic mode", "DIAG", NULL },
    { UNIT_DIAG, 0, "printer mode", "PRINTER", NULL },
    { UNIT_POWEROFF, UNIT_POWEROFF, "power off", "POWEROFF", lps_poweroff },
    { UNIT_POWEROFF, 0, "power on", "POWERON", lps_poweron },
    { UNIT_OFFLINE, UNIT_OFFLINE, "offline", "OFFLINE", NULL },
    { UNIT_OFFLINE, 0, "online", "ONLINE", lps_restart },
    { MTAB_XTD | MTAB_VDV, 0, NULL, "REALTIME",
      &lps_set_timing, NULL, NULL },
    { MTAB_XTD | MTAB_VDV, 1, NULL, "FASTTIME",
      &lps_set_timing, NULL, NULL },
    { MTAB_XTD | MTAB_VDV, 0, "TIMING", NULL,
      NULL, &lps_show_timing, NULL },
    { MTAB_XTD | MTAB_VDV, 0, "DEVNO", "DEVNO",
      &hp_setdev, &hp_showdev, &lps_dev },
    { 0 }
    };

DEVICE lps_dev = {
    "LPS", &lps_unit, lps_reg, lps_mod,
    1, 10, 31, 1, 8, 8,
    NULL, NULL, &lps_reset,
    NULL, &lps_attach, NULL,
    &lps_dib, DEV_DISABLE | DEV_DIS | DEV_DEBUG
    };

/* IO instructions */

int32 lpsio (int32 inst, int32 IR, int32 dat)
{
int32 dev, sched;

dev = IR & I_DEVMASK;                                   /* get device no */
switch (inst) {                                         /* case on opcode */

    case ioFLG:                                         /* flag clear/set */
        if ((IR & I_HC) == 0) { setFSR (dev); }         /* STF */
        break;

    case ioSFC:                                         /* skip flag clear */
        if (FLG (dev) == 0) PC = (PC + 1) & VAMASK;
        break;

    case ioSFS:                                         /* skip flag set */
        if (FLG (dev) != 0) PC = (PC + 1) & VAMASK;
        break;

    case ioOTX:                                         /* output */
        if (DEBUG_PRS (lps_dev))
            fprintf (sim_deb, ">>LPS OTx: Character %06o output\n", dat);
        lps_unit.buf = dat;
        break;

    case ioLIX:                                         /* load */
        dat = 0;                                        /* default sta = 0 */
    case ioMIX:                                         /* merge */
        if ((lps_unit.flags & UNIT_DIAG) == 0) {        /* real lpt? */
            if (lps_power == LPS_ON) {                  /* power on? */
                if (((lps_unit.flags & UNIT_ATT) == 0) ||   /* paper out? */
                    (lps_unit.flags & UNIT_OFFLINE) ||      /* offline? */
                    sim_is_active (&lps_unit)) lps_sta = LPS_BUSY;
                else lps_sta = 0;
                }
            else lps_sta = LPS_PWROFF;
            }
        dat = dat | lps_sta;                            /* diag, rtn status */
        if (DEBUG_PRS (lps_dev))
            fprintf (sim_deb, ">>LPS LIx: Status %06o returned\n", dat);
        break;

    case ioCTL:                                         /* control clear/set */
        if (IR & I_CTL) {                               /* CLC */
            clrCMD (dev);                               /* clear ctl, cmd */
            clrCTL (dev);
            }
        else {                                          /* STC */
            setCMD (dev);                               /* set ctl, cmd */
            setCTL (dev);
            if (lps_unit.flags & UNIT_DIAG)             /* diagnostic? */
                sim_activate (&lps_unit, 1);            /* loop back */
            else {                                      /* real lpt, sched */
                if (DEBUG_PRS (lps_dev)) fprintf (sim_deb,
                    ">>LPS STC: Character %06o scheduled for line %d, column %d, ",
                    lps_unit.buf, lps_lcnt + 1, lps_ccnt + 1);
                if ((lps_unit.buf != '\f') &&
                    (lps_unit.buf != '\n') &&
                    (lps_unit.buf != '\r')) {           /* normal char */
                    lps_ccnt = lps_ccnt + 1;            /* incr char counter */
                    if (lps_ccnt % LPS_ZONECNT == 0)    /* end of zone? */
                        sched = lps_ptime;              /* print zone */
                    else sched = lps_ctime;             /* xfer char */
                    }
                else {                                  /* print cmd */
                    if (lps_ccnt % LPS_ZONECNT == 0)    /* last zone printed? */
                        sched = lps_ctime;              /* yes, so just char time */
                    else sched = lps_ptime;             /* no, so print needed */
                    lps_ccnt = 0;                       /* reset char counter */
                    if (lps_unit.buf == '\n') {         /* line advance */
                        lps_lcnt = (lps_lcnt + 1) % LPS_PAGELNT;
                        if (lps_lcnt > 0) sched = sched + lps_stime;
                        else sched = sched +            /* allow for perf skip */
                            lps_stime * (LPS_FORMLNT - LPS_PAGELNT);
                        }
                    else if (lps_unit.buf == '\f') {    /* form advance */
                        sched = sched + lps_stime * (LPS_FORMLNT - lps_lcnt);
                        lps_lcnt = 0;
                        }
                    }
                sim_activate (&lps_unit, sched);
                if (DEBUG_PRS (lps_dev))
                    fprintf (sim_deb, "time = %d\n", sched);
                }
            }
        break;

    default:
        break;
        }

if (IR & I_HC) { clrFSR (dev); }                        /* H/C option */
return dat;
}

/* Unit service */

t_stat lps_svc (UNIT *uptr)
{
int32 dev;
int32 c = uptr->buf & 0177;

if (lps_power == LPS_TURNING_ON) {                      /* printer warmed up? */
    lps_power = LPS_ON;                                 /* change state */
    lps_restart (uptr, 0, NULL, NULL);                  /* restart I/O if hung */
    if (DEBUG_PRS (lps_dev))
        fputs (">>LPS svc: Power state is ON\n", sim_deb);
    return SCPE_OK;                                     /* done */
    }
dev = lps_dib.devno;                                    /* get dev no */
if (uptr->flags & UNIT_DIAG) {                          /* diagnostic? */
    lps_sta = uptr->buf;                                /* loop back */
    clrCMD (dev);                                       /* clear cmd */
    setFSR (dev);                                       /* set flag, fbf */
    return SCPE_OK;                                     /* done */
    }
if ((uptr->flags & UNIT_ATT) == 0)                      /* attached? */
    return IORETURN (lps_stopioe, SCPE_UNATT);
else if (uptr->flags & UNIT_OFFLINE)                    /* offline? */
    return IORETURN (lps_stopioe, STOP_OFFLINE);
else if (uptr->flags & UNIT_POWEROFF)                   /* powered off? */
    return IORETURN (lps_stopioe, STOP_PWROFF);
clrCMD (dev);                                           /* clear cmd */
setFSR (dev);                                           /* set flag, fbf */
if (((c < ' ') || (c > '_')) &&                         /* non-printing char? */
    (c != '\f') && (c != '\n') && (c != '\r')) {
        if (DEBUG_PRS (lps_dev))
            fprintf (sim_deb, ">>LPS svc: Character %06o erased\n", c);
        c = ' ';                                        /* replace with blank */
        }
if (lps_ccnt > LPS_PAGECNT) {                           /* 81st character? */
    fputc ('\r', uptr->fileref);                        /* return to line start */
    uptr->pos = uptr->pos + 1;                          /* update pos */
    lps_ccnt = 1;                                       /* reset char counter */
    if (DEBUG_PRS (lps_dev))
        fputs (">>LPS svc: Line wraparound to column 1\n", sim_deb);
    }
fputc (c, uptr->fileref);                               /* "print" char */
uptr->pos = uptr->pos + 1;                              /* update pos */
if (DEBUG_PRS (lps_dev))
    fprintf (sim_deb, ">>LPS svc: Character %06o printed\n", c);
if ((lps_lcnt == 0) && (c == '\n')) {                   /* LF did TOF? */
    fputc ('\f', uptr->fileref);                        /* do perf skip */
    uptr->pos = uptr->pos + 1;                          /* update pos */
    if (DEBUG_PRS (lps_dev))
        fputs (">>LPS svc: Perforation skip to TOF\n", sim_deb);
    }
if (ferror (uptr->fileref)) {
    perror ("LPS I/O error");
    clearerr (uptr->fileref);
    return SCPE_IOERR;
    }
return SCPE_OK;
}

/* Reset routine - called from SCP, flags in DIB */

t_stat lps_reset (DEVICE *dptr)
{
lps_dib.cmd = lps_dib.ctl = 0;                          /* clear cmd, ctl */
lps_dib.flg = lps_dib.fbf = lps_dib.srq = 1;            /* set flg, fbf, srq */
lps_sta = lps_unit.buf = 0;
lps_power = LPS_ON;                                     /* power is on */
sim_cancel (&lps_unit);                                 /* deactivate unit */
lps_set_timing (NULL, lps_timing, NULL, NULL);          /* init timing set */
return SCPE_OK;
}

/* Restart I/O routine

   If I/O is started via STC, and the printer is powered off, offline,
   or out of paper, the CTL and CMD flip-flops will set, a service event
   will be scheduled, and the service routine will be entered.  If
   STOP_IOE is not set, the I/O operation will "hang" at that point
   until the printer is powered on, set online, or paper is supplied
   (attached).

   If a pending operation is "hung" when this routine is called, it is
   restarted, which clears CTL and sets FBF and FLG, completing the
   original I/O request.
 */

t_stat lps_restart (UNIT *uptr, int32 value, char *cptr, void *desc)
{
if (lps_dib.cmd && lps_dib.ctl && !sim_is_active (uptr))
    sim_activate (uptr, 0);                             /* reschedule I/O */
return SCPE_OK;
}

/* Printer power off */

t_stat lps_poweroff (UNIT *uptr, int32 value, char *cptr, void *desc)
{
lps_power = LPS_OFF;                                    /* change state */
if (DEBUG_PRS (lps_dev)) fputs (">>LPS set: Power state is OFF\n", sim_deb);
return SCPE_OK;
}

/* Printer power on */

t_stat lps_poweron (UNIT *uptr, int32 value, char *cptr, void *desc)
{
if (lps_unit.flags & UNIT_DIAG) {                       /* diag mode? */
    lps_power = LPS_ON;                                 /* no delay */
    if (DEBUG_PRS (lps_dev))
        fputs (">>LPS set: Power state is ON\n", sim_deb);
    }
else {
    lps_power = LPS_TURNING_ON;                         /* change state */
    lps_unit.flags |= UNIT_OFFLINE;                     /* set offline */
    sim_activate (&lps_unit, lps_rtime);                /* schedule ready */
    if (DEBUG_PRS (lps_dev)) fprintf (sim_deb,
        ">>LPS set: Power state is TURNING ON, scheduled time = %d\n",
        lps_rtime );
    }
return SCPE_OK;
}

/* Attach routine */

t_stat lps_attach (UNIT *uptr, char *cptr)
{
lps_ccnt = lps_lcnt = 0;                                /* top of form */
lps_restart (uptr, 0, NULL, NULL);                      /* restart I/O if hung */
return attach_unit (uptr, cptr);
}

/* Set printer timing */

t_stat lps_set_timing (UNIT *uptr, int32 val, char *cptr, void *desc)
{
int32 i;

lps_timing = (val != 0);                                /* determine choice */
for (i = 0; i < (sizeof (lps_timers) / sizeof (lps_timers[0])); i++)
    *lps_timers[i] = lps_times[lps_timing][i];          /* assign times */
return SCPE_OK;
}

/* Show printer timing */

t_stat lps_show_timing (FILE *st, UNIT *uptr, int32 val, void *desc)
{
if (lps_timing) fputs ("fast timing", st);
else fputs ("realistic timing", st);
return SCPE_OK;
}
