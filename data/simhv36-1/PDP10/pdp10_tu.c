/* pdp10_tu.c - PDP-10 RH11/TM03/TU45 magnetic tape simulator

   Copyright (c) 1993-2006, Robert M Supnik

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

   tu           RH11/TM03/TU45 magtape

   16-Feb-06    RMS     Added tape capacity checking
   16-Aug-05    RMS     Fixed C++ declaration and cast problems
   07-Jul-05    RMS     Removed extraneous externs
   31-Mar-05    RMS     Fixed bug, ERASE/WREOF incorrectly clear CS1<done>
                        Fixed inaccuracies in error reporting
   18-Mar-05    RMS     Added attached test to detach routine
   23-Oct-04    RMS     Fixed setting done on non data transfers
   01-Oct-04    RMS     Modified to set FCE on read short record, eof
                        Implemented write check
                        TM03 uses only den<2> for validity test
                        TMK is cleared by new motion command, not DCLR
   14-Sep-04    RMS     Fixed RIP value
   25-Apr-03    RMS     Revised for extended file support
   28-Mar-03    RMS     Added multiformat support
   28-Feb-03    RMS     Revised for magtape library
   27-Jan-03    RMS     Changed to dynamically allocate buffer
   21-Nov-02    RMS     Fixed bug in bootstrap (reported by Michael Thompson)
                        Fixed bug in read (reported by Harris Newman)
   29-Sep-02    RMS     Added variable vector support
                        New data structures
   28-Aug-02    RMS     Added end of medium support
   30-May-02    RMS     Widened POS to 32b
   22-Apr-02    RMS     Changed record length error code
   06-Jan-02    RMS     Revised enable/disable support
   30-Nov-01    RMS     Added read only unit, extended SET/SHOW support
   24-Nov-01    RMS     Changed POS, FLG, UST to arrays
   23-Oct-01    RMS     Fixed bug in error interrupts
                        New IO page address constants
   05-Oct-01    RMS     Rewrote interrupt handling from schematics
   30-Sep-01    RMS     Fixed handling of non-existent formatters
   28-Sep-01    RMS     Fixed interrupt handling for SC/ATA
   4-May-01     RMS     Fixed bug in odd address test
   3-May-01     RMS     Fixed drive reset to clear SSC

   Magnetic tapes are represented as a series of variable 8b records
   of the form:

        32b record length in bytes - exact number, sign = error
        byte 0
        byte 1
        :
        byte n-2
        byte n-1
        32b record length in bytes - exact number, sign = error

   If the byte count is odd, the record is padded with an extra byte
   of junk.  File marks are represented by a single record length of 0.
   End of tape is two consecutive end of file marks.

   WARNING: The interupt logic of the RH11/RH70 is unusual and must be
   simulated with great precision.  The RH11 has an internal interrupt
   request flop, CSTB INTR, which is controlled as follows:
   - Writing IE and DONE simultaneously sets CSTB INTR
   - Controller clear, INIT, and interrupt acknowledge clear CSTB INTR
     (and also clear IE)
   - A transition of DONE from 0 to 1 sets CSTB from INTR
   The output of INTR is OR'd with the AND of RPCS1<SC,DONE,IE> to
   create the interrupt request signal.  Thus,
   - The DONE interrupt is edge sensitive, but the SC interrupt is
     level sensitive.
   - The DONE interrupt, once set, is not disabled if IE is cleared,
     but the SC interrupt is.
*/

#include "pdp10_defs.h"
#include "sim_tape.h"

#define TU_NUMFM        1                               /* #formatters */
#define TU_NUMDR        8                               /* #drives */
#define USTAT           u3                              /* unit status */
#define UDENS           u4                              /* unit density */
#define  UD_UNK         0                               /* unknown */
#define MT_MAXFR        (1 << 16)                       /* max data buf */

/* MTCS1 - 172440 - control/status 1 */

#define CS1_GO          CSR_GO                          /* go */
#define CS1_V_FNC       1                               /* function pos */
#define CS1_M_FNC       037                             /* function mask */
#define CS1_N_FNC       (CS1_M_FNC + 1)
#define CS1_FNC         (CS1_M_FNC << CS1_V_FNC)
#define  FNC_NOP        000                             /* no operation */
#define  FNC_UNLOAD     001                             /* unload */
#define  FNC_REWIND     003                             /* rewind */
#define  FNC_FCLR       004                             /* formatter clear */
#define  FNC_RIP        010                             /* read in preset */
#define  FNC_ERASE      012                             /* erase tape */
#define  FNC_WREOF      013                             /* write tape mark */
#define  FNC_SPACEF     014                             /* space forward */
#define  FNC_SPACER     015                             /* space reverse */
#define FNC_XFER        024                             /* >=? data xfr */
#define  FNC_WCHKF      024                             /* write check */
#define  FNC_WCHKR      027                             /* write check rev */
#define  FNC_WRITE      030                             /* write */
#define  FNC_READF      034                             /* read forward */
#define  FNC_READR      037                             /* read reverse */
#define CS1_IE          CSR_IE                          /* int enable */
#define CS1_DONE        CSR_DONE                        /* ready */
#define CS1_V_UAE       8                               /* Unibus addr ext */
#define CS1_M_UAE       03
#define CS1_UAE         (CS1_M_UAE << CS1_V_UAE)
#define CS1_DVA         0004000                         /* drive avail NI */
#define CS1_MCPE        0020000                         /* Mbus par err NI */
#define CS1_TRE         0040000                         /* transfer err */
#define CS1_SC          0100000                         /* special cond */
#define CS1_MBZ         0012000
#define CS1_DRV         (CS1_FNC | CS1_GO)
#define GET_FNC(x)      (((x) >> CS1_V_FNC) & CS1_M_FNC)
#define GET_UAE(x)      (((x) & CS1_UAE) << (16 - CS1_V_UAE))

/* MTWC - 172442 - word count */

/* MTBA - 172444 - base address */

#define BA_MBZ          0000001                         /* must be zero */

/* MTFC - 172446 - frame count */

/* MTCS2 - 172450 - control/status 2 */

#define CS2_V_FMTR      0                               /* formatter select */
#define CS2_M_FMTR      07
#define CS2_FMTR        (CS2_M_FMTR << CS2_V_FMTR)
#define CS2_UAI         0000010                         /* addr inhibit NI */
#define CS2_PAT         0000020                         /* parity test NI */
#define CS2_CLR         0000040                         /* controller clear */
#define CS2_IR          0000100                         /* input ready */
#define CS2_OR          0000200                         /* output ready */
#define CS2_MDPE        0000400                         /* Mbus par err NI */
#define CS2_MXF         0001000                         /* missed xfer NI */
#define CS2_PGE         0002000                         /* program err */
#define CS2_NEM         0004000                         /* nx mem err */
#define CS2_NEF         0010000                         /* nx fmter err */
#define CS2_PE          0020000                         /* parity err NI */
#define CS2_WCE         0040000                         /* write chk err */
#define CS2_DLT         0100000                         /* data late NI */
#define CS2_MBZ         (CS2_CLR | CS2_WCE)
#define CS2_RW          (CS2_FMTR | CS2_UAI | CS2_PAT | CS2_MXF | CS2_PE)
#define CS2_ERR         (CS2_MDPE | CS2_MXF | CS2_PGE | CS2_NEM | \
                     CS2_NEF | CS2_PE | CS2_DLT )
#define GET_FMTR(x)     (((x) >> CS2_V_FMTR) & CS2_M_FMTR)

/* MTFS - 172452 - formatter status
   + indicates kept in drive status
   ^ indicates calculated on the fly
*/

#define FS_SAT          0000001                         /* slave attention */
#define FS_BOT          0000002                         /* ^beginning of tape */
#define FS_TMK          0000004                         /* end of file */
#define FS_ID           0000010                         /* ID burst detected */
#define FS_SLOW         0000020                         /* slowing down NI */
#define FS_PE           0000040                         /* ^PE status */
#define FS_SSC          0000100                         /* slave stat change */
#define FS_RDY          0000200                         /* ^formatter ready */
#define FS_FPR          0000400                         /* formatter present */
#define FS_EOT          0002000                         /* +end of tape */
#define FS_WRL          0004000                         /* ^write locked */
#define FS_MOL          0010000                         /* ^medium online */
#define FS_PIP          0020000                         /* +pos in progress */
#define FS_ERR          0040000                         /* ^error */
#define FS_ATA          0100000                         /* attention active */
#define FS_REW          0200000                         /* +rewinding */

#define FS_DYN          (FS_ERR | FS_PIP | FS_MOL | FS_WRL | FS_EOT | \
                         FS_RDY | FS_PE | FS_BOT)

/* MTER - 172454 - error register */

#define ER_ILF          0000001                         /* illegal func */
#define ER_ILR          0000002                         /* illegal register */
#define ER_RMR          0000004                         /* reg mod refused */
#define ER_MCP          0000010                         /* Mbus cpar err NI */
#define ER_FER          0000020                         /* format sel err */
#define ER_MDP          0000040                         /* Mbus dpar err NI */
#define ER_VPE          0000100                         /* vert parity err */
#define ER_CRC          0000200                         /* CRC err NI */
#define ER_NSG          0000400                         /* non std gap err NI */
#define ER_FCE          0001000                         /* frame count err */
#define ER_ITM          0002000                         /* inv tape mark NI */
#define ER_NXF          0004000                         /* wlock or fnc err */
#define ER_DTE          0010000                         /* time err NI */
#define ER_OPI          0020000                         /* op incomplete */
#define ER_UNS          0040000                         /* drive unsafe */
#define ER_DCK          0100000                         /* data check NI */

/* MTAS - 172456 - attention summary */

#define AS_U0           0000001                         /* unit 0 flag */

/* MTCC - 172460 - check character, read only */

#define CC_MBZ          0177000                         /* must be zero */

/* MTDB - 172462 - data buffer */

/* MTMR - 172464 - maintenance register */

#define MR_RW           0177637                         /* read/write */

/* MTDT - 172466 - drive type */

#define DT_NSA          0100000                         /* not sect addr */
#define DT_TAPE         0040000                         /* tape */
#define DT_PRES         0002000                         /* slave present */
#define DT_TM03         0000040                         /* TM03 formatter */
#define DT_OFF          0000010                         /* drive off */
#define DT_TE16         0000011                         /* TE16 */
#define DT_TU45         0000012                         /* TU45 */
#define DT_TU77         0000014                         /* TU77 */

/* MTSN - 172470 - serial number */

/* MTTC - 172472 - tape control register */

#define TC_V_UNIT       0                               /* unit select */
#define TC_M_UNIT       07
#define TC_V_EVN        0000010                         /* even parity */
#define TC_V_FMT        4                               /* format select */
#define TC_M_FMT        017
#define  TC_10C          00                             /* PDP-10 core dump */
#define  TC_IND          03                             /* industry standard */
#define TC_V_DEN        8                               /* density select */
#define TC_M_DEN        07
#define  TC_800          3                              /* 800 bpi */
#define  TC_1600         4                              /* 1600 bpi */
#define TC_AER          0010000                         /* abort on error */
#define TC_SAC          0020000                         /* slave addr change */
#define TC_FCS          0040000                         /* frame count status */
#define TC_ACC          0100000                         /* accelerating NI */
#define TC_RW           0013777
#define TC_MBZ          0004000
#define TC_RIP          ((TC_800 << TC_V_DEN) || (TC_10C << TC_V_FMT))
#define GET_DEN(x)      (((x) >> TC_V_DEN) & TC_M_DEN)
#define GET_FMT(x)      (((x) >> TC_V_FMT) & TC_M_FMT)
#define GET_DRV(x)      (((x) >> TC_V_UNIT) & TC_M_UNIT)

/* Mapping macros */

#define XWC_MBZ         0000001                         /* wc<0> mbz */
#define XBA_MBZ         0000001                         /* addr<0> mbz */
#define XBA_ODD         0000002                         /* odd address */
#define TXFR(b,w,od)    if (((b) & XBA_MBZ) || ((w) & XWC_MBZ) || \
                            (((b) & XBA_ODD) != ((od) << 1))) { \
                            tucs2 = tucs2 | CS2_NEM; \
                            ubcs[1] = ubcs[1] | UBCS_TMO; \
                            tucs1 = tucs1 & ~CS1_GO; \
                            update_tucs (CS1_DONE, drv); \
                            return SCPE_OK; \
                            }
#define NEWPAGE(v,m)    (((v) & PAG_M_OFF) == (m))
#define MAPM(v,p,f)     vpn = PAG_GETVPN (v); \
                        if ((vpn >= UMAP_MEMSIZE) || ((ubmap[1][vpn] & \
                            (UMAP_VLD | UMAP_DSB | UMAP_RRV)) != \
                            (UMAP_VLD | f))) { \
                            tucs2 = tucs2 | CS2_NEM; \
                            ubcs[1] = ubcs[1] | UBCS_TMO; \
                            break; \
                            } \
                        p = (ubmap[1][vpn] + PAG_GETOFF (v)) & PAMASK; \
                        if (MEM_ADDR_NXM (p)) { \
                            tucs2 = tucs2 | CS2_NEM; \
                            ubcs[1] = ubcs[1] | UBCS_TMO; \
                            break; \
                            }

extern d10 *M;                                          /* memory */
extern int32 int_req;
extern int32 ubmap[UBANUM][UMAP_MEMSIZE];               /* Unibus map */
extern int32 ubcs[UBANUM];
extern UNIT cpu_unit;
extern int32 sim_switches;
extern FILE *sim_deb;

int32 tucs1 = 0;                                        /* control/status 1 */
int32 tuwc = 0;                                         /* word count */
int32 tuba = 0;                                         /* bus address */
int32 tufc = 0;                                         /* frame count */
int32 tucs2 = 0;                                        /* control/status 2 */
int32 tufs = 0;                                         /* formatter status */
int32 tuer = 0;                                         /* error status */
int32 tucc = 0;                                         /* check character */
int32 tudb = 0;                                         /* data buffer */
int32 tumr = 0;                                         /* maint register */
int32 tutc = 0;                                         /* tape control */
int32 tuiff = 0;                                        /* INTR flip/flop */
int32 tu_time = 10;                                     /* record latency */
int32 tu_stopioe = 1;                                   /* stop on error */
int32 tu_log = 0;                                       /* debug */
int32 reg_in_fmtr[32] = {                               /* reg in formatter */
    0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    };
int32 reg_in_fmtr1[32] = {                              /* rmr if write + go */
    0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    };
int32 fmt_test[16] = {                                  /* fmt bytes/10 wd */
    5, 0, 5, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
static char *tu_fname[CS1_N_FNC] = {
    "NOP", "UNLD", "2", "REW", "FCLR", "5", "6", "7",
    "RIP", "11", "ERASE", "WREOF", "SPCF", "SPCR", "16", "17",
    "20", "21", "22", "23", "WRCHKF", "25", "26", "WRCHKR",
    "WRITE", "31", "32", "33", "READF", "35", "36" "READR"
    };
static uint8 *xbuf = NULL;                              /* xfer buffer */

t_stat tu_rd (int32 *data, int32 PA, int32 access);
t_stat tu_wr (int32 data, int32 PA, int32 access);
int32 tu_inta (void);
t_stat tu_svc (UNIT *uptr);
t_stat tu_reset (DEVICE *dptr);
t_stat tu_attach (UNIT *uptr, char *cptr);
t_stat tu_detach (UNIT *uptr);
t_stat tu_boot (int32 unitno, DEVICE *dptr);
void tu_go (int32 drv);
void set_tuer (int32 flag);
void update_tucs (int32 flag, int32 drv);
t_stat tu_map_err (UNIT *uptr, t_stat st, t_bool qdt);

/* TU data structures

   tu_dev       TU device descriptor
   tu_unit      TU unit list
   tu_reg       TU register list
   tu_mod       TU modifier list
*/

DIB tu_dib = {
    IOBA_TU, IOLN_TU, &tu_rd, &tu_wr,
    1, IVCL (TU), VEC_TU, { &tu_inta }
    };

UNIT tu_unit[] = {
    { UDATA (&tu_svc, UNIT_ATTABLE+UNIT_DISABLE+UNIT_ROABLE, 0) },
    { UDATA (&tu_svc, UNIT_ATTABLE+UNIT_DISABLE+UNIT_ROABLE, 0) },
    { UDATA (&tu_svc, UNIT_ATTABLE+UNIT_DISABLE+UNIT_ROABLE, 0) },
    { UDATA (&tu_svc, UNIT_ATTABLE+UNIT_DISABLE+UNIT_ROABLE, 0) },
    { UDATA (&tu_svc, UNIT_ATTABLE+UNIT_DISABLE+UNIT_ROABLE, 0) },
    { UDATA (&tu_svc, UNIT_ATTABLE+UNIT_DISABLE+UNIT_ROABLE, 0) },
    { UDATA (&tu_svc, UNIT_ATTABLE+UNIT_DISABLE+UNIT_ROABLE, 0) },
    { UDATA (&tu_svc, UNIT_ATTABLE+UNIT_DISABLE+UNIT_ROABLE, 0) }
    };

REG tu_reg[] = {
    { ORDATA (MTCS1, tucs1, 16) },
    { ORDATA (MTWC, tuwc, 16) },
    { ORDATA (MTBA, tuba, 16) },
    { ORDATA (MTFC, tufc, 16) },
    { ORDATA (MTCS2, tucs2, 16) },
    { ORDATA (MTFS, tufs, 16) },
    { ORDATA (MTER, tuer, 16) },
    { ORDATA (MTCC, tucc, 16) },
    { ORDATA (MTDB, tudb, 16) },
    { ORDATA (MTMR, tumr, 16) },
    { ORDATA (MTTC, tutc, 16) },
    { FLDATA (IFF, tuiff, 0) },
    { FLDATA (INT, int_req, INT_V_TU) },
    { FLDATA (DONE, tucs1, CSR_V_DONE) },
    { FLDATA (IE, tucs1, CSR_V_IE) },
    { FLDATA (STOP_IOE, tu_stopioe, 0) },
    { DRDATA (TIME, tu_time, 24), PV_LEFT },
    { URDATA (UST, tu_unit[0].USTAT, 8, 17, 0, TU_NUMDR, 0) },
    { URDATA (POS, tu_unit[0].pos, 10, T_ADDR_W, 0,
              TU_NUMDR, PV_LEFT | REG_RO) },
    { ORDATA (LOG, tu_log, 8), REG_HIDDEN },
    { NULL }
    };

MTAB tu_mod[] = {
    { MTUF_WLK, 0, "write enabled", "WRITEENABLED", NULL },
    { MTUF_WLK, MTUF_WLK, "write locked", "LOCKED", NULL }, 
    { MTAB_XTD|MTAB_VUN, 0, "FORMAT", "FORMAT",
      &sim_tape_set_fmt, &sim_tape_show_fmt, NULL },
    { MTAB_XTD|MTAB_VUN, 0, "CAPACITY", "CAPACITY",
      &sim_tape_set_capac, &sim_tape_show_capac, NULL },
    { MTAB_XTD|MTAB_VDV, 0, "ADDRESS", NULL,
      NULL, &show_addr, NULL },
    { MTAB_XTD|MTAB_VDV, 0, "VECTOR", NULL,
      NULL, &show_vec, NULL },
    { 0 }
    };

DEVICE tu_dev = {
    "TU", tu_unit, tu_reg, tu_mod,
    TU_NUMDR, 10, 31, 1, 8, 8,
    NULL, NULL, &tu_reset,
    &tu_boot, &tu_attach, &tu_detach,
    &tu_dib, DEV_UBUS | DEV_DEBUG
    };

/* I/O dispatch routine, I/O addresses 17772440 - 17772472 */

t_stat tu_rd (int32 *data, int32 PA, int32 access)
{
int32 fmtr, drv, j;

fmtr = GET_FMTR (tucs2);                                /* get current fmtr */
drv = GET_DRV (tutc);                                   /* get current drive */
j = (PA >> 1) & 017;                                    /* get reg offset */
if (reg_in_fmtr[j] && (fmtr != 0)) {                    /* nx formatter */
    tucs2 = tucs2 | CS2_NEF;                            /* set error flag */
    update_tucs (CS1_SC, drv);                          /* request intr */
    *data = 0;
    return SCPE_OK;
    }

update_tucs (0, drv);                                   /* update status */
switch (j) {                                            /* decode PA<4:1> */

    case 000:                                           /* MTCS1 */
        if (fmtr != 0) *data = tucs1 & ~CS1_DRV;
        else *data = tucs1;
        break;

    case 001:                                           /* MTWC */
        *data = tuwc;
        break;

    case 002:                                           /* MTBA */
        *data = tuba = tuba & ~BA_MBZ;
        break;

    case 003:                                           /* MTFC */
        *data = tufc;
        break;

    case 004:                                           /* MTCS2 */
        *data = tucs2 = (tucs2 & ~CS2_MBZ) | CS2_IR | CS2_OR;
        break;

    case 005:                                           /* MTFS */
        *data = tufs & 0177777;                         /* mask off rewind */
        break;

    case 006:                                           /* MTER */
        *data = tuer;
        break;

    case 007:                                           /* MTAS */
        *data = (tufs & FS_ATA)? AS_U0: 0;
        break;

    case 010:                                           /* MTCC */
        *data = tucc = tucc & ~CC_MBZ;
        break;

    case 011:                                           /* MTDB */
        *data = tudb;
        break;

    case 012:                                           /* MTMR */
        *data = tumr;
        break;

    case 013:                                           /* MTDT */
        *data = DT_NSA | DT_TAPE | DT_TM03 |
             ((tu_unit[drv].flags & UNIT_DIS)? DT_OFF: (DT_PRES | DT_TU45));
        break;

    case 014:                                           /* MTSN */
        *data = (tu_unit[drv].flags & UNIT_DIS)? 0: 040 | (drv + 1);
        break;

    case 015:                                           /* MTTC */
        *data = tutc = tutc & ~TC_MBZ;
        break;

    default:                                            /* all others */
        set_tuer (ER_ILR);
        update_tucs (0, drv);
        break;
        }

return SCPE_OK;
}

t_stat tu_wr (int32 data, int32 PA, int32 access)
{
int32 cs1f, fmtr, drv, j;

cs1f = 0;                                               /* no int on cs1 upd */
fmtr = GET_FMTR (tucs2);                                /* get formatter */
drv = GET_DRV (tutc);                                   /* get current unit */
j = (PA >> 1) & 017;                                    /* get reg offset */
if (reg_in_fmtr[j] && (fmtr != 0)) {                    /* nx formatter */
    tucs2 = tucs2 | CS2_NEF;                            /* set error flag */
    update_tucs (CS1_SC, drv);                          /* request intr */
    return SCPE_OK;
    }
if (reg_in_fmtr1[j] && ((tucs1 & CS1_DONE) == 0)) {     /* formatter busy? */
    set_tuer (ER_RMR);                                  /* won't write */
    update_tucs (0, drv);
    return SCPE_OK;
    }

switch (j) {                                            /* decode PA<4:1> */

    case 000:                                           /* MTCS1 */
        if ((access == WRITEB) && (PA & 1)) data = data << 8;
        if (data & CS1_TRE) {                           /* error clear? */
            tucs1 = tucs1 & ~CS1_TRE;                   /* clr CS1<TRE> */
            tucs2 = tucs2 & ~CS2_ERR;                   /* clr CS2<15:8> */
            }
        if ((access == WRITE) || (PA & 1)) {            /* hi byte write? */
            if (tucs1 & CS1_DONE)                       /* done set? */
                tucs1 = (tucs1 & ~CS1_UAE) | (data & CS1_UAE);
            }
        if ((access == WRITE) || !(PA & 1)) {           /* lo byte write? */
            if ((data & CS1_DONE) && (data & CS1_IE))   /* to DONE+IE? */
                tuiff = 1;                              /* set CSTB INTR */
            tucs1 = (tucs1 & ~CS1_IE) | (data & CS1_IE);
            if (fmtr != 0) {                            /* nx formatter? */
                tucs2 = tucs2 | CS2_NEF;                /* set error flag */
                cs1f = CS1_SC;                          /* req interrupt */
                }
            else if (tucs1 & CS1_GO) {                  /* busy? */
                if (tucs1 & CS1_DONE) set_tuer (ER_RMR);
                else tucs2 = tucs2 | CS2_PGE;
                }
            else {
                tucs1 = (tucs1 & ~CS1_DRV) | (data & CS1_DRV);
                if (tucs1 & CS1_GO) tu_go (drv);
                }
            }
        break;  

    case 001:                                           /* MTWC */
        if (access == WRITEB) data = (PA & 1)?
            (tuwc & 0377) | (data << 8): (tuwc & ~0377) | data;
        tuwc = data;
        break;

    case 002:                                           /* MTBA */
        if (access == WRITEB) data = (PA & 1)?
            (tuba & 0377) | (data << 8): (tuba & ~0377) | data;
        tuba = data & ~BA_MBZ;
        break;

    case 003:                                           /* MTFC */
        if (access == WRITEB) data = (PA & 1)?
            (tufc & 0377) | (data << 8): (tufc & ~0377) | data;
        tufc = data;
        tutc = tutc | TC_FCS;                           /* set fc flag */
        break;

    case 004:                                           /* MTCS2 */
        if ((access == WRITEB) && (PA & 1)) data = data << 8;
        if (data & CS2_CLR) tu_reset (&tu_dev);         /* init? */
        else {
            if ((data & ~tucs2) & (CS2_PE | CS2_MXF))
                cs1f = CS1_SC;                          /* diagn intr */
            if (access == WRITEB) data = (tucs2 &       /* merge data */
                ((PA & 1)? 0377: 0177400)) | data;
            tucs2 = (tucs2 & ~CS2_RW) | (data & CS2_RW) | CS2_IR | CS2_OR;
            }
        break;

    case 007:                                           /* MTAS */
        if ((access == WRITEB) && (PA & 1)) break;
        if (data & AS_U0) tufs = tufs & ~FS_ATA;
        break;

    case 011:                                           /* MTDB */
        if (access == WRITEB) data = (PA & 1)?
            (tudb & 0377) | (data << 8): (tudb & ~0377) | data;
        tudb = data;
        break;

    case 012:                                           /* MTMR */
        if (access == WRITEB) data = (PA & 1)?
            (tumr & 0377) | (data << 8): (tumr & ~0377) | data;
        tumr = (tumr & ~MR_RW) | (data & MR_RW);
        break;

    case 015:                                           /* MTTC */
        if (access == WRITEB) data = (PA & 1)?
            (tutc & 0377) | (data << 8): (tutc & ~0377) | data;
        tutc = (tutc & ~TC_RW) | (data & TC_RW) | TC_SAC;
        drv = GET_DRV (tutc);
        break;

    case 005:                                           /* MTFS */
    case 006:                                           /* MTER */
    case 010:                                           /* MTCC */
    case 013:                                           /* MTDT */
    case 014:                                           /* MTSN */
        break;                                          /* read only */

    default:                                            /* all others */
        set_tuer (ER_ILR);
        break;
        }                                               /* end switch */

update_tucs (cs1f, drv);                                /* update status */
return SCPE_OK;
}

/* New magtape command */

void tu_go (int32 drv)
{
int32 fnc, den;
UNIT *uptr;

fnc = GET_FNC (tucs1);                                  /* get function */
den = GET_DEN (tutc);                                   /* get density */
uptr = tu_dev.units + drv;                              /* get unit */
if (DEBUG_PRS (tu_dev)) fprintf (sim_deb,
    ">>TU%d STRT: fnc=%s, cs1=%06o, cs2=%06o, ba=%06o, wc=%06o, fc=%06o, fs=%06o, er=%06o, pos=%d\n",
    drv, tu_fname[fnc], tucs1, tucs2, tuba, tuwc, tufc, tufs, tuer, uptr->pos);
if ((fnc != FNC_FCLR) &&                                /* not clear & err */
    ((tufs & FS_ERR) || sim_is_active (uptr))) {        /* or in motion? */
    set_tuer (ER_ILF);                                  /* set err, ATN */
    tucs1 = tucs1 & ~CS1_GO;                            /* clear go */
    update_tucs (CS1_SC, drv);                          /* request intr */
    return;
    }
tufs = tufs & ~FS_ATA;                                  /* clear attention */
tutc = tutc & ~TC_SAC;                                  /* clear addr change */

switch (fnc) {                                          /* case on function */
    case FNC_FCLR:                                      /* drive clear */
        tuer = 0;                                       /* clear errors */
        tutc = tutc & ~TC_FCS;                          /* clear fc status */
        tufs = tufs & ~(FS_SAT | FS_SSC | FS_ID | FS_ERR);
        sim_cancel (uptr);                              /* reset drive */
        uptr->USTAT = 0;
    case FNC_NOP:
        tucs1 = tucs1 & ~CS1_GO;                        /* no operation */
        return;

    case FNC_RIP:                                       /* read-in preset */
        tutc = TC_RIP;                                  /* density = 800 */
        sim_tape_rewind (&tu_unit[0]);                  /* rewind unit 0 */
        tu_unit[0].USTAT = 0;
        tucs1 = tucs1 & ~CS1_GO;
        tufs = tufs & ~FS_TMK;
        return;

    case FNC_UNLOAD:                                    /* unload */
        if ((uptr->flags & UNIT_ATT) == 0) {            /* unattached? */
            set_tuer (ER_UNS);
            break;
            }
        detach_unit (uptr);
        uptr->USTAT = FS_REW;
        sim_activate (uptr, tu_time);
        tucs1 = tucs1 & ~CS1_GO;
        tufs = tufs & ~FS_TMK;
        return; 

    case FNC_REWIND:
        if ((uptr->flags & UNIT_ATT) == 0) {            /* unattached? */
            set_tuer (ER_UNS);
            break;
            }
        uptr->USTAT = FS_PIP | FS_REW;
        sim_activate (uptr, tu_time);
        tucs1 = tucs1 & ~CS1_GO;
        tufs = tufs & ~FS_TMK;
        return;

    case FNC_SPACEF:
        if ((uptr->flags & UNIT_ATT) == 0) {            /* unattached? */
            set_tuer (ER_UNS);
            break;
            }
        if (sim_tape_eot (uptr) || ((tutc & TC_FCS) == 0)) {
            set_tuer (ER_NXF);
            break;
            }
        uptr->USTAT = FS_PIP;
        goto GO_XFER;

    case FNC_SPACER:
        if ((uptr->flags & UNIT_ATT) == 0) {            /* unattached? */
            set_tuer (ER_UNS);
            break;
            }
        if (sim_tape_bot (uptr) || ((tutc & TC_FCS) == 0)) {
            set_tuer (ER_NXF);
            break;
            }
        uptr->USTAT = FS_PIP;
        goto GO_XFER;

    case FNC_WREOF:                                     /* write tape mark */
    case FNC_ERASE:                                     /* erase */
        if ((uptr->flags & UNIT_ATT) == 0) {            /* unattached? */
            set_tuer (ER_UNS);
            break;
            }
        if (sim_tape_wrp (uptr)) {                      /* write locked? */
            set_tuer (ER_NXF);
            break;
            }
        if (fmt_test[GET_FMT (tutc)] == 0) {            /* invalid format? */
            set_tuer (ER_FER);
            break;
            }
        if (uptr->UDENS == UD_UNK) uptr->UDENS = den;   /* set dens */
        uptr->USTAT = 0;
        goto GO_XFER;

    case FNC_WCHKR:                                     /* wchk = read */
    case FNC_READR:                                     /* read rev */
        if (tufs & FS_BOT) {                            /* beginning of tape? */
            set_tuer (ER_NXF);
            break;
            }
        goto DATA_XFER;

    case FNC_WRITE:                                     /* write */
        if (((tutc & TC_FCS) == 0) ||                   /* frame cnt = 0? */
            ((den == TC_800) && (tufc > 0777765))) {    /* NRZI, fc < 13? */
            set_tuer (ER_NXF);
            break;
            }
    case FNC_WCHKF:                                     /* wchk = read */
    case FNC_READF:                                     /* read */
    DATA_XFER:
        if ((uptr->flags & UNIT_ATT) == 0) {            /* unattached? */
            set_tuer (ER_UNS);
            break;
            }
        if (fmt_test[GET_FMT (tutc)] == 0) {            /* invalid format? */
            set_tuer (ER_FER);
            break;
            }
        if (uptr->UDENS == UD_UNK) uptr->UDENS = den;   /* set dens */
        uptr->USTAT = 0;
        tucs1 = tucs1 & ~CS1_DONE;                      /* clear done */
    GO_XFER:
        tucs2 = tucs2 & ~CS2_ERR;                       /* clear errors */
        tucs1 = tucs1 & ~(CS1_TRE | CS1_MCPE);
        tufs = tufs & ~(FS_TMK | FS_ID);                /* clear eof, id */
        sim_activate (uptr, tu_time);
        return;

    default:                                            /* all others */
        set_tuer (ER_ILF);                              /* not supported */
        break;
        }                                               /* end case function */

tucs1 = tucs1 & ~CS1_GO;                                /* clear go */
update_tucs (CS1_SC, drv);                              /* set intr */
return;
}

/* Unit service

   Complete movement or data transfer command
   Unit must exist - can't remove an active unit
   Unit must be attached - detach cancels in progress operations
*/

t_stat tu_svc (UNIT *uptr)
{
int32 fnc, fmt, i, j, k, wc10, ba10;
int32 ba, fc, wc, drv, mpa10, vpn;
d10 val, v[4];
t_mtrlnt tbc;
t_stat st, r = SCPE_OK;

drv = (int32) (uptr - tu_dev.units);                    /* get drive # */
if (uptr->USTAT & FS_REW) {                             /* rewind or unload? */
    sim_tape_rewind (uptr);                             /* rewind tape */
    uptr->USTAT = 0;                                    /* clear status */
    tufs = tufs | FS_ATA | FS_SSC;
    update_tucs (CS1_SC, drv);                          /* update status */
    return SCPE_OK;
    }

fnc = GET_FNC (tucs1);                                  /* get command */
fmt = GET_FMT (tutc);                                   /* get format */
ba = GET_UAE (tucs1) | tuba;                            /* get bus address */
wc = 0200000 - tuwc;                                    /* get word count */
fc = 0200000 - tufc;                                    /* get frame count */
wc10 = wc >> 1;                                         /* 10 word count */
ba10 = ba >> 2;                                         /* 10 word addr */
uptr->USTAT = 0;                                        /* clear status */

switch (fnc) {                                          /* case on function */

/* Non-data transfer commands - set ATA when done */

    case FNC_SPACEF:                                    /* space forward */
        do {
            tufc = (tufc + 1) & 0177777;                /* incr fc */
            if (st = sim_tape_sprecf (uptr, &tbc)) {    /* space rec fwd, err? */
                r = tu_map_err (uptr, st, 0);           /* map error */
                break;
                }
            } while ((tufc != 0) && !sim_tape_eot (uptr));
        if (tufc) set_tuer (ER_FCE);
        else tutc = tutc & ~TC_FCS;
        tufs = tufs | FS_ATA;
        break;

    case FNC_SPACER:                                    /* space reverse */
        do {
            tufc = (tufc + 1) & 0177777;                /* incr wc */
            if (st = sim_tape_sprecr (uptr, &tbc)) {    /* space rec rev, err? */
                r = tu_map_err (uptr, st, 0);           /* map error */
                break;
                }
            } while (tufc != 0);
        if (tufc) set_tuer (ER_FCE);
        else tutc = tutc & ~TC_FCS;
        tufs = tufs | FS_ATA;
        break;

    case FNC_WREOF:                                             /* write end of file */
        if (st = sim_tape_wrtmk (uptr))                 /* write tmk, err? */
            r = tu_map_err (uptr, st, 0);               /* map error */
        tufs = tufs | FS_ATA;
        break;

    case FNC_ERASE:
        if (sim_tape_wrp (uptr))                        /* write protected? */
            r = tu_map_err (uptr, MTSE_WRP, 0);         /* map error */
        tufs = tufs | FS_ATA;
        break;

/* Data transfer commands

   These commands must take into account the action of the "bit fiddler", which
   converts between PDP-10 format and tape format.  Only two tape formats are
   supported:

   PDP-10 core dump: write 36b as byte 0/byte 1/byte 2/byte 3/0000'last nibble
   industry mode: write hi 32b as byte 0/byte 1/byte 2/byte 3

   These commands must also take into account the action of the Unibus adapter,
   which munges PDP-10 addresses through the Unibus map.
*/

    case FNC_READF:                                     /* read */
    case FNC_WCHKF:                                     /* wcheck = read */
        tufc = 0;                                       /* clear frame count */
        if ((uptr->UDENS == TC_1600) && sim_tape_bot (uptr))
            tufs = tufs | FS_ID;                        /* PE BOT? ID burst */
        TXFR (ba, wc, 0);                               /* validate transfer */
        if (st = sim_tape_rdrecf (uptr, xbuf, &tbc, MT_MAXFR)) { /* read fwd */
            r = tu_map_err (uptr, st, 1);               /* map error */
            break;                                      /* done */
            }
        for (i = j = 0; (i < wc10) && (j < ((int32) tbc)); i++) {
            if ((i == 0) || NEWPAGE (ba10 + i, 0)) {    /* map new page */
                MAPM (ba10 + i, mpa10, 0);
                }
            for (k = 0; k < 4; k++) v[k] = xbuf[j++];
            val = (v[0] << 28) | (v[1] << 20) | (v[2] << 12) | (v[3] << 4);
            if (fmt == TC_10C) val = val | ((d10) xbuf[j++] & 017);
            if (fnc == FNC_READF) M[mpa10] = val;       /* read? store */
            else if (M[mpa10] != val) {                 /* wchk, mismatch? */
                tucs2 = tucs2 | CS2_WCE;                /* flag, stop */
                break;
                }
            mpa10 = mpa10 + 1;
            }                                           /* end for */
        tufc = tbc & 0177777;
        tuwc = (tuwc + (i << 1)) & 0177777;
        ba = ba + (i << 2);
        if (tuwc) set_tuer (ER_FCE);                    /* short record? */
        break;

    case FNC_WRITE:                                     /* write */
        TXFR (ba, wc, 0);                               /* validate transfer */
        for (i = j = 0; (i < wc10) && (j < fc); i++) {
            if ((i == 0) || NEWPAGE (ba10 + i, 0)) {    /* map new page */
                MAPM (ba10 + i, mpa10, 0);
                }
            val = M[mpa10];
            xbuf[j++] = (uint8) ((val >> 28) & 0377);
            xbuf[j++] = (uint8) ((val >> 20) & 0377);
            xbuf[j++] = (uint8) ((val >> 12) & 0377);
            xbuf[j++] = (uint8) ((val >> 4) & 0377);
            if (fmt == TC_10C) xbuf[j++] = (uint8) (val & 017);
            mpa10 = mpa10 + 1;
            }                                           /* end for */
        if (j < fc) fc = j;                             /* short record? */
        if (st = sim_tape_wrrecf (uptr, xbuf, fc))      /* write rec, err? */
            r = tu_map_err (uptr, st, 1);               /* map error */
        else {
            tufc = (tufc + fc) & 0177777;
            if (tufc == 0) tutc = tutc & ~TC_FCS;
            tuwc = (tuwc + (i << 1)) & 0177777;
            ba = ba + (i << 2);
            }
        break;

    case FNC_READR:                                     /* read reverse */
    case FNC_WCHKR:                                     /* wcheck = read */
        tufc = 0;                                       /* clear frame count */
        TXFR (ba, wc, 1);                               /* validate xfer rev */
        if (st = sim_tape_rdrecr (uptr, xbuf + 4, &tbc, MT_MAXFR)) { /* read rev */
            r = tu_map_err (uptr, st, 1);               /* map error */
            break;                                      /* done */
            }
        for (i = 0; i < 4; i++) xbuf[i] = 0;
        for (i = 0, j = tbc + 4; (i < wc10) && (j >= 4); i++) {
            if ((i == 0) || NEWPAGE (ba10 - i, PAG_M_OFF)) { /* map page */
                MAPM (ba10 - i, mpa10, UMAP_RRV);
                }
            val = ((fmt == TC_10C)? (((d10) xbuf [--j]) & 017): 0);
            for (k = 0; k < 4; i++) v[k] = xbuf[--j];
            val = val | (v[0] << 4) | (v[1] << 12) | (v[2] << 20) | (v[3] << 28);
            if (fnc == FNC_READR) M[mpa10] = val;       /* read? store */
            else if (M[mpa10] != val) {                 /* wchk, mismatch? */
                tucs2 = tucs2 | CS2_WCE;                /* flag, stop */
                break;
                }
            mpa10 = mpa10 - 1;
            }                                           /* end for */
        tufc = tbc & 0177777;
        tuwc = (tuwc + (i << 1)) & 0177777;
        ba = ba - (i << 2);
        if (tuwc) set_tuer (ER_FCE);                    /* short record? */
        break;
        }                                               /* end case */

tucs1 = (tucs1 & ~CS1_UAE) | ((ba >> (16 - CS1_V_UAE)) & CS1_UAE);
tuba = ba & 0177777;                                    /* update mem addr */
tucs1 = tucs1 & ~CS1_GO;                                /* clear go */
if (fnc >= FNC_XFER) update_tucs (CS1_DONE, drv);       /* data xfer? */
else update_tucs (CS1_SC, drv);                         /* no, set attn */
if (DEBUG_PRS (tu_dev)) fprintf (sim_deb,
    ">>TU%d DONE: fnc=%s, cs1=%06o, cs2=%06o, ba=%06o, wc=%06o, fc=%06o, fs=%06o, er=%06o, pos=%d\n",
    drv, tu_fname[fnc], tucs1, tucs2, tuba, tuwc, tufc, tufs, tuer, uptr->pos);
return SCPE_OK;
}

/* Formatter error */

void set_tuer (int32 flag)
{
tuer = tuer | flag;
tufs = tufs | FS_ATA;
tucs1 = tucs1 | CS1_SC;
return;
}

/* Controller status update

   Check for done transition
   Update drive status
   Update MTCS1
   Update interrupt request
*/

void update_tucs (int32 flag, int32 drv)
{
int32 act = sim_is_active (&tu_unit[drv]);

if ((flag & ~tucs1) & CS1_DONE)                         /* DONE 0 to 1? */
    tuiff = (tucs1 & CS1_IE)? 1: 0;                     /* CSTB INTR <- IE */
if (GET_FMTR (tucs2) == 0) {                            /* formatter present? */
    tufs = (tufs & ~FS_DYN) | FS_FPR;
    if (tu_unit[drv].flags & UNIT_ATT) {
        tufs = tufs | FS_MOL | tu_unit[drv].USTAT;
        if (tu_unit[drv].UDENS == TC_1600) tufs = tufs | FS_PE;
        if (sim_tape_wrp (&tu_unit[drv])) tufs = tufs | FS_WRL;
        if (!act) {
            if (sim_tape_bot (&tu_unit[drv])) tufs = tufs | FS_BOT;
            if (sim_tape_eot (&tu_unit[drv])) tufs = tufs | FS_EOT;
            }
        }
    if (tuer) tufs = tufs | FS_ERR;
    }
else tufs = 0;
tucs1 = (tucs1 & ~(CS1_SC | CS1_MCPE | CS1_MBZ)) | CS1_DVA | flag;
if (tucs2 & CS2_ERR) tucs1 = tucs1 | CS1_TRE | CS1_SC;
else if (tucs1 & CS1_TRE) tucs1 = tucs1 | CS1_SC;
if (tufs & FS_ATA) tucs1 = tucs1 | CS1_SC;
if (tuiff || ((tucs1 & CS1_SC) && (tucs1 & CS1_DONE) && (tucs1 & CS1_IE)))
    int_req = int_req | INT_TU;
else int_req = int_req & ~INT_TU;
if ((tucs1 & CS1_DONE) && tufs && !act) tufs = tufs | FS_RDY;
return;
}

/* Interrupt acknowledge */

int32 tu_inta (void)
{
tucs1 = tucs1 & ~CS1_IE;                                /* clear int enable */
tuiff = 0;                                              /* clear CSTB INTR */
return VEC_TU;                                          /* acknowledge */
}

/* Map tape error status */

t_stat tu_map_err (UNIT *uptr, t_stat st, t_bool qdt)
{
switch (st) {

    case MTSE_FMT:                                      /* illegal fmt */
    case MTSE_UNATT:                                    /* not attached */
        set_tuer (ER_NXF);                              /* can't execute */
        if (qdt) tucs1 = tucs1 | CS1_TRE;               /* data xfr? set TRE */
    case MTSE_OK:                                       /* no error */
        return SCPE_IERR;

    case MTSE_TMK:                                      /* end of file */
        set_tuer (ER_FCE);                              /* also sets FCE */
        tufs = tufs | FS_TMK;
        break;

    case MTSE_IOERR:                                    /* IO error */
        set_tuer (ER_VPE);                              /* flag error */
        if (qdt) tucs1 = tucs1 | CS1_TRE;               /* data xfr? set TRE */
        if (tu_stopioe) return SCPE_IOERR;
        break;

    case MTSE_INVRL:                                    /* invalid rec lnt */
        set_tuer (ER_VPE);                              /* flag error */
        if (qdt) tucs1 = tucs1 | CS1_TRE;               /* data xfr? set TRE */
        return SCPE_MTRLNT;

    case MTSE_RECE:                                     /* record in error */
        set_tuer (ER_CRC);                              /* set crc err */
        if (qdt) tucs1 = tucs1 | CS1_TRE;               /* data xfr? set TRE */
        break;

    case MTSE_EOM:                                      /* end of medium */
        set_tuer (ER_OPI);                              /* incomplete */
        if (qdt) tucs1 = tucs1 | CS1_TRE;               /* data xfr? set TRE */
        break;

    case MTSE_BOT:                                      /* reverse into BOT */
        break;

    case MTSE_WRP:                                      /* write protect */
        set_tuer (ER_NXF);                              /* can't execute */
        if (qdt) tucs1 = tucs1 | CS1_TRE;               /* data xfr? set TRE */
        break;
        }

return SCPE_OK;
}

/* Reset routine */

t_stat tu_reset (DEVICE *dptr)
{
int32 u;
UNIT *uptr;

tucs1 = CS1_DVA | CS1_DONE;
tucs2 = CS2_IR | CS2_OR;
tuba = 0;
tuwc = 0;
tufc = 0;
tuer = 0;
tufs = FS_FPR | FS_RDY;
if (sim_switches & SWMASK ('P')) tutc = 0;              /* powerup? clr TC */
else tutc = tutc & ~TC_FCS;                             /* no, clr <fcs> */
tuiff = 0;                                              /* clear CSTB INTR */
int_req = int_req & ~INT_TU;                            /* clear interrupt */
for (u = 0; u < TU_NUMDR; u++) {                        /* loop thru units */
    uptr = tu_dev.units + u;
    sim_tape_reset (uptr);                              /* clear pos flag */
    sim_cancel (uptr);                                  /* cancel activity */
    uptr->USTAT = 0;
    }
if (xbuf == NULL) xbuf = (uint8 *) calloc (MT_MAXFR + 4, sizeof (uint8));
if (xbuf == NULL) return SCPE_MEM;
return SCPE_OK;
}

/* Attach routine */

t_stat tu_attach (UNIT *uptr, char *cptr)
{
int32 drv = uptr - tu_dev.units;
t_stat r;

r = sim_tape_attach (uptr, cptr);
if (r != SCPE_OK) return r;
uptr->USTAT = 0;                                        /* clear unit status */
uptr->UDENS = UD_UNK;                                   /* unknown density */
tufs = tufs | FS_ATA | FS_SSC;                          /* set attention */
if ((GET_FMTR (tucs2) == 0) && (GET_DRV (tutc) == drv)) /* selected drive? */
    tufs = tufs | FS_SAT;                               /* set slave attn */
update_tucs (CS1_SC, drv);                              /* update status */
return r;
}

/* Detach routine */

t_stat tu_detach (UNIT* uptr)
{
int32 drv = uptr - tu_dev.units;

if (!(uptr->flags & UNIT_ATT)) return SCPE_OK;          /* attached? */
if (sim_is_active (uptr)) {                             /* unit active? */
    sim_cancel (uptr);                                  /* cancel operation */
    tuer = tuer | ER_UNS;                               /* set formatter error */
    if ((uptr->USTAT & FS_REW) == 0)                    /* data transfer? */
        tucs1 = tucs1 | CS1_DONE | CS1_TRE;             /* set done, err */
    }
uptr->USTAT = 0;                                        /* clear status flags */
tufs = tufs | FS_ATA | FS_SSC;                          /* set attention */
update_tucs (CS1_SC, drv);                              /* update status */
return sim_tape_detach (uptr);
}

/* Device bootstrap */

#define BOOT_START      0377000                         /* start */
#define BOOT_LEN (sizeof (boot_rom_dec) / sizeof (d10))

static const d10 boot_rom_dec[] = {
    0515040000003,                          /* boot:hrlzi 1,3       ; uba # */
    0201000040001,                          /*      movei 0,40001   ; vld,pg 1 */
    0713001000000+(IOBA_UBMAP+1 & RMASK),   /*      wrio 0,763001(1); set ubmap */
    0435040000000+(IOBA_TU & RMASK),        /*      iori 1,772440   ; rh addr */
    0202040000000+FE_RHBASE,                /*      movem 1,FE_RHBASE */
    0201000000040,                          /*      movei 0,40      ; ctrl reset */
    0713001000010,                          /*      wrio 0,10(1)    ; ->MTFS */
    0201100000031,                          /*      movei 2,31      ; space f */
    0265740377014,                          /*      jsp 17,tpop     ; skip ucode */
    0201100000071,                          /*      movei 2,71      ; read f */
    0265740377014,                          /*      jsp 17,tpop     ; read boot */
    0254000001000,                          /*      jrst 1000       ; start */
    0200000000000+FE_MTFMT,                 /* tpop:move 0,FE_MTFMT ; den,fmt,slv */
    0713001000032,                          /*      wrio 0,32(1)    ; ->MTTC */
    0201000000011,                          /*      movei 0,11      ; clr+go */
    0713001000000,                          /*      wrio 0,0(1)     ; ->MTCS1 */
    0201140176000,                          /*      movei 3,176000  ; wd cnt */
    0201200004000,                          /*      movei 4,4000    ; addr */
    0200240000000+FE_MTFMT,                 /*      move 5,FE_MTFMT ; unit */
    0201300000000,                          /*      movei 6,0       ; fmtr */
    0713141000002,                          /*      wrio 3,2(1)     ; ->MTWC */
    0713201000004,                          /*      wrio 4,4(1)     ; ->MTBA */
    0713301000006,                          /*      wrio 6,6(1)     ; ->MTFC */
    0713301000010,                          /*      wrio 6,10(1)    ; ->MTFS */
    0713241000032,                          /*      wrio 5,32(1)    ; ->MTTC */
    0713101000000,                          /*      wrio 2,0(1)     ; ->MTCS1 */
    0712341000012,                          /*      rdio 7,12(1)    ; read FS */
    0606340000200,                          /*      trnn 7,200      ; test rdy */
    0254000377032,                          /*      jrst .-2        ; loop */
    0606340040000,                          /*      trnn 7,40000    ; test err */
    0254017000000,                          /*      jrst 0(17)      ; return */
    0712341000014,                          /*      rdio 7,14(1)    ; read err */
    0302340001000,                          /*      caie 7,1000     ; fce? */
    0254200377052,                          /*      halt */
    0254017000000,                          /*      jrst 0(17)      ; return */
    };

static const d10 boot_rom_its[] = {
    0515040000003,                          /* boot:hrlzi 1,3       ; uba # - not used */
    0201000040001,                          /*      movei 0,40001   ; vld,pg 1 */
    0714000000000+(IOBA_UBMAP+1 & RMASK),   /*      iowri 0,763001  ; set ubmap */
    0435040000000+(IOBA_TU & RMASK),        /*      iori 1,772440   ; rh addr */
    0202040000000+FE_RHBASE,                /*      movem 1,FE_RHBASE */
    0201000000040,                          /*      movei 0,40      ; ctrl reset */
    0714001000010,                          /*      iowri 0,10(1)   ; ->MTFS */
    0201100000031,                          /*      movei 2,31      ; space f */
    0265740377014,                          /*      jsp 17,tpop     ; skip ucode */
    0201100000071,                          /*      movei 2,71      ; read f */
    0265740377014,                          /*      jsp 17,tpop     ; read boot */
    0254000001000,                          /*      jrst 1000       ; start */
    0200000000000+FE_MTFMT,                 /* tpop:move 0,FE_MTFMT ; den,fmt,slv */
    0714001000032,                          /*      iowri 0,32(1)   ; ->MTTC */
    0201000000011,                          /*      movei 0,11      ; clr+go */
    0714001000000,                          /*      iowri 0,0(1)    ; ->MTCS1 */
    0201140176000,                          /*      movei 3,176000  ; wd cnt */
    0201200004000,                          /*      movei 4,4000    ; addr */
    0200240000000+FE_MTFMT,                 /*      move 5,FE_MTFMT ; unit */
    0201300000000,                          /*      movei 6,0       ; fmtr */
    0714141000002,                          /*      iowri 3,2(1)    ; ->MTWC */
    0714201000004,                          /*      iowri 4,4(1)    ; ->MTBA */
    0714301000006,                          /*      iowri 6,6(1)    ; ->MTFC */
    0714301000010,                          /*      iowri 6,10(1)   ; ->MTFS */
    0714241000032,                          /*      iowri 5,32(1)   ; ->MTTC */
    0714101000000,                          /*      iowri 2,0(1)    ; ->MTCS1 */
    0710341000012,                          /*      iordi 7,12(1)   ; read FS */
    0606340000200,                          /*      trnn 7,200      ; test rdy */
    0254000377032,                          /*      jrst .-2        ; loop */
    0606340040000,                          /*      trnn 7,40000    ; test err */
    0254017000000,                          /*      jrst 0(17)      ; return */
    0710341000014,                          /*      iordi 7,14(1)   ; read err */
    0302340001000,                          /*      caie 7,1000     ; fce? */
    0254200377052,                          /*      halt */
    0254017000000,                          /*      jrst 0(17)      ; return */
    };

t_stat tu_boot (int32 unitno, DEVICE *dptr)
{
int32 i;
extern a10 saved_PC;

M[FE_UNIT] = 0;
M[FE_MTFMT] = (unitno & TC_M_UNIT) | (TC_1600 << TC_V_DEN) | (TC_10C << TC_V_FMT);
tu_unit[unitno].pos = 0;
for (i = 0; i < BOOT_LEN; i++)
    M[BOOT_START + i] = ITS? boot_rom_its[i]: boot_rom_dec[i];
saved_PC = BOOT_START;
return SCPE_OK;
}
