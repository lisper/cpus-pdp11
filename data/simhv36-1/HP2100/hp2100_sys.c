/* hp2100_sys.c: HP 2100 simulator interface

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

   19-Nov-04    JDB     Added STOP_OFFLINE, STOP_PWROFF messages
   25-Sep-04    JDB     Added memory protect device
                        Fixed display of CCA/CCB/CCE instructions
   01-Jun-04    RMS     Added latent 13037 support
   19-Apr-04    RMS     Recognize SFS x,C and SFC x,C
   22-Mar-02    RMS     Revised for dynamically allocated memory
   14-Feb-02    RMS     Added DMS instructions
   04-Feb-02    RMS     Fixed bugs in alter/skip display and parsing
   01-Feb-02    RMS     Added terminal multiplexor support
   16-Jan-02    RMS     Added additional device support
   17-Sep-01    RMS     Removed multiconsole support
   27-May-01    RMS     Added multiconsole support
   14-Mar-01    RMS     Revised load/dump interface (again)
   30-Oct-00    RMS     Added examine to file support
   15-Oct-00    RMS     Added dynamic device number support
   27-Oct-98    RMS     V2.4 load interface
*/

#include "hp2100_defs.h"
#include <ctype.h>

extern DEVICE cpu_dev;
extern UNIT cpu_unit;
extern DEVICE mp_dev;
extern DEVICE dma0_dev, dma1_dev;
extern DEVICE ptr_dev, ptp_dev;
extern DEVICE tty_dev, clk_dev;
extern DEVICE lps_dev, lpt_dev;
extern DEVICE mtd_dev, mtc_dev;
extern DEVICE msd_dev, msc_dev;
extern DEVICE dpd_dev, dpc_dev;
extern DEVICE dqd_dev, dqc_dev;
extern DEVICE drd_dev, drc_dev;
extern DEVICE ds_dev;
extern DEVICE muxl_dev, muxu_dev, muxc_dev;
extern DEVICE ipli_dev, iplo_dev;
extern REG cpu_reg[];
extern uint16 *M;

/* SCP data structures and interface routines

   sim_name             simulator name string
   sim_PC               pointer to saved PC register descriptor
   sim_emax             maximum number of words for examine/deposit
   sim_devices          array of pointers to simulated devices
   sim_stop_messages    array of pointers to stop messages
   sim_load             binary loader
*/

char sim_name[] = "HP 2100";

char halt_msg[] = "HALT instruction xxxxxx";

REG *sim_PC = &cpu_reg[0];

int32 sim_emax = 3;

DEVICE *sim_devices[] = {
    &cpu_dev,
    &mp_dev,
    &dma0_dev,
    &dma1_dev,
    &ptr_dev,
    &ptp_dev,
    &tty_dev,
    &clk_dev,
    &lps_dev,
    &lpt_dev,
    &dpd_dev, &dpc_dev,
    &dqd_dev, &dqc_dev,
    &drd_dev, &drc_dev,
    &ds_dev,
    &mtd_dev, &mtc_dev,
    &msd_dev, &msc_dev,
    &muxl_dev, &muxu_dev, &muxc_dev,
    &ipli_dev, &iplo_dev,
    NULL
    };

const char *sim_stop_messages[] = {
    "Unknown error",
    "Unimplemented instruction",
    "Non-existent I/O device",
    halt_msg,
    "Breakpoint",
    "Indirect address loop",
    "Indirect address interrupt (should not happen!)",
    "No connection on interprocessor link",
    "Device/unit offline",
    "Device/unit powered off"
    };

/* Binary loader

   The binary loader consists of blocks preceded and trailed by zero frames.
   A block consists of 16b words (punched big endian), as follows:

        count'xxx
        origin
        word 0
        :
        word count-1
        checksum

   The checksum includes the origin but not the count.   
*/

int32 fgetw (FILE *fileref)
{
int c1, c2;

if ((c1 = fgetc (fileref)) == EOF) return -1;
if ((c2 = fgetc (fileref)) == EOF) return -1;
return ((c1 & 0377) << 8) | (c2 & 0377);
}

t_stat sim_load (FILE *fileref, char *cptr, char *fnam, int flag)
{
int32 origin, csum, zerocnt, count, word, i;

if ((*cptr != 0) || (flag != 0)) return SCPE_ARG;
for (zerocnt = 1;; zerocnt = -10) {                     /* block loop */
    for (;; zerocnt++) {                                /* skip 0's */
        if ((count = fgetc (fileref)) == EOF) return SCPE_OK;
        else if (count) break;
        else if (zerocnt == 0) return SCPE_OK;
        }
    if (fgetc (fileref) == EOF) return SCPE_FMT;
    if ((origin = fgetw (fileref)) < 0) return SCPE_FMT;
    csum = origin;                                      /* seed checksum */
    for (i = 0; i < count; i++) {                       /* get data words */
        if ((word = fgetw (fileref)) < 0) return SCPE_FMT;
        if (MEM_ADDR_OK (origin)) M[origin] = word;
        origin = origin + 1;
        csum = csum + word;
        }
    if ((word = fgetw (fileref)) < 0) return SCPE_FMT;
    if ((word ^ csum) & DMASK) return SCPE_CSUM;
    }
}

/* Symbol tables */

#define I_V_FL          16                              /* flag start */
#define I_M_FL          017                             /* flag mask */
#define I_V_NPN         0                               /* no operand */
#define I_V_NPC         1                               /* no operand + C */
#define I_V_MRF         2                               /* mem ref */
#define I_V_ASH         3                               /* alter/skip, shift */
#define I_V_ESH         4                               /* extended shift */
#define I_V_EMR         5                               /* extended mem ref */
#define I_V_IO1         6                               /* I/O + HC */
#define I_V_IO2         7                               /* I/O only */
#define I_V_EGZ         010                             /* ext grp, 1 op + 0 */
#define I_V_EG2         011                             /* ext grp, 2 op */
#define I_NPN           (I_V_NPN << I_V_FL)
#define I_NPC           (I_V_NPC << I_V_FL)
#define I_MRF           (I_V_MRF << I_V_FL)
#define I_ASH           (I_V_ASH << I_V_FL)
#define I_ESH           (I_V_ESH << I_V_FL)
#define I_EMR           (I_V_EMR << I_V_FL)
#define I_IO1           (I_V_IO1 << I_V_FL)
#define I_IO2           (I_V_IO2 << I_V_FL)
#define I_EGZ           (I_V_EGZ << I_V_FL)
#define I_EG2           (I_V_EG2 << I_V_FL)

static const int32 masks[] = {
 0177777, 0176777, 0074000, 0170000,
 0177760, 0177777, 0176700, 0177700,
 0177777, 0177777
 };

static const char *opcode[] = {
 "NOP", "NOP", "AND", "JSB",
 "XOR", "JMP", "IOR", "ISZ",
 "ADA", "ADB" ,"CPA", "CPB",
 "LDA", "LDB", "STA", "STB",
 "DIAG", "ASL", "LSL", "TIMER",
 "RRL", "ASR", "LSR", "RRR",
 "MPY", "DIV", "DLD", "DST",
 "FAD", "FSB", "FMP", "FDV",
 "FIX", "FLT",
 "STO", "CLO", "SOC", "SOS",
 "HLT", "STF", "CLF",
 "SFC", "SFS", "MIA", "MIB",
 "LIA", "LIB", "OTA", "OTB",
 "STC", "CLC",
 "SYA", "USA", "PAA", "PBA",
               "XMA",
 "XLA", "XSA", "XCA", "LFA",
 "RSA", "RVA",
               "MBI", "MBF",
 "MBW", "MWI", "MWF", "MWW",
 "SYB", "USB", "PAB", "PBB",
 "SSM", "JRS",
 "XMM", "XMS", "XMB",
 "XLB", "XSB", "XCB", "LFB",
 "RSB", "RVB", "DJP", "DJS",
 "SJP", "SJS", "UJP", "UJS",
 "SAX", "SBX", "CAX", "CBX",
 "LAX", "LBX", "STX",
 "CXA", "CXB", "LDX",
 "ADX", "XAX", "XBX",
 "SAY", "SBY", "CAY", "CBY",
 "LAY", "LBY", "STY",
 "CYA", "CYB", "LDY",
 "ADY", "XAY", "XBY",
 "ISX", "DSX", "JLY", "LBT",
 "SBT", "MBT", "CBT", "SBT",
 "ISY", "DSY", "JPY", "SBS",
 "CBS", "TBS", "CMW", "MVW",
 NULL,                                                  /* decode only */
 NULL
 };

static const int32 opc_val[] = {
 0000000+I_NPN, 0002000+I_NPN, 0010000+I_MRF, 0014000+I_MRF,
 0020000+I_MRF, 0024000+I_MRF, 0030000+I_MRF, 0034000+I_MRF,
 0040000+I_MRF, 0044000+I_MRF, 0050000+I_MRF, 0054000+I_MRF,
 0060000+I_MRF, 0064000+I_MRF, 0070000+I_MRF, 0074000+I_MRF,
 0100000+I_NPN, 0100020+I_ESH, 0100040+I_ESH, 0100060+I_NPN,
 0100100+I_ESH, 0101020+I_ESH, 0101040+I_ESH, 0101100+I_ESH,
 0100200+I_EMR, 0100400+I_EMR, 0104200+I_EMR, 0104400+I_EMR,
 0105000+I_EMR, 0105020+I_EMR, 0105040+I_EMR, 0105060+I_EMR,
 0105100+I_NPN, 0105120+I_NPN,
 0102101+I_NPN, 0103101+I_NPN, 0102201+I_NPC, 0102301+I_NPC,
 0102000+I_IO1, 0102100+I_IO2, 0103100+I_IO2,
 0102200+I_IO1, 0102300+I_IO1, 0102400+I_IO1, 0106400+I_IO1,
 0102500+I_IO1, 0106500+I_IO1, 0102600+I_IO1, 0106600+I_IO1,
 0102700+I_IO1, 0106700+I_IO1,
 0101710+I_NPN, 0101711+I_NPN, 0101712+I_NPN, 0101713+I_NPN,
                               0101722+I_NPN,
 0101724+I_EMR, 0101725+I_EMR, 0101726+I_EMR, 0101727+I_NPN,
 0101730+I_NPN, 0101731+I_NPN,
                               0105702+I_NPN, 0105703+I_NPN,
 0105704+I_NPN, 0105705+I_NPN, 0105706+I_NPN, 0105707+I_NPN,
 0105710+I_NPN, 0105711+I_NPN, 0105712+I_NPN, 0105713+I_NPN,
 0105714+I_EMR, 0105715+I_EG2,
 0105720+I_NPN, 0105721+I_NPN, 0105722+I_NPN,
 0105724+I_EMR, 0105725+I_EMR, 0105726+I_EMR, 0105727+I_NPN,
 0105730+I_NPN, 0105731+I_NPN, 0105732+I_EMR, 0105733+I_EMR,
 0105734+I_EMR, 0105735+I_EMR, 0105736+I_EMR, 0105737+I_EMR,
 0101740+I_EMR, 0105740+I_EMR, 0101741+I_NPN, 0105741+I_NPN,
 0101742+I_EMR, 0105742+I_EMR, 0105743+I_EMR,
 0101744+I_NPN, 0105744+I_NPN, 0105745+I_EMR,
 0105746+I_EMR, 0101747+I_NPN, 0105747+I_NPN,
 0101750+I_EMR, 0105750+I_EMR, 0101751+I_NPN, 0105751+I_NPN,
 0101752+I_EMR, 0105752+I_EMR, 0105753+I_EMR,
 0101754+I_NPN, 0105754+I_NPN, 0105755+I_EMR,
 0105756+I_EMR, 0101757+I_NPN, 0105757+I_NPN,
 0105760+I_NPN, 0105761+I_NPN, 0105762+I_EMR, 0105763+I_NPN,
 0105764+I_NPN, 0105765+I_EGZ, 0105766+I_EGZ, 0105767+I_NPN,
 0105770+I_NPN, 0105771+I_NPN, 0105772+I_EMR, 0105773+I_EG2,
 0105774+I_EG2, 0105775+I_EG2, 0105776+I_EGZ, 0105777+I_EGZ,
 0000000+I_ASH,                                         /* decode only */
 -1
 };

/* Decode tables for shift and alter/skip groups */

static const char *stab[] = {
 "ALS", "ARS", "RAL", "RAR", "ALR", "ERA", "ELA", "ALF",
 "BLS", "BRS", "RBL", "RBR", "BLR", "ERB", "ELB", "BLF",
 "CLA", "CMA", "CCA", "CLB", "CMB", "CCB",
 "SEZ", "CLE", "CLE", "CME", "CCE",
 "SSA", "SSB", "SLA", "SLB",
 "ALS", "ARS", "RAL", "RAR", "ALR", "ERA", "ELA", "ALF",
 "BLS", "BRS", "RBL", "RBR", "BLR", "ERB", "ELB", "BLF",
 "INA", "INB", "SZA", "SZB", "RSS",
 NULL
 };

static const int32 mtab[] = {
 0007700, 0007700, 0007700, 0007700, 0007700, 0007700, 0007700, 0007700,
 0007700, 0007700, 0007700, 0007700, 0007700, 0007700, 0007700, 0007700,
 0007400, 0007400, 0007400, 0007400, 0007400, 0007400,
 0002040, 0002040, 0002300, 0002300, 0002300,
 0006020, 0006020, 0004010, 0004010,
 0006027, 0006027, 0006027, 0006027, 0006027, 0006027, 0006027, 0006027,
 0006027, 0006027, 0006027, 0006027, 0006027, 0006027, 0006027, 0006027,
 0006004, 0006004, 0006002, 0006002, 0002001,
 0
 };

static const int32 vtab[] = {
 0001000, 0001100, 0001200, 0001300, 0001400, 0001500, 0001600, 0001700,
 0005000, 0005100, 0005200, 0005300, 0005400, 0005500, 0005600, 0005700,
 0002400, 0003000, 0003400, 0006400, 0007000, 0007400,
 0002040, 0000040, 0002100, 0002200, 0002300,
 0002020, 0006020, 0000010, 0004010,
 0000020, 0000021, 0000022, 0000023, 0000024, 0000025, 0000026, 0000027,
 0004020, 0004021, 0004022, 0004023, 0004024, 0004025, 0004026, 0004027,
 0002004, 0006004, 0002002, 0006002, 0002001,
 -1
 };

/* Symbolic decode

   Inputs:
        *of     =       output stream
        addr    =       current PC
        *val    =       pointer to data
        *uptr   =       pointer to unit 
        sw      =       switches
   Outputs:
        return  =       status code
*/

#define FMTASC(x) ((x) < 040)? "<%03o>": "%c", (x)

t_stat fprint_sym (FILE *of, t_addr addr, t_value *val,
    UNIT *uptr, int32 sw)
{
int32 cflag, cm, i, j, inst, disp;

cflag = (uptr == NULL) || (uptr == &cpu_unit);
inst = val[0];
if (sw & SWMASK ('A')) {                                /* ASCII? */
    if (inst > 0377) return SCPE_ARG;
    fprintf (of, FMTASC (inst & 0177));
    return SCPE_OK;
    }
if (sw & SWMASK ('C')) {                                /* characters? */
    fprintf (of, FMTASC ((inst >> 8) & 0177));
    fprintf (of, FMTASC (inst & 0177));
    return SCPE_OK;
    }
if (!(sw & SWMASK ('M'))) return SCPE_ARG;

for (i = 0; opc_val[i] >= 0; i++) {                     /* loop thru ops */
    j = (opc_val[i] >> I_V_FL) & I_M_FL;                /* get class */
    if ((opc_val[i] & DMASK) == (inst & masks[j])) {    /* match? */
        switch (j) {                                    /* case on class */

        case I_V_NPN:                                   /* no operands */
            fprintf (of, "%s", opcode[i]);              /* opcode */
            break;

        case I_V_NPC:                                   /* no operands + C */
            fprintf (of, "%s", opcode[i]);
            if (inst & I_HC) fprintf (of, " C");
            break;

        case I_V_MRF:                                   /* mem ref */
            disp = inst & I_DISP;                       /* displacement */
            fprintf (of, "%s ", opcode[i]);             /* opcode */
            if (inst & I_CP) {                          /* current page? */
                if (cflag) fprintf (of, "%-o", (addr & I_PAGENO) | disp);
                else fprintf (of, "C %-o", disp);
                }
            else fprintf (of, "%-o", disp);             /* page zero */
            if (inst & I_IA) fprintf (of, ",I");
            break;

        case I_V_ASH:                                   /* shift, alter-skip */
            cm = FALSE;
            for (i = 0; mtab[i] != 0; i++) {
                if ((inst & mtab[i]) == vtab[i]) {
                    inst = inst & ~(vtab[i] & 01777);
                    if (cm) fprintf (of, ",");
                    cm = TRUE;
                    fprintf (of, "%s", stab[i]);
                    }
                }       
            if (!cm) return SCPE_ARG;                   /* nothing decoded? */
            break;

        case I_V_ESH:                                   /* extended shift */
            disp = inst & 017;                          /* shift count */
            if (disp == 0) disp = 16;
            fprintf (of, "%s %d", opcode[i], disp);
            break;

        case I_V_EMR:                                   /* extended mem ref */
            fprintf (of, "%s %-o", opcode[i], val[1] & VAMASK);
            if (val[1] & I_IA) fprintf (of, ",I");
            return -1;                                  /* extra word */

        case I_V_IO1:                                   /* IOT with H/C */
            fprintf (of, "%s %-o", opcode[i], inst & I_DEVMASK);
            if (inst & I_HC) fprintf (of, ",C");
            break;

        case I_V_IO2:                                   /* IOT */
            fprintf (of, "%s %-o", opcode[i], inst & I_DEVMASK);
            break;

        case I_V_EGZ:                                   /* ext grp 1 op + 0 */
            fprintf (of, "%s %-o", opcode[i], val[1] & VAMASK);
            if (val[1] & I_IA) fprintf (of, ",I");
            return -2;                                  /* extra words */

        case I_V_EG2:                                   /* ext grp 2 op */
            fprintf (of, "%s %-o", opcode[i], val[1] & VAMASK);
            if (val[1] & I_IA) fprintf (of, ",I");
            fprintf (of, " %-o", val[2] & VAMASK);
            if (val[2] & I_IA) fprintf (of, ",I");
            return -2;                                  /* extra words */
            }
        return SCPE_OK;
        }                                               /* end if */
    }                                                   /* end for */
return SCPE_ARG;
}

/* Get address with indirection

   Inputs:
        *cptr   =       pointer to input string
   Outputs:
        val     =       address
                        -1 if error
*/

int32 get_addr (char *cptr)
{
int32 d;
t_stat r;
char gbuf[CBUFSIZE];

cptr = get_glyph (cptr, gbuf, ',');                     /* get next field */
d = get_uint (gbuf, 8, VAMASK, &r);                     /* construe as addr */
if (r != SCPE_OK) return -1;
if (*cptr != 0) {                                       /* more? */
    cptr = get_glyph (cptr, gbuf, 0);                   /* look for indirect */
    if (*cptr != 0) return -1;                          /* should be done */
    if (strcmp (gbuf, "I")) return -1;                  /* I? */
    d = d | I_IA;
    }
return d;
}

/* Symbolic input

   Inputs:
        *iptr   =       pointer to input string
        addr    =       current PC
        *uptr   =       pointer to unit
        *val    =       pointer to output values
        sw      =       switches
   Outputs:
        status  =       error status
*/

t_stat parse_sym (char *iptr, t_addr addr, UNIT *uptr, t_value *val, int32 sw)
{
int32 cflag, d, i, j, k, clef, tbits;
t_stat r, ret;
char *cptr, gbuf[CBUFSIZE];

cflag = (uptr == NULL) || (uptr == &cpu_unit);
while (isspace (*iptr)) iptr++;                         /* absorb spaces */
if ((sw & SWMASK ('A')) || ((*iptr == '\'') && iptr++)) { /* ASCII char? */
    if (iptr[0] == 0) return SCPE_ARG;                  /* must have 1 char */
    val[0] = (t_value) iptr[0] & 0177;
    return SCPE_OK;
    }
if ((sw & SWMASK ('C')) || ((*iptr == '"') && iptr++)) { /* char string? */
    if (iptr[0] == 0) return SCPE_ARG;                  /* must have 1 char */
    val[0] = (((t_value) iptr[0] & 0177) << 8) |
              ((t_value) iptr[1] & 0177);
    return SCPE_OK;
    }

ret = SCPE_OK;
cptr = get_glyph (iptr, gbuf, 0);                       /* get opcode */
for (i = 0; (opcode[i] != NULL) && (strcmp (opcode[i], gbuf) != 0) ; i++) ;
if (opcode[i]) {                                        /* found opcode? */
    val[0] = opc_val[i] & DMASK;                        /* get value */
    j = (opc_val[i] >> I_V_FL) & I_M_FL;                /* get class */
    switch (j) {                                        /* case on class */

    case I_V_NPN:                                       /* no operand */
        break;

    case I_V_NPC:                                       /* no operand + C */
        if (*cptr != 0) {
            cptr = get_glyph (cptr, gbuf, 0);
            if (strcmp (gbuf, "C")) return SCPE_ARG;
            val[0] = val[0] | I_HC;
			}
        break;

    case I_V_MRF:                                       /* mem ref */
        cptr = get_glyph (cptr, gbuf, 0);               /* get next field */
        if (k = (strcmp (gbuf, "C") == 0)) {            /* C specified? */
            val[0] = val[0] | I_CP;
            cptr = get_glyph (cptr, gbuf, 0);
            }
        else if (k = (strcmp (gbuf, "Z") == 0)) {       /* Z specified? */
            cptr = get_glyph (cptr, gbuf, ',');
            }
        if ((d = get_addr (gbuf)) < 0) return SCPE_ARG;
        if ((d & VAMASK) <= I_DISP) val[0] = val[0] | d;
        else if (cflag && !k && (((addr ^ d) & I_PAGENO) == 0))
            val[0] = val[0] | (d & (I_IA | I_DISP)) | I_CP;
        else return SCPE_ARG;
        break;

    case I_V_ESH:                                       /* extended shift */
        cptr = get_glyph (cptr, gbuf, 0);
        d = get_uint (gbuf, 10, 16, &r);
        if ((r != SCPE_OK) || (d == 0)) return SCPE_ARG;
        val[0] = val[0] | (d & 017);
        break;

    case I_V_EMR:                                       /* extended mem ref */
        cptr = get_glyph (cptr, gbuf, 0);               /* get next field */
        if ((d = get_addr (gbuf)) < 0) return SCPE_ARG;
        val[1] = d;
        ret = -1;
        break;

    case I_V_IO1:                                       /* IOT + optional C */
        cptr = get_glyph (cptr, gbuf, ',');             /* get device */
        d = get_uint (gbuf, 8, I_DEVMASK, &r);
        if (r != SCPE_OK) return SCPE_ARG;
        val[0] = val[0] | d;
        if (*cptr != 0) {
            cptr = get_glyph (cptr, gbuf, 0);
            if (strcmp (gbuf, "C")) return SCPE_ARG;
            val[0] = val[0] | I_HC;
            }
        break;

    case I_V_IO2:                                       /* IOT */
        cptr = get_glyph (cptr, gbuf, 0);               /* get device */
        d = get_uint (gbuf, 8, I_DEVMASK, &r);
        if (r != SCPE_OK) return SCPE_ARG;
        val[0] = val[0] | d;
        break;

    case I_V_EGZ:                                       /* ext grp 1 op + 0 */
        cptr = get_glyph (cptr, gbuf, 0);               /* get next field */
        if ((d = get_addr (gbuf)) < 0) return SCPE_ARG;
        val[1] = d;
        val[2] = 0;
        ret = -2;
        break;

    case I_V_EG2:                                       /* ext grp 2 op */
        cptr = get_glyph (cptr, gbuf, 0);               /* get next field */
        if ((d = get_addr (gbuf)) < 0) return SCPE_ARG;
        cptr = get_glyph (cptr, gbuf, 0);               /* get next field */
        if ((k = get_addr (gbuf)) < 0) return SCPE_ARG;
        val[1] = d;
        val[2] = k;
        ret = -2;
        break;
        }                                               /* end case */

    if (*cptr != 0) return SCPE_ARG;                    /* junk at end? */
    return ret;
    }                                                   /* end if opcode */

/* Shift or alter-skip

   Each opcode is matched by a mask, specifiying the bits affected, and
   the value, specifying the value.  As opcodes are processed, the mask
   values are used to specify which fields have already been filled in.

   The mask has two subfields, the type bits (A/B and A/S), and the field
   bits.  The type bits, once specified by any instruction, must be
   consistent in all other instructions.  The mask bits assure that no
   field is filled in twice.

   Two special cases:

   1. The dual shift field in shift requires checking how much of the
      target word has been filled in before assigning the shift value.
      To implement this, shifts are listed twice is the decode table.
      If the current subopcode is a shift in the first part of the table
      (entries 0..15), and CLE has been seen or the first shift field is
      filled in, the code forces a mismatch.  The glyph will match in
      the second part of the table.

   2. CLE processing must be deferred until the instruction can be
      classified as shift or alter-skip, since it has two different
      bit values in the two classes.  To implement this, CLE seen is
      recorded as a flag and processed after all other subopcodes.
*/

clef = FALSE;
tbits = 0;
val[0] = 0;
for (cptr = get_glyph (iptr, gbuf, ','); gbuf[0] != 0;
     cptr = get_glyph (cptr, gbuf, ',')) {              /* loop thru glyphs */
    if (strcmp (gbuf, "CLE") == 0) {                    /* CLE? */
        if (clef) return SCPE_ARG;                      /* already seen? */
        clef = TRUE;                                    /* set flag */
        continue;
        }
    for (i = 0; stab[i] != NULL; i++) {                 /* find subopcode */
        if ((strcmp (gbuf, stab[i]) == 0) &&
            ((i >= 16) || (!clef && ((val[0] & 001710) == 0)))) break;
        } 
    if (stab[i] == NULL) return SCPE_ARG;
    if (tbits & mtab[i] & (I_AB | I_ASKP) & (vtab[i] ^ val[0]))
        return SCPE_ARG;
    if (tbits & mtab[i] & ~(I_AB | I_ASKP)) return SCPE_ARG;
    tbits = tbits | mtab[i];                            /* fill type+mask */
    val[0] = val[0] | vtab[i];                          /* fill value */
    }
if (clef) {                                             /* CLE seen? */
    if (val[0] & I_ASKP) {                              /* alter-skip? */
        if (tbits & 0100) return SCPE_ARG;              /* already filled in? */
        else val[0] = val[0] | 0100;
        }
    else val[0] = val[0] | 040;                         /* fill in shift */
    }
return ret;
}
