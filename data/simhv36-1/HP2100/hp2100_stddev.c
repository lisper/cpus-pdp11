/* hp2100_stddev.c: HP2100 standard devices simulator

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

   ptr          12597A-002 paper tape reader interface
   ptp          12597A-005 paper tape punch interface
   tty          12531C buffered teleprinter interface
   clk          12539C time base generator

   22-Nov-05    RMS     Revised for new terminal processing routines
   13-Sep-04    JDB     Added paper tape loop mode, DIAG/READER modifiers to PTR
                        Added PV_LEFT to PTR TRLLIM register
                        Modified CLK to permit disable
   15-Aug-04    RMS     Added tab to control char set (from Dave Bryan)
   14-Jul-04    RMS     Generalized handling of control char echoing
                        (from Dave Bryan)
   26-Apr-04    RMS     Fixed SFS x,C and SFC x,C
                        Fixed SR setting in IBL
                        Fixed input behavior during typeout for RTE-IV
                        Suppressed nulls on TTY output for RTE-IV
                        Implemented DMA SRQ (follows FLG)
   29-Mar-03    RMS     Added support for console backpressure
   25-Apr-03    RMS     Added extended file support
   22-Dec-02    RMS     Added break support
   01-Nov-02    RMS     Revised BOOT command for IBL ROMs
                        Fixed bug in TTY reset, TTY starts in input mode
                        Fixed bug in TTY mode OTA, stores data as well
                        Fixed clock to add calibration, proper start/stop
                        Added UC option to TTY output
   30-May-02    RMS     Widened POS to 32b
   22-Mar-02    RMS     Revised for dynamically allocated memory
   03-Nov-01    RMS     Changed DEVNO to use extended SET/SHOW
   29-Nov-01    RMS     Added read only unit support
   24-Nov-01    RMS     Changed TIME to an array
   07-Sep-01    RMS     Moved function prototypes
   21-Nov-00    RMS     Fixed flag, buffer power up state
                        Added status input for ptp, tty
   15-Oct-00    RMS     Added dynamic device number support

   The reader and punch, like most HP devices, have a command flop.  The
   teleprinter and clock do not.

   Reader diagnostic mode simulates a tape loop by rewinding the tape image file
   upon EOF.  Normal mode EOF action is to supply TRLLIM nulls and then either
   return SCPE_IOERR or SCPE_OK without setting the device flag.

   The clock autocalibrates.  If the specified clock frequency is below
   10Hz, the clock service routine runs at 10Hz and counts down a repeat
   counter before generating an interrupt.  Autocalibration will not work
   if the clock is running at 1Hz or less.

   Clock diagnostic mode corresponds to inserting jumper W2 on the 12539C.
   This turns off autocalibration and divides the longest time intervals down
   by 10**3.  The clk_time values were chosen to allow the diagnostic to
   pass its clock calibration test.

   References:
   - 2748B Tape Reader Operating and Service Manual (02748-90041, Oct-1977)
   - 12597A 8-Bit Duplex Register Interface Kit Operating and Service Manual
            (12597-9002, Sep-1974)
   - 12539C Time Base Generator Interface Kit Operating and Service Manual
            (12539-90008, Jan-1975)
*/

#include "hp2100_defs.h"
#include <ctype.h>

#define CHAR_FLAG(c)    (1u << (c))

#define BEL_FLAG        CHAR_FLAG('\a')
#define BS_FLAG         CHAR_FLAG('\b')
#define LF_FLAG         CHAR_FLAG('\n')
#define CR_FLAG         CHAR_FLAG('\r')
#define HT_FLAG         CHAR_FLAG('\t')

#define FULL_SET        0xFFFFFFFF
#define CNTL_SET        (BEL_FLAG | BS_FLAG | HT_FLAG | LF_FLAG | CR_FLAG)

#define UNIT_V_DIAG     (TTUF_V_UF + 0)                 /* diag mode */
#define UNIT_V_AUTOLF   (TTUF_V_UF + 1)                 /* auto linefeed */
#define UNIT_DIAG       (1 << UNIT_V_DIAG)
#define UNIT_AUTOLF     (1 << UNIT_V_AUTOLF)

#define PTP_LOW         0000040                         /* low tape */
#define TM_MODE         0100000                         /* mode change */
#define TM_KBD          0040000                         /* enable keyboard */
#define TM_PRI          0020000                         /* enable printer */
#define TM_PUN          0010000                         /* enable punch */
#define TP_BUSY         0100000                         /* busy */

#define CLK_V_ERROR     4                               /* clock overrun */
#define CLK_ERROR       (1 << CLK_V_ERROR)

extern uint32 PC, SR;
extern uint32 dev_cmd[2], dev_ctl[2], dev_flg[2], dev_fbf[2], dev_srq[2];

int32 ptr_stopioe = 0;                                  /* stop on error */
int32 ptr_trlcnt = 0;                                   /* trailer counter */
int32 ptr_trllim = 40;                                  /* trailer to add */
int32 ptp_stopioe = 0;
int32 ttp_stopioe = 0;
int32 tty_buf = 0;                                      /* tty buffer */
int32 tty_mode = 0;                                     /* tty mode */
int32 tty_shin = 0377;                                  /* tty shift in */
int32 tty_lf = 0;                                       /* lf flag */
uint32 tty_cntlprt = CNTL_SET;                          /* tty print flags */
uint32 tty_cntlset = CNTL_SET;                          /* tty cntl set */
int32 clk_select = 0;                                   /* clock time select */
int32 clk_error = 0;                                    /* clock error */
int32 clk_ctr = 0;                                      /* clock counter */
int32 clk_time[8] = {                                   /* clock intervals */
    155, 1550, 15500, 155000, 155000, 155000, 155000, 155000
    };
int32 clk_tps[8] = {                                    /* clock tps */
    10000, 1000, 100, 10, 10, 10, 10, 10
    };
int32 clk_rpt[8] = {                                    /* number of repeats */
    1, 1, 1, 1, 10, 100, 1000, 10000
    };

DEVICE ptr_dev, ptp_dev, tty_dev, clk_dev;
int32 ptrio (int32 inst, int32 IR, int32 dat);
t_stat ptr_svc (UNIT *uptr);
t_stat ptr_attach (UNIT *uptr, char *cptr);
t_stat ptr_reset (DEVICE *dptr);
t_stat ptr_boot (int32 unitno, DEVICE *dptr);
int32 ptpio (int32 inst, int32 IR, int32 dat);
t_stat ptp_svc (UNIT *uptr);
t_stat ptp_reset (DEVICE *dptr);
int32 ttyio (int32 inst, int32 IR, int32 dat);
t_stat tti_svc (UNIT *uptr);
t_stat tto_svc (UNIT *uptr);
t_stat tty_reset (DEVICE *dptr);
t_stat tty_set_opt (UNIT *uptr, int32 val, char *cptr, void *desc);
t_stat tty_set_alf (UNIT *uptr, int32 val, char *cptr, void *desc);
int32 clkio (int32 inst, int32 IR, int32 dat);
t_stat clk_svc (UNIT *uptr);
t_stat clk_reset (DEVICE *dptr);
int32 clk_delay (int32 flg);
t_stat tto_out (int32 c);
t_stat ttp_out (int32 c);

/* PTR data structures

   ptr_dev      PTR device descriptor
   ptr_unit     PTR unit descriptor
   ptr_mod      PTR modifiers
   ptr_reg      PTR register list
*/

DIB ptr_dib = { PTR, 0, 0, 0, 0, 0, &ptrio };

UNIT ptr_unit = {
    UDATA (&ptr_svc, UNIT_SEQ+UNIT_ATTABLE+UNIT_ROABLE, 0),
           SERIAL_IN_WAIT
    };

REG ptr_reg[] = {
    { ORDATA (BUF, ptr_unit.buf, 8) },
    { FLDATA (CMD, ptr_dib.cmd, 0) },
    { FLDATA (CTL, ptr_dib.ctl, 0) },
    { FLDATA (FLG, ptr_dib.flg, 0) },
    { FLDATA (FBF, ptr_dib.fbf, 0) },
    { FLDATA (SRQ, ptr_dib.srq, 0) },
    { DRDATA (TRLCTR, ptr_trlcnt, 8), REG_HRO },
    { DRDATA (TRLLIM, ptr_trllim, 8), PV_LEFT },
    { DRDATA (POS, ptr_unit.pos, T_ADDR_W), PV_LEFT },
    { DRDATA (TIME, ptr_unit.wait, 24), PV_LEFT },
    { FLDATA (STOP_IOE, ptr_stopioe, 0) },
    { ORDATA (DEVNO, ptr_dib.devno, 6), REG_HRO },
    { NULL }
    };

MTAB ptr_mod[] = {
    { UNIT_DIAG, UNIT_DIAG, "diagnostic mode", "DIAG", NULL },
    { UNIT_DIAG, 0, "reader mode", "READER", NULL },
    { MTAB_XTD | MTAB_VDV, 0, "DEVNO", "DEVNO",
      &hp_setdev, &hp_showdev, &ptr_dev },
    { 0 }
    };

DEVICE ptr_dev = {
    "PTR", &ptr_unit, ptr_reg, ptr_mod,
    1, 10, 31, 1, 8, 8,
    NULL, NULL, &ptr_reset,
    &ptr_boot, &ptr_attach, NULL,
    &ptr_dib, DEV_DISABLE
    };

/* PTP data structures

   ptp_dev      PTP device descriptor
   ptp_unit     PTP unit descriptor
   ptp_mod      PTP modifiers
   ptp_reg      PTP register list
*/

DIB ptp_dib = { PTP, 0, 0, 0, 0, 0, &ptpio };

UNIT ptp_unit = {
    UDATA (&ptp_svc, UNIT_SEQ+UNIT_ATTABLE, 0), SERIAL_OUT_WAIT
    };

REG ptp_reg[] = {
    { ORDATA (BUF, ptp_unit.buf, 8) },
    { FLDATA (CMD, ptp_dib.cmd, 0) },
    { FLDATA (CTL, ptp_dib.ctl, 0) },
    { FLDATA (FLG, ptp_dib.flg, 0) },
    { FLDATA (FBF, ptp_dib.fbf, 0) },
    { FLDATA (SRQ, ptp_dib.srq, 0) },
    { DRDATA (POS, ptp_unit.pos, T_ADDR_W), PV_LEFT },
    { DRDATA (TIME, ptp_unit.wait, 24), PV_LEFT },
    { FLDATA (STOP_IOE, ptp_stopioe, 0) },
    { ORDATA (DEVNO, ptp_dib.devno, 6), REG_HRO },
    { NULL }
    };

MTAB ptp_mod[] = {
    { MTAB_XTD | MTAB_VDV, 0, "DEVNO", "DEVNO",
      &hp_setdev, &hp_showdev, &ptp_dev },
    { 0 }
    };

DEVICE ptp_dev = {
    "PTP", &ptp_unit, ptp_reg, ptp_mod,
    1, 10, 31, 1, 8, 8,
    NULL, NULL, &ptp_reset,
    NULL, NULL, NULL,
    &ptp_dib, DEV_DISABLE
    };

/* TTY data structures

   tty_dev      TTY device descriptor
   tty_unit     TTY unit descriptor
   tty_reg      TTY register list
   tty_mod      TTy modifiers list
*/

#define TTI     0
#define TTO     1
#define TTP     2

DIB tty_dib = { TTY, 0, 0, 0, 0, 0, &ttyio };

UNIT tty_unit[] = {
    { UDATA (&tti_svc, TT_MODE_UC, 0), KBD_POLL_WAIT },
    { UDATA (&tto_svc, TT_MODE_UC, 0), SERIAL_OUT_WAIT },
    { UDATA (&tto_svc, UNIT_SEQ+UNIT_ATTABLE+TT_MODE_8B, 0), SERIAL_OUT_WAIT }
    };

REG tty_reg[] = {
    { ORDATA (BUF, tty_buf, 8) },
    { ORDATA (MODE, tty_mode, 16) },
    { ORDATA (SHIN, tty_shin, 8), REG_HRO },
    { FLDATA (CMD, tty_dib.cmd, 0), REG_HRO },
    { FLDATA (CTL, tty_dib.ctl, 0) },
    { FLDATA (FLG, tty_dib.flg, 0) },
    { FLDATA (FBF, tty_dib.fbf, 0) },
    { FLDATA (SRQ, tty_dib.srq, 0) },
    { FLDATA (KLFP, tty_lf, 0), REG_HRO },
    { DRDATA (KPOS, tty_unit[TTI].pos, T_ADDR_W), PV_LEFT },
    { DRDATA (KTIME, tty_unit[TTI].wait, 24), REG_NZ + PV_LEFT },
    { DRDATA (TPOS, tty_unit[TTO].pos, T_ADDR_W), PV_LEFT },
    { DRDATA (TTIME, tty_unit[TTO].wait, 24), REG_NZ + PV_LEFT },
    { DRDATA (PPOS, tty_unit[TTP].pos, T_ADDR_W), PV_LEFT },
    { FLDATA (STOP_IOE, ttp_stopioe, 0) },
    { ORDATA (CNTLPRT, tty_cntlprt, 32), REG_HRO },
    { ORDATA (CNTLSET, tty_cntlset, 32), REG_HIDDEN },
    { ORDATA (DEVNO, tty_dib.devno, 6), REG_HRO },
    { NULL }
    };

MTAB tty_mod[] = {
    { TT_MODE, TT_MODE_UC, "UC", "UC", &tty_set_opt },
    { TT_MODE, TT_MODE_7B, "7b", "7B", &tty_set_opt },
    { TT_MODE, TT_MODE_8B, "8b", "8B", &tty_set_opt },
    { TT_MODE, TT_MODE_7P, "7p", "7P", &tty_set_opt },
    { UNIT_AUTOLF, UNIT_AUTOLF, "autolf", "AUTOLF", &tty_set_alf },
    { UNIT_AUTOLF, 0          , NULL, "NOAUTOLF", &tty_set_alf },
    { MTAB_XTD | MTAB_VDV, 0, "DEVNO", "DEVNO",
      &hp_setdev, &hp_showdev, &tty_dev },
    { 0 }
    };

DEVICE tty_dev = {
    "TTY", tty_unit, tty_reg, tty_mod,
    3, 10, 31, 1, 8, 8,
    NULL, NULL, &tty_reset,
    NULL, NULL, NULL,
    &tty_dib, 0
    };

/* CLK data structures

   clk_dev      CLK device descriptor
   clk_unit     CLK unit descriptor
   clk_mod      CLK modifiers
   clk_reg      CLK register list
*/

DIB clk_dib = { CLK, 0, 0, 0, 0, 0, &clkio };

UNIT clk_unit = { UDATA (&clk_svc, 0, 0) };

REG clk_reg[] = {
    { ORDATA (SEL, clk_select, 3) },
    { DRDATA (CTR, clk_ctr, 14) },
    { FLDATA (CMD, clk_dib.cmd, 0), REG_HRO },
    { FLDATA (CTL, clk_dib.ctl, 0) },
    { FLDATA (FLG, clk_dib.flg, 0) },
    { FLDATA (FBF, clk_dib.fbf, 0) },
    { FLDATA (SRQ, clk_dib.srq, 0) },
    { FLDATA (ERR, clk_error, CLK_V_ERROR) },
    { BRDATA (TIME, clk_time, 10, 24, 8) },
    { ORDATA (DEVNO, clk_dib.devno, 6), REG_HRO },
    { NULL }
    };

MTAB clk_mod[] = {
    { UNIT_DIAG, UNIT_DIAG, "diagnostic mode", "DIAG", NULL },
    { UNIT_DIAG, 0, "calibrated", "CALIBRATED", NULL },
    { MTAB_XTD | MTAB_VDV, 0, "DEVNO", "DEVNO",
      &hp_setdev, &hp_showdev, &clk_dev },
    { 0 }
    };

DEVICE clk_dev = {
    "CLK", &clk_unit, clk_reg, clk_mod,
    1, 0, 0, 0, 0, 0,
    NULL, NULL, &clk_reset,
    NULL, NULL, NULL,
    &clk_dib, DEV_DISABLE
    };

/* Paper tape reader IO instructions */

int32 ptrio (int32 inst, int32 IR, int32 dat)
{
int32 dev;

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

    case ioMIX:                                         /* merge */
        dat = dat | ptr_unit.buf;
        break;

    case ioLIX:                                         /* load */
        dat = ptr_unit.buf;
        break;

    case ioCTL:                                         /* control clear/set */
        if (IR & I_CTL) {                               /* CLC */
            clrCMD (dev);                               /* clear cmd, ctl */
            clrCTL (dev);
            }
        else {                                          /* STC */
            setCMD (dev);                               /* set cmd, ctl */
            setCTL (dev);
            sim_activate (&ptr_unit, ptr_unit.wait);
            }
        break;

    default:
        break;
        }

if (IR & I_HC) { clrFSR (dev); }                        /* H/C option */
return dat;
}

/* Unit service */

t_stat ptr_svc (UNIT *uptr)
{
int32 dev, temp;

dev = ptr_dib.devno;                                    /* get device no */
clrCMD (dev);                                           /* clear cmd */
if ((ptr_unit.flags & UNIT_ATT) == 0)                   /* attached? */
    return IORETURN (ptr_stopioe, SCPE_UNATT);
while ((temp = getc (ptr_unit.fileref)) == EOF) {       /* read byte, error? */
    if (feof (ptr_unit.fileref)) {                      /* end of file? */
        if ((ptr_unit.flags & UNIT_DIAG) && (ptr_unit.pos > 0)) {
            rewind (ptr_unit.fileref);                  /* rewind if loop mode */
            ptr_unit.pos = 0;
            }
        else {
            if (ptr_trlcnt >= ptr_trllim) {             /* added all trailer? */        
                if (ptr_stopioe) {                      /* stop on error? */
                    printf ("PTR end of file\n");
                    return SCPE_IOERR;
                    }
                else return SCPE_OK;                    /* no, just hang */
                }
            ptr_trlcnt++;                               /* count trailer */
            temp = 0;                                   /* read a zero */
            break;
            }
        }
    else {                                              /* no, real error */
        perror ("PTR I/O error");
        clearerr (ptr_unit.fileref);
        return SCPE_IOERR;
        }
    }
setFSR (dev);                                           /* set flag */
ptr_unit.buf = temp & 0377;                             /* put byte in buf */
ptr_unit.pos = ftell (ptr_unit.fileref);
return SCPE_OK;
}

/* Attach routine - clear the trailer counter */

t_stat ptr_attach (UNIT *uptr, char *cptr)
{
ptr_trlcnt = 0;
return attach_unit (uptr, cptr);
}

/* Reset routine - called from SCP, flags in DIB's */

t_stat ptr_reset (DEVICE *dptr)
{
ptr_dib.cmd = ptr_dib.ctl = 0;                          /* clear cmd, ctl */
ptr_dib.flg = ptr_dib.fbf = ptr_dib.srq = 1;            /* set flg, fbf, srq */
ptr_unit.buf = 0;
sim_cancel (&ptr_unit);                                 /* deactivate unit */
return SCPE_OK;
}

/* Paper tape reader bootstrap routine (HP 12992K ROM) */

const uint16 ptr_rom[IBL_LNT] = {
    0107700,                    /*ST CLC 0,C            ; intr off */
    0002401,                    /*   CLA,RSS            ; skip in */            
    0063756,                    /*CN LDA M11            ; feed frame */
    0006700,                    /*   CLB,CCE            ; set E to rd byte */
    0017742,                    /*   JSB READ           ; get #char */
    0007306,                    /*   CMB,CCE,INB,SZB    ; 2's comp */
    0027713,                    /*   JMP *+5            ; non-zero byte */
    0002006,                    /*   INA,SZA            ; feed frame ctr */
    0027703,                    /*   JMP *-3 */
    0102077,                    /*   HLT 77B            ; stop */
    0027700,                    /*   JMP ST             ; next */
    0077754,                    /*   STA WC             ; word in rec */
    0017742,                    /*   JSB READ           ; get feed frame */
    0017742,                    /*   JSB READ           ; get address */
    0074000,                    /*   STB 0              ; init csum */
    0077755,                    /*   STB AD             ; save addr */
    0067755,                    /*CK LDB AD             ; check addr */
    0047777,                    /*   ADB MAXAD          ; below loader */
    0002040,                    /*   SEZ                ; E =0 => OK */
    0027740,                    /*   JMP H55 */
    0017742,                    /*   JSB READ           ; get word */
    0040001,                    /*   ADA 1              ; cont checksum */
    0177755,                    /*   STA AD,I           ; store word */
    0037755,                    /*   ISZ AD */
    0000040,                    /*   CLE                ; force wd read */
    0037754,                    /*   ISZ WC             ; block done? */
    0027720,                    /*   JMP CK             ; no */
    0017742,                    /*   JSB READ           ; get checksum */
    0054000,                    /*   CPB 0              ; ok? */
    0027702,                    /*   JMP CN             ; next block */
    0102011,                    /*   HLT 11             ; bad csum */
    0027700,                    /*   JMP ST             ; next */
    0102055,                    /*H55 HALT 55           ; bad address */
    0027700,                    /*   JMP ST             ; next */
    0000000,                    /*RD 0 */
    0006600,                    /*   CLB,CME            ; E reg byte ptr */
    0103710,                    /*   STC RDR,C          ; start reader */
    0102310,                    /*   SFS RDR            ; wait */
    0027745,                    /*   JMP *-1 */
    0106410,                    /*   MIB RDR            ; get byte */
    0002041,                    /*   SEZ,RSS            ; E set? */
    0127742,                    /*   JMP RD,I           ; no, done */
    0005767,                    /*   BLF,CLE,BLF        ; shift byte */
    0027744,                    /*   JMP RD+2           ; again */
    0000000,                    /*WC 000000             ; word count */
    0000000,                    /*AD 000000             ; address */
    0177765,                    /*M11 -11               ; feed count */
    0, 0, 0, 0, 0, 0, 0, 0,     /* unused */
    0, 0, 0, 0, 0, 0, 0,        /* unused */
    0000000                     /*MAXAD -ST             ; max addr */
    };

t_stat ptr_boot (int32 unitno, DEVICE *dptr)
{
int32 dev;

dev = ptr_dib.devno;                                    /* get device no */
if (ibl_copy (ptr_rom, dev)) return SCPE_IERR;          /* copy boot to memory */
SR = (SR & IBL_OPT) | IBL_PTR | (dev << IBL_V_DEV);     /* set SR */
return SCPE_OK;
}

/* Paper tape punch IO instructions */

int32 ptpio (int32 inst, int32 IR, int32 dat)
{
int32 dev;

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

    case ioLIX:                                         /* load */
        dat = 0;
    case ioMIX:                                         /* merge */
        if ((ptp_unit.flags & UNIT_ATT) == 0)
            dat = dat | PTP_LOW;                        /* out of tape? */
        break;

    case ioOTX:                                         /* output */
        ptp_unit.buf = dat;
        break;

    case ioCTL:                                         /* control clear/set */
        if (IR & I_CTL) {                               /* CLC */
            clrCMD (dev);                               /* clear cmd, ctl */
            clrCTL (dev);
            }
        else {                                          /* STC */
            setCMD (dev);                               /* set cmd, ctl */
            setCTL (dev);
            sim_activate (&ptp_unit, ptp_unit.wait);
            }
        break;

    default:
        break;
		}

if (IR & I_HC) { clrFSR (dev); }                        /* H/C option */
return dat;
}

/* Unit service */

t_stat ptp_svc (UNIT *uptr)
{
int32 dev;

dev = ptp_dib.devno;                                    /* get device no */
clrCMD (dev);                                           /* clear cmd */
setFSR (dev);                                           /* set flag */
if ((ptp_unit.flags & UNIT_ATT) == 0)                   /* attached? */
    return IORETURN (ptp_stopioe, SCPE_UNATT);
if (putc (ptp_unit.buf, ptp_unit.fileref) == EOF) {     /* output byte */
    perror ("PTP I/O error");
    clearerr (ptp_unit.fileref);
    return SCPE_IOERR;
    }
ptp_unit.pos = ftell (ptp_unit.fileref);                /* update position */
return SCPE_OK;
}

/* Reset routine */

t_stat ptp_reset (DEVICE *dptr)
{
ptp_dib.cmd = ptp_dib.ctl = 0;                          /* clear cmd, ctl */
ptp_dib.flg = ptp_dib.fbf = ptp_dib.srq = 1;            /* set flg, fbf, srq */
ptp_unit.buf = 0;
sim_cancel (&ptp_unit);                                 /* deactivate unit */
return SCPE_OK;
}

/* Terminal IO instructions */

int32 ttyio (int32 inst, int32 IR, int32 dat)
{
int32 dev;

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

    case ioLIX:                                         /* load */
        dat = 0;
    case ioMIX:                                         /* merge */
        dat = dat | tty_buf;
        if (!(tty_mode & TM_KBD) && sim_is_active (&tty_unit[TTO]))
            dat = dat | TP_BUSY;
        break;

    case ioOTX:                                         /* output */
        if (dat & TM_MODE) tty_mode = dat & (TM_KBD|TM_PRI|TM_PUN);
        tty_buf = dat & 0377;
        break;

    case ioCTL:                                         /* control clear/set */
        if (IR & I_CTL) { clrCTL (dev); }               /* CLC */
        else {                                          /* STC */
            setCTL (dev);
            if (!(tty_mode & TM_KBD))                   /* output? */
                sim_activate (&tty_unit[TTO], tty_unit[TTO].wait);
            }
        break;

    default:
        break;
        }

if (IR & I_HC) { clrFSR (dev); }                        /* H/C option */
return dat;
}

/* Unit service routines.  Note from Dave Bryan:

   Referring to the 12531C schematic, the terminal input enters on pin X 
   ("DATA FROM EIA COMPATIBLE DEVICE").  The signal passes through four 
   transistor inversions (Q8, Q1, Q2, and Q3) to appear on pin 12 of NAND gate 
   U104C.  If the flag flip-flop is not set, the terminal input passes to the 
   (inverted) output of U104C and thence to the D input of the first of the 
   flip-flops forming the data register.  

   In the idle condition (no key pressed), the terminal input line is marking 
   (voltage negative), so in passing through a total of five inversions, a 
   logic one is presented at the serial input of the data register.  During an 
   output operation, the register is parallel loaded and serially shifted, 
   sending the output data through the register to the device and -- this is 
   the crux -- filling the register with logic ones from U104C.  

   At the end of the output operation, the card flag is set, an interrupt 
   occurs, and the RTE driver is entered.  The driver then does an LIA SC to 
   read the contents of the data register.  If no key has been pressed during 
   the output operation, the register will read as all ones (octal 377).  If, 
   however, any key was struck, at least one zero bit will be present.  If the 
   register value doesn't equal 377, the driver sets the system "operator 
   attention" flag, which will cause RTE to output the asterisk and initiate a 
   terminal read when the current output line is completed. */

t_stat tti_svc (UNIT *uptr)
{
int32 c, dev;

dev = tty_dib.devno;                                    /* get device no */
sim_activate (uptr, uptr->wait);                        /* continue poll */
tty_shin = 0377;                                        /* assume inactive */
if (tty_lf) {                                           /* auto lf pending? */
    c = 012;                                            /* force lf */
    tty_lf = 0;
    }
else {
    if ((c = sim_poll_kbd ()) < SCPE_KFLAG) return c;   /* no char or error? */
    if (c & SCPE_BREAK) c = 0;                          /* break? */
    else c = sim_tt_inpcvt (c, TT_GET_MODE (uptr->flags));
    tty_lf = ((c & 0177) == 015) && (uptr->flags & UNIT_AUTOLF);
    }
if (tty_mode & TM_KBD) {                                /* keyboard enabled? */
    tty_buf = c;                                        /* put char in buf */
    uptr->pos = uptr->pos + 1;
    setFSR (dev);                                       /* set flag */
    if (c) {
        tto_out (c);                                    /* echo? */
        return ttp_out (c);                             /* punch? */
        }
    }
else tty_shin = c;                                      /* no, char shifts in */
return SCPE_OK;
}

t_stat tto_svc (UNIT *uptr)
{
int32 c, dev;
t_stat r;

c = tty_buf;                                            /* get char */
tty_buf = tty_shin;                                     /* shift in */
tty_shin = 0377;                                        /* line inactive */
if ((r = tto_out (c)) != SCPE_OK) {                     /* output; error? */
    sim_activate (uptr, uptr->wait);                    /* retry */
    return ((r == SCPE_STALL)? SCPE_OK: r);             /* !stall? report */
    }
dev = tty_dib.devno;                                    /* get device no */
setFSR (dev);                                           /* set done flag */
return ttp_out (c);                                     /* punch if enabled */
}

t_stat tto_out (int32 c)
{
t_stat r;

if (tty_mode & TM_PRI) {                                /* printing? */
    c = sim_tt_outcvt (c, TT_GET_MODE (tty_unit[TTO].flags));
    if (c >= 0) {                                       /* valid? */
        if (r = sim_putchar_s (c)) return r;            /* output char */
        tty_unit[TTO].pos = tty_unit[TTO].pos + 1;
        }
    }
return SCPE_OK;
}

t_stat ttp_out (int32 c)
{
if (tty_mode & TM_PUN) {                                /* punching? */
    if ((tty_unit[TTP].flags & UNIT_ATT) == 0)          /* attached? */
        return IORETURN (ttp_stopioe, SCPE_UNATT);
    if (putc (c, tty_unit[TTP].fileref) == EOF) {       /* output char */
        perror ("TTP I/O error");
        clearerr (tty_unit[TTP].fileref);
        return SCPE_IOERR;
        }
    tty_unit[TTP].pos = ftell (tty_unit[TTP].fileref);
    }
return SCPE_OK;
}

/* Reset routine */

t_stat tty_reset (DEVICE *dptr)
{
tty_dib.cmd = tty_dib.ctl = 0;                          /* clear cmd, ctl */
tty_dib.flg = tty_dib.fbf = tty_dib.srq = 1;            /* set flg, fbf, srq */
tty_mode = TM_KBD;                                      /* enable input */
tty_buf = 0;
tty_shin = 0377;                                        /* input inactive */
tty_lf = 0;                                             /* no lf pending */
sim_activate (&tty_unit[TTI], tty_unit[TTI].wait);      /* activate poll */
sim_cancel (&tty_unit[TTO]);                            /* cancel output */
return SCPE_OK;
}

t_stat tty_set_opt (UNIT *uptr, int32 val, char *cptr, void *desc)
{
int32 u = uptr - tty_dev.units;

if (u > TTO) return SCPE_NOFNC;
tty_unit[TTO].flags = (tty_unit[TTO].flags & ~TT_MODE) | val;
if (val == TT_MODE_7P) val = TT_MODE_7B;
tty_unit[TTI].flags = (tty_unit[TTI].flags & ~TT_MODE) | val;
return SCPE_OK;
}

t_stat tty_set_alf (UNIT *uptr, int32 val, char *cptr, void *desc)
{
int32 u = uptr - tty_dev.units;

if (u != TTI) return SCPE_NOFNC;
return SCPE_OK;
}

/* Clock IO instructions */

int32 clkio (int32 inst, int32 IR, int32 dat)
{
int32 dev;

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

    case ioMIX:                                         /* merge */
        dat = dat | clk_error;
        break;

    case ioLIX:                                         /* load */
        dat = clk_error;
        break;

    case ioOTX:                                         /* output */
        clk_select = dat & 07;                          /* save select */
        sim_cancel (&clk_unit);                         /* stop the clock */
        clrCTL (dev);                                   /* clear control */
        break;

    case ioCTL:                                         /* control clear/set */
        if (IR & I_CTL) {                               /* CLC */
            clrCTL (dev);                               /* turn off clock */
            sim_cancel (&clk_unit);                     /* deactivate unit */
            }
        else {                                          /* STC */
            setCTL (dev);                               /* set CTL */
            if (!sim_is_active (&clk_unit)) {           /* clock running? */
                sim_activate (&clk_unit,
                     sim_rtc_init (clk_delay (0)));     /* no, start clock */
                clk_ctr = clk_delay (1);                /* set repeat ctr */
                }
            clk_error = 0;                              /* clear error */
            }
        break;

    default:
        break;
        }

if (IR & I_HC) { clrFSR (dev); }                        /* H/C option */
return dat;
}

/* Unit service */

t_stat clk_svc (UNIT *uptr)
{
int32 tim, dev;

dev = clk_dib.devno;                                    /* get device no */
if (!CTL (dev)) return SCPE_OK;                         /* CTL off? done */
if (clk_unit.flags & UNIT_DIAG)                         /* diag mode? */
    tim = clk_delay (0);                                /* get fixed delay */
else tim = sim_rtc_calb (clk_tps[clk_select]);          /* calibrate delay */
sim_activate (uptr, tim);                               /* reactivate */
clk_ctr = clk_ctr - 1;                                  /* decrement counter */
if (clk_ctr <= 0) {                                     /* end of interval? */
    tim = FLG (dev);
    if (FLG (dev)) clk_error = CLK_ERROR;               /* overrun? error */
    else { setFSR (dev); }                              /* else set flag */
    clk_ctr = clk_delay (1);                            /* reset counter */
    }
return SCPE_OK;
}

/* Reset routine */

t_stat clk_reset (DEVICE *dptr)
{
clk_dib.cmd = clk_dib.ctl = 0;                          /* clear cmd, ctl */
clk_dib.flg = clk_dib.fbf = clk_dib.srq = 1;            /* set flg, fbf, srq */
clk_error = 0;                                          /* clear error */
clk_select = 0;                                         /* clear select */
clk_ctr = 0;                                            /* clear counter */
sim_cancel (&clk_unit);                                 /* deactivate unit */
return SCPE_OK;
}

/* Clock delay routine */

int32 clk_delay (int32 flg)
{
int32 sel = clk_select;

if ((clk_unit.flags & UNIT_DIAG) && (sel >= 4)) sel = sel - 3;
if (flg) return clk_rpt[sel];
else return clk_time[sel];
}
