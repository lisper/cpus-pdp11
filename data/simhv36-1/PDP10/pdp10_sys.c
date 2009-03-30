/* pdp10_sys.c: PDP-10 simulator interface

   Copyright (c) 1993-2005, Robert M Supnik

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

   22-Jul-05    RMS     Fixed warning from Solaris C (from Doug Gwyn)
   09-Jan-03    RMS     Added DEUNA/DELUA support
   12-Sep-02    RMS     Added RX211 support
   22-Apr-02    RMS     Removed magtape record length error
   17-Sep-01    RMS     Removed multiconsole support
   25-Aug-01    RMS     Enabled DZ11
   27-May-01    RMS     Added multiconsole support
   29-Apr-01    RMS     Fixed format for RDPCST, WRPCST
                        Added CLRCSH for ITS
   03-Apr-01    RMS     Added support for loading EXE files
   19-Mar-01    RMS     Added support for loading SAV files
   30-Oct-00    RMS     Added support for examine to file
*/

#include "pdp10_defs.h"
#include <ctype.h>

extern DEVICE cpu_dev, pag_dev;
extern DEVICE tim_dev, fe_dev, uba_dev;
extern DEVICE ptr_dev, ptp_dev;
extern DEVICE rp_dev, tu_dev;
extern DEVICE dz_dev, ry_dev;
extern DEVICE lp20_dev;
extern DEVICE xu_dev;
extern UNIT cpu_unit;
extern REG cpu_reg[];
extern d10 *M;
extern a10 saved_PC;

/* SCP data structures and interface routines

   sim_name             simulator name string
   sim_PC               pointer to saved PC register descriptor
   sim_emax             number of words for examine
   sim_devices          array of pointers to simulated devices
   sim_stop_messages    array of pointers to stop messages
   sim_load             binary loader
*/

char sim_name[] = "PDP-10";

REG *sim_PC = &cpu_reg[0];

int32 sim_emax = 1;

DEVICE *sim_devices[] = { 
    &cpu_dev,
    &pag_dev,
    &tim_dev,
    &fe_dev,
    &uba_dev,
    &ptr_dev,
    &ptp_dev,
    &ry_dev,
    &lp20_dev,
    &rp_dev,
    &tu_dev,
    &dz_dev,
    &xu_dev,
    NULL
    };

const char *sim_stop_messages[] = {
    "Unknown error",
    "HALT instruction",
    "Breakpoint",
    "Illegal instruction",
    "Illegal interrupt instruction",
    "Paging error in interrupt",
    "Zero vector table",
    "NXM on UPT/EPT reference",
    "Nested indirect address limit exceeded",
    "Nested XCT limit exceeded",
    "Invalid I/O controller",
    "Address stop",
    "Panic stop"
     };

/* Binary loader, supports RIM10, SAV, EXE */

#define FMT_R   1                                       /* RIM10 */
#define FMT_S   2                                       /* SAV */
#define FMT_E   3                                       /* EXE */

#define EXE_DIR 01776                                   /* EXE directory */
#define EXE_VEC 01775                                   /* EXE entry vec */
#define EXE_PDV 01774                                   /* EXE ignored */
#define EXE_END 01777                                   /* EXE end

/* RIM10 loader

   RIM10 format is a binary paper tape format (all data frames
   are 200 or greater).  It consists of blocks containing

        -count,,origin-1
        word
        :
        word
        checksum (includes IOWD)
        :
        JRST start
*/

d10 getrimw (FILE *fileref)
{
int32 i, tmp;
d10 word;

word = 0;
for (i = 0; i < 6;) {
    if ((tmp = getc (fileref)) == EOF) return -1;
    if (tmp & 0200) {
        word = (word << 6) | ((d10) tmp & 077);
        i++;
        }
    }
return word;
}

t_stat load_rim (FILE *fileref)
{
d10 count, cksm, data;
a10 pa;
int32 op;

for ( ;; ) {                                            /* loop until JRST */
    count = cksm = getrimw (fileref);                   /* get header */
    if (count < 0) return SCPE_FMT;                     /* read err? */
    if (TSTS (count)) {                                 /* hdr = IOWD? */
        for ( ; TSTS (count); count = AOB (count)) {
            data = getrimw (fileref);                   /* get data wd */
            if (data < 0) return SCPE_FMT;
            cksm = cksm + data;                         /* add to cksm */
            pa = ((a10) count + 1) & AMASK;             /* store */
            M[pa] = data;
            }                                           /* end for */
        data = getrimw (fileref);                       /* get cksm */
        if (data < 0) return SCPE_FMT;
        if ((cksm + data) & DMASK) return SCPE_CSUM;    /* test cksm */
        }                                               /* end if count */
    else {
        op = GET_OP (count);                            /* not IOWD */
        if (op != OP_JRST) return SCPE_FMT;             /* JRST? */
        saved_PC = (a10) count & AMASK;                 /* set PC */
        break;
        }                                               /* end else */
    }                                                   /* end for */
return SCPE_OK;
}

/* SAV file loader

   SAV format is a disk file format (36b words).  It consists of
   blocks containing:

        -count,,origin-1
        word
        :
        word
        :
        JRST start
*/

t_stat load_sav (FILE *fileref)
{
d10 count, data;
a10 pa;
int32 wc, op;

for ( ;; ) {                                            /* loop */
    wc = fxread (&count, sizeof (d10), 1, fileref);     /* read IOWD */
    if (wc == 0) return SCPE_OK;                        /* done? */
    if (TSTS (count)) {                                 /* IOWD? */
        for ( ; TSTS (count); count = AOB (count)) {
            wc = fxread (&data, sizeof (d10), 1, fileref);
            if (wc == 0) return SCPE_FMT;
            pa = ((a10) count + 1) & AMASK;             /* store data */
            M[pa] = data;
            }                                           /* end for */
        }                                               /* end if  count*/
    else {
        op = GET_OP (count);                            /* not IOWD */
        if (op != OP_JRST) return SCPE_FMT;             /* JRST? */
        saved_PC = (a10) count & AMASK;                 /* set PC */
        break;
        }                                               /* end else */
    }                                                   /* end for */
return SCPE_OK;
}

/* EXE file loader

   EXE format is a disk file format (36b words).  It consists of
   blocks containing:

        block type,,total words = n
        n - 1 data words

   Block types are

        EXE_DIR (1776)  directory
        EXE_VEC (1775)  entry vector
        EXE_PDV (1774)  optional blocks
        EXE_END (1777)  end block

   The directory blocks are the most important and contain doubleword
   page loading information:

        word0<0:8>      =       flags
            <9:35>      =       page in file (0 if 0 page)
        word1<0:8>      =       repeat count - 1
            <9:35>      =       page in memory
*/

#define DIRSIZ  (2 * PAG_SIZE)

t_stat load_exe (FILE *fileref)
{
d10 data, dirbuf[DIRSIZ], pagbuf[PAG_SIZE], entbuf[2];
int32 ndir, entvec, i, j, k, cont, bsz, bty, rpt, wc;
int32 fpage, mpage;
a10 ma;

ndir = entvec = 0;                                      /* no dir, entvec */
cont = 1;
do {
    wc = fxread (&data, sizeof (d10), 1, fileref);      /* read blk hdr */
    if (wc == 0) return SCPE_FMT;                       /* error? */
    bsz = (int32) ((data & RMASK) - 1);                 /* get count */
    if (bsz <= 0) return SCPE_FMT;                      /* zero? */
    bty = (int32) LRZ (data);                           /* get type */
    switch (bty) {                                      /* case type */

    case EXE_DIR:                                       /* directory */
        if (ndir) return SCPE_FMT;                      /* got one */
        ndir = fxread (dirbuf, sizeof (d10), bsz, fileref);
        if (ndir < bsz) return SCPE_FMT;                /* error */
        break;

    case EXE_PDV:                                       /* ??? */
        fseek (fileref, bsz * sizeof (d10), SEEK_CUR);
        break;

    case EXE_VEC:                                       /* entry vec */
        if (bsz != 2) return SCPE_FMT;                  /* must be 2 wds */
        entvec = fxread (entbuf, sizeof (d10), bsz, fileref);
        if (entvec < 2) return SCPE_FMT;                /* error? */
        cont = 0;                                       /* stop */
        break;

    case EXE_END:                                       /* end */
        if (bsz != 0) return SCPE_FMT;                  /* must be hdr */
        cont = 0;                                       /* stop */
        break;

    default:
        return SCPE_FMT;
        }                                               /* end switch */
    } while (cont);                                     /* end do */

for (i = 0; i < ndir; i = i + 2) {                      /* loop thru dir */
    fpage = (int32) (dirbuf[i] & RMASK);                /* file page */
    mpage = (int32) (dirbuf[i + 1] & RMASK);            /* memory page */
    rpt = (int32) ((dirbuf[i + 1] >> 27) + 1);          /* repeat count */
    for (j = 0; j < rpt; j++, mpage++) {                /* loop thru rpts */
        if (fpage) {                                    /* file pages? */
            fseek (fileref, (fpage << PAG_V_PN) * sizeof (d10), SEEK_SET);
            wc = fxread (pagbuf, sizeof (d10), PAG_SIZE, fileref);
            if (wc < PAG_SIZE) return SCPE_FMT;
            fpage++;
            }
        ma = mpage << PAG_V_PN;                         /* mem addr */
        for (k = 0; k < PAG_SIZE; k++, ma++) {          /* copy buf to mem */
            if (MEM_ADDR_NXM (ma)) return SCPE_NXM;
            M[ma] = fpage? (pagbuf[k] & DMASK): 0;
            }                                           /* end copy */
        }                                               /* end rpt */
    }                                                   /* end directory */
if (entvec && entbuf[1])
    saved_PC = (int32) entbuf[1] & RMASK;               /* start addr */
return SCPE_OK;
}

/* Master loader */

t_stat sim_load (FILE *fileref, char *cptr, char *fnam, int flag)
{
d10 data;
int32 wc, fmt;
extern int32 sim_switches;

fmt = 0;                                                /* no fmt */
if (sim_switches & SWMASK ('R')) fmt = FMT_R;           /* -r? */
else if (sim_switches & SWMASK ('S')) fmt = FMT_S;      /* -s? */
else if (sim_switches & SWMASK ('E')) fmt = FMT_E;      /* -e? */
else if (match_ext (fnam, "RIM")) fmt = FMT_R;          /* .RIM? */
else if (match_ext (fnam, "SAV")) fmt = FMT_S;          /* .SAV? */
else if (match_ext (fnam, "EXE")) fmt = FMT_E;          /* .EXE? */
else {
    wc = fxread (&data, sizeof (d10), 1, fileref);      /* read hdr */
    if (wc == 0) return SCPE_FMT;                       /* error? */
    if (LRZ (data) == EXE_DIR) fmt = FMT_E;             /* EXE magic? */
    else if (TSTS (data)) fmt = FMT_S;                  /* SAV magic? */
    fseek (fileref, 0, SEEK_SET);                       /* rewind */
    }

switch (fmt) {                                          /* case fmt */

    case FMT_R:                                         /* RIM */
        return load_rim (fileref);

    case FMT_S:                                         /* SAV */
        return load_sav (fileref);

    case FMT_E:                                         /* EXE */
        return load_exe (fileref);
        }

printf ("Can't determine load file format\n");
return SCPE_FMT;
}

/* Symbol tables */

#define I_V_FL          39                              /* inst class */
#define I_M_FL          03                              /* class mask */
#define I_ITS           004000000000000                 /* ITS flag */
#define I_AC            000000000000000                 /* AC, address */
#define I_OP            010000000000000                 /* address only */
#define I_IO            020000000000000                 /* classic I/O */
#define I_V_AC          00
#define I_V_OP          01
#define I_V_IO          02

static const d10 masks[] = {
 0777000000000, 0777740000000,
 0700340000000, 0777777777777
 }; 

static const char *opcode[] = {
"XCTR", "XCTI",                                         /* ITS only */
"IORDI", "IORDQ", "IORD", "IOWR", "IOWRI", "IOWRQ",
"IORDBI", "IORDBQ", "IORDB", "IOWRB", "IOWRBI", "IOWRBQ",
"CLRCSH", "RDPCST", "WRPCST",
"SDBR1", "SDBR2", "SDBR3", "SDBR4", "SPM",
"LDBR1", "LDBR2", "LDBR3", "LDBR4", "LPMR",

"PORTAL", "JRSTF", "HALT",                              /* AC defines op */
"XJRSTF", "XJEN", "XPCW",
"JEN", "SFM", "XJRST", "IBP",
"JFOV", "JCRY1", "JCRY0", "JCRY", "JOV",

"APRID", "WRAPR", "RDAPR", "WRPI", "RDPI", "RDUBR", "CLRPT", "WRUBR",
"WREBR", "RDEBR",
"RDSPB", "RDCSB", "RDPUR", "RDCSTM", "RDTIM", "RDINT", "RDHSB",
"WRSPB", "WRCSB", "WRPUR", "WRCSTM", "WRTIM", "WRINT", "WRHSB",

          "LUUO01", "LUUO02", "LUUO03", "LUUO04", "LUUO05", "LUUO06", "LUUO07",
"LUUO10", "LUUO11", "LUUO12", "LUUO13", "LUUO14", "LUUO15", "LUUO16", "LUUO17",
"LUUO20", "LUUO21", "LUUO22", "LUUO23", "LUUO24", "LUUO25", "LUUO26", "LUUO27",
"LUUO30", "LUUO31", "LUUO32", "LUUO33", "LUUO34", "LUUO35", "LUUO36", "LUUO37",
"MUUO40", "MUUO41", "MUUO42", "MUUO43", "MUUO44", "MUUO45", "MUUO46", "MUUO47",
"MUUO50", "MUUO51", "MUUO52", "MUUO53", "MUUO54", "MUUO55", "MUUO56", "MUUO57",
"MUUO60", "MUUO61", "MUUO62", "MUUO63", "MUUO64", "MUUO65", "MUUO66", "MUUO67",
"MUUO70", "MUUO71", "MUUO72", "MUUO73", "MUUO74", "MUUO75", "MUUO76", "MUUO77",

"UJEN",         "GFAD", "GFSB", "JSYS", "ADJSP", "GFMP", "GFDV ", 
"DFAD", "DFSB", "DFMP", "DFDV", "DADD", "DSUB", "DMUL", "DDIV",
"DMOVE", "DMOVN", "FIX", "EXTEND", "DMOVEM", "DMOVNM", "FIXR", "FLTR",
"UFA", "DFN", "FSC", "ADJBP", "ILDB", "LDB", "IDPB", "DPB",
"FAD", "FADL", "FADM", "FADB", "FADR", "FADRL", "FADRM", "FADRB",
"FSB", "FSBL", "FSBM", "FSBB", "FSBR", "FSBRL", "FSBRM", "FSBRB",
"FMP", "FMPL", "FMPM", "FMPB", "FMPR", "FMPRL", "FMPRM", "FMPRB",
"FDV", "FDVL", "FDVM", "FDVB", "FDVR", "FDVRL", "FDVRM", "FDVRB",

"MOVE", "MOVEI", "MOVEM", "MOVES", "MOVS", "MOVSI", "MOVSM", "MOVSS",
"MOVN", "MOVNI", "MOVNM", "MOVNS", "MOVM", "MOVMI", "MOVMM", "MOVMS",
"IMUL", "IMULI", "IMULM", "IMULB", "MUL", "MULI", "MULM", "MULB",
"IDIV", "IDIVI", "IDIVM", "IDIVB", "DIV", "DIVI", "DIVM", "DIVB",
"ASH", "ROT", "LSH", "JFFO", "ASHC", "ROTC", "LSHC", "CIRC",
"EXCH", "BLT", "AOBJP", "AOBJN", "JRST", "JFCL", "XCT", "MAP", 
"PUSHJ", "PUSH", "POP", "POPJ", "JSR", "JSP", "JSA", "JRA",
"ADD", "ADDI", "ADDM", "ADDB", "SUB", "SUBI", "SUBM", "SUBB",

"CAI", "CAIL", "CAIE", "CAILE", "CAIA", "CAIGE", "CAIN", "CAIG",
"CAM", "CAML", "CAME", "CAMLE", "CAMA", "CAMGE", "CAMN", "CAMG",
"JUMP", "JUMPL", "JUMPE", "JUMPLE", "JUMPA", "JUMPGE", "JUMPN", "JUMPG",
"SKIP", "SKIPL", "SKIPE", "SKIPLE", "SKIPA", "SKIPGE", "SKIPN", "SKIPG",
"AOJ", "AOJL", "AOJE", "AOJLE", "AOJA", "AOJGE", "AOJN", "AOJG",
"AOS", "AOSL", "AOSE", "AOSLE", "AOSA", "AOSGE", "AOSN", "AOSG",
"SOJ", "SOJL", "SOJE", "SOJLE", "SOJA", "SOJGE", "SOJN", "SOJG",
"SOS", "SOSL", "SOSE", "SOSLE", "SOSA", "SOSGE", "SOSN", "SOSG",

"SETZ", "SETZI", "SETZM", "SETZB", "AND", "ANDI", "ANDM", "ANDB", 
"ANDCA", "ANDCAI", "ANDCAM", "ANDCAB", "SETM", "SETMI", "SETMM", "SETMB",
"ANDCM", "ANDCMI", "ANDCMM", "ANDCMB", "SETA", "SETAI", "SETAM", "SETAB",
"XOR", "XORI", "XORM", "XORB", "IOR", "IORI", "IORM", "IORB",  
"ANDCB", "ANDCBI", "ANDCBM", "ANDCBB", "EQV", "EQVI", "EQVM", "EQVB", 
"SETCA", "SETCAI", "SETCAM", "SETCAB", "ORCA", "ORCAI", "ORCAM", "ORCAB",
"SETCM", "SETCMI", "SETCMM", "SETCMB", "ORCM", "ORCMI", "ORCMM", "ORCMB",
"ORCB", "ORCBI", "ORCBM", "ORCBB", "SETO", "SETOI", "SETOM", "SETOB",

"HLL", "HLLI", "HLLM", "HLLS", "HRL", "HRLI", "HRLM", "HRLS", 
"HLLZ", "HLLZI", "HLLZM", "HLLZS", "HRLZ", "HRLZI", "HRLZM", "HRLZS",
"HLLO", "HLLOI", "HLLOM", "HLLOS", "HRLO", "HRLOI", "HRLOM", "HRLOS",
"HLLE", "HLLEI", "HLLEM", "HLLES", "HRLE", "HRLEI", "HRLEM", "HRLES",
"HRR", "HRRI", "HRRM", "HRRS", "HLR", "HLRI", "HLRM", "HLRS",
"HRRZ", "HRRZI", "HRRZM", "HRRZS", "HLRZ", "HLRZI", "HLRZM", "HLRZS",
"HRRO", "HRROI", "HRROM", "HRROS", "HLRO", "HLROI", "HLROM", "HLROS",
"HRRE", "HRREI", "HRREM", "HRRES", "HLRE", "HLREI", "HLREM", "HLRES",

"TRN", "TLN", "TRNE", "TLNE", "TRNA", "TLNA", "TRNN", "TLNN",
"TDN", "TSN", "TDNE", "TSNE", "TDNA", "TSNA", "TDNN", "TSNN",
"TRZ", "TLZ", "TRZE", "TLZE", "TRZA", "TLZA", "TRZN", "TLZN",
"TDZ", "TSZ", "TDZE", "TSZE", "TDZA", "TSZA", "TDZN", "TSZN",
"TRC", "TLC", "TRCE", "TLCE", "TRCA", "TLCA", "TRCN", "TLCN",
"TDC", "TSC", "TDCE", "TSCE", "TDCA", "TSCA", "TDCN", "TSCN",
"TRO", "TLO", "TROE", "TLOE", "TROA", "TLOA", "TRON", "TLON",
"TDO", "TSO", "TDOE", "TSOE", "TDOA", "TSOA", "TDON", "TSON",

"UMOVE", "UMOVEM",                                      /* KS10 I/O */
"TIOE", "TION", "RDIO", "WRIO",
"BSIO", "BCIO", "BLTBU", "BLTUB",
"TIOEB", "TIONB", "RDIOB", "WRIOB",
"BSIOB", "BCIOB", 

"BLKI", "DATAI", "BLKO", "DATAO",                       /* classic I/O */
"CONO",  "CONI", "CONSZ", "CONSO",

"CLEAR", "CLEARI", "CLEARM", "CLEARB",
"OR", "ORI", "ORM", "ORB", "XMOVEI", "XHLLI",           /* alternate ops */

        "CMPSL", "CMPSE", "CMPSLE",                     /* extended ops */
"EDIT", "CMPSGE", "CMPSN", "CMPSG",
"CVTDBO", "CVTDBT", "CVTBDO", "CVTBDT",
"MOVSO", "MOVST", "MOVSLJ", "MOVSRJ",
"XBLT", "GSNGL", "GDBLE", "GDFIX",
"GFIX", "GDFIXR", "GFIXR", "DGFLTR",
"GFLTR", "GFSC",

NULL
};

static const d10 opc_val[] = {
 0102000000000+I_AC+I_ITS, 0103000000000+I_AC+I_ITS,
 0710000000000+I_AC+I_ITS, 0711000000000+I_AC+I_ITS, 0712000000000+I_AC+I_ITS,
 0713000000000+I_AC+I_ITS, 0714000000000+I_AC+I_ITS, 0715000000000+I_AC+I_ITS,
 0720000000000+I_AC+I_ITS, 0721000000000+I_AC+I_ITS, 0722000000000+I_AC+I_ITS,
 0723000000000+I_AC+I_ITS, 0724000000000+I_AC+I_ITS, 0725000000000+I_AC+I_ITS,
 0701000000000+I_OP+I_ITS, 0701440000000+I_OP+I_ITS, 0701540000000+I_OP+I_ITS,
 0702000000000+I_OP+I_ITS, 0702040000000+I_OP+I_ITS,
 0702100000000+I_OP+I_ITS, 0702140000000+I_OP+I_ITS, 0702340000000+I_OP+I_ITS,
 0702400000000+I_OP+I_ITS, 0702440000000+I_OP+I_ITS,
 0702500000000+I_OP+I_ITS, 0702540000000+I_OP+I_ITS, 0702740000000+I_OP+I_ITS,
 
 0254040000000+I_OP, 0254100000000+I_OP,
 0254200000000+I_OP, 0254240000000+I_OP, 0254300000000+I_OP, 0254340000000+I_OP,
 0254500000000+I_OP, 0254600000000+I_OP, 0254640000000+I_OP, 0133000000000+I_OP,
 0255040000000+I_OP, 0255100000000+I_OP, 0255200000000+I_OP, 0255300000000+I_OP,
 0255400000000+I_OP,

 0700000000000+I_OP, 0700200000000+I_OP, 0700240000000+I_OP, 0700600000000+I_OP,
 0700640000000+I_OP, 0701040000000+I_OP, 0701100000000+I_OP, 0701140000000+I_OP,
 0701200000000+I_OP, 0701240000000+I_OP,
 0702000000000+I_OP, 0702040000000+I_OP, 0702100000000+I_OP, 0702140000000+I_OP,
 0702200000000+I_OP, 0702240000000+I_OP, 0702300000000+I_OP,
 0702400000000+I_OP, 0702440000000+I_OP, 0702500000000+I_OP, 0702540000000+I_OP,
 0702600000000+I_OP, 0702640000000+I_OP, 0702700000000+I_OP,

                     0001000000000+I_AC, 0002000000000+I_AC, 0003000000000+I_AC,
 0004000000000+I_AC, 0005000000000+I_AC, 0006000000000+I_AC, 0007000000000+I_AC,
 0010000000000+I_AC, 0011000000000+I_AC, 0012000000000+I_AC, 0013000000000+I_AC,
 0014000000000+I_AC, 0015000000000+I_AC, 0016000000000+I_AC, 0017000000000+I_AC,
 0020000000000+I_AC, 0021000000000+I_AC, 0022000000000+I_AC, 0023000000000+I_AC,
 0024000000000+I_AC, 0025000000000+I_AC, 0026000000000+I_AC, 0027000000000+I_AC,
 0030000000000+I_AC, 0031000000000+I_AC, 0032000000000+I_AC, 0033000000000+I_AC,
 0034000000000+I_AC, 0035000000000+I_AC, 0036000000000+I_AC, 0037000000000+I_AC,
 0040000000000+I_AC, 0041000000000+I_AC, 0042000000000+I_AC, 0043000000000+I_AC,
 0044000000000+I_AC, 0045000000000+I_AC, 0046000000000+I_AC, 0047000000000+I_AC,
 0050000000000+I_AC, 0051000000000+I_AC, 0052000000000+I_AC, 0053000000000+I_AC,
 0054000000000+I_AC, 0055000000000+I_AC, 0056000000000+I_AC, 0057000000000+I_AC,
 0060000000000+I_AC, 0061000000000+I_AC, 0062000000000+I_AC, 0063000000000+I_AC,
 0064000000000+I_AC, 0065000000000+I_AC, 0066000000000+I_AC, 0067000000000+I_AC,
 0070000000000+I_AC, 0071000000000+I_AC, 0072000000000+I_AC, 0073000000000+I_AC,
 0074000000000+I_AC, 0075000000000+I_AC, 0076000000000+I_AC, 0077000000000+I_AC,

 0100000000000+I_AC,                     0102000000000+I_AC, 0103000000000+I_AC,
 0104000000000+I_AC, 0105000000000+I_AC, 0106000000000+I_AC, 0107000000000+I_AC,
 0110000000000+I_AC, 0111000000000+I_AC, 0112000000000+I_AC, 0113000000000+I_AC,
 0114000000000+I_AC, 0115000000000+I_AC, 0116000000000+I_AC, 0117000000000+I_AC,
 0120000000000+I_AC, 0121000000000+I_AC, 0122000000000+I_AC, 0123000000000+I_AC,
 0124000000000+I_AC, 0125000000000+I_AC, 0126000000000+I_AC, 0127000000000+I_AC,
 0130000000000+I_AC, 0131000000000+I_AC, 0132000000000+I_AC, 0133000000000+I_AC,
 0134000000000+I_AC, 0135000000000+I_AC, 0136000000000+I_AC, 0137000000000+I_AC,
 0140000000000+I_AC, 0141000000000+I_AC, 0142000000000+I_AC, 0143000000000+I_AC,
 0144000000000+I_AC, 0145000000000+I_AC, 0146000000000+I_AC, 0147000000000+I_AC,
 0150000000000+I_AC, 0151000000000+I_AC, 0152000000000+I_AC, 0153000000000+I_AC,
 0154000000000+I_AC, 0155000000000+I_AC, 0156000000000+I_AC, 0157000000000+I_AC,
 0160000000000+I_AC, 0161000000000+I_AC, 0162000000000+I_AC, 0163000000000+I_AC,
 0164000000000+I_AC, 0165000000000+I_AC, 0166000000000+I_AC, 0167000000000+I_AC,
 0170000000000+I_AC, 0171000000000+I_AC, 0172000000000+I_AC, 0173000000000+I_AC,
 0174000000000+I_AC, 0175000000000+I_AC, 0176000000000+I_AC, 0177000000000+I_AC,

 0200000000000+I_AC, 0201000000000+I_AC, 0202000000000+I_AC, 0203000000000+I_AC,
 0204000000000+I_AC, 0205000000000+I_AC, 0206000000000+I_AC, 0207000000000+I_AC,
 0210000000000+I_AC, 0211000000000+I_AC, 0212000000000+I_AC, 0213000000000+I_AC,
 0214000000000+I_AC, 0215000000000+I_AC, 0216000000000+I_AC, 0217000000000+I_AC,
 0220000000000+I_AC, 0221000000000+I_AC, 0222000000000+I_AC, 0223000000000+I_AC,
 0224000000000+I_AC, 0225000000000+I_AC, 0226000000000+I_AC, 0227000000000+I_AC,
 0230000000000+I_AC, 0231000000000+I_AC, 0232000000000+I_AC, 0233000000000+I_AC,
 0234000000000+I_AC, 0235000000000+I_AC, 0236000000000+I_AC, 0237000000000+I_AC,
 0240000000000+I_AC, 0241000000000+I_AC, 0242000000000+I_AC, 0243000000000+I_AC,
 0244000000000+I_AC, 0245000000000+I_AC, 0246000000000+I_AC, 0247000000000+I_AC+I_ITS,
 0250000000000+I_AC, 0251000000000+I_AC, 0252000000000+I_AC, 0253000000000+I_AC,
 0254000000000+I_AC, 0255000000000+I_AC, 0256000000000+I_AC, 0257000000000+I_AC,
 0260000000000+I_AC, 0261000000000+I_AC, 0262000000000+I_AC, 0263000000000+I_AC,
 0264000000000+I_AC, 0265000000000+I_AC, 0266000000000+I_AC, 0267000000000+I_AC,
 0270000000000+I_AC, 0271000000000+I_AC, 0272000000000+I_AC, 0273000000000+I_AC,
 0274000000000+I_AC, 0275000000000+I_AC, 0276000000000+I_AC, 0277000000000+I_AC,

 0300000000000+I_AC, 0301000000000+I_AC, 0302000000000+I_AC, 0303000000000+I_AC,
 0304000000000+I_AC, 0305000000000+I_AC, 0306000000000+I_AC, 0307000000000+I_AC,
 0310000000000+I_AC, 0311000000000+I_AC, 0312000000000+I_AC, 0313000000000+I_AC,
 0314000000000+I_AC, 0315000000000+I_AC, 0316000000000+I_AC, 0317000000000+I_AC,
 0320000000000+I_AC, 0321000000000+I_AC, 0322000000000+I_AC, 0323000000000+I_AC,
 0324000000000+I_AC, 0325000000000+I_AC, 0326000000000+I_AC, 0327000000000+I_AC,
 0330000000000+I_AC, 0331000000000+I_AC, 0332000000000+I_AC, 0333000000000+I_AC,
 0334000000000+I_AC, 0335000000000+I_AC, 0336000000000+I_AC, 0337000000000+I_AC,
 0340000000000+I_AC, 0341000000000+I_AC, 0342000000000+I_AC, 0343000000000+I_AC,
 0344000000000+I_AC, 0345000000000+I_AC, 0346000000000+I_AC, 0347000000000+I_AC,
 0350000000000+I_AC, 0351000000000+I_AC, 0352000000000+I_AC, 0353000000000+I_AC,
 0354000000000+I_AC, 0355000000000+I_AC, 0356000000000+I_AC, 0357000000000+I_AC,
 0360000000000+I_AC, 0361000000000+I_AC, 0362000000000+I_AC, 0363000000000+I_AC,
 0364000000000+I_AC, 0365000000000+I_AC, 0366000000000+I_AC, 0367000000000+I_AC,
 0370000000000+I_AC, 0371000000000+I_AC, 0372000000000+I_AC, 0373000000000+I_AC,
 0374000000000+I_AC, 0375000000000+I_AC, 0376000000000+I_AC, 0377000000000+I_AC,

 0400000000000+I_AC, 0401000000000+I_AC, 0402000000000+I_AC, 0403000000000+I_AC,
 0404000000000+I_AC, 0405000000000+I_AC, 0406000000000+I_AC, 0407000000000+I_AC,
 0410000000000+I_AC, 0411000000000+I_AC, 0412000000000+I_AC, 0413000000000+I_AC,
 0414000000000+I_AC, 0415000000000+I_AC, 0416000000000+I_AC, 0417000000000+I_AC,
 0420000000000+I_AC, 0421000000000+I_AC, 0422000000000+I_AC, 0423000000000+I_AC,
 0424000000000+I_AC, 0425000000000+I_AC, 0426000000000+I_AC, 0427000000000+I_AC,
 0430000000000+I_AC, 0431000000000+I_AC, 0432000000000+I_AC, 0433000000000+I_AC,
 0434000000000+I_AC, 0435000000000+I_AC, 0436000000000+I_AC, 0437000000000+I_AC,
 0440000000000+I_AC, 0441000000000+I_AC, 0442000000000+I_AC, 0443000000000+I_AC,
 0444000000000+I_AC, 0445000000000+I_AC, 0446000000000+I_AC, 0447000000000+I_AC,
 0450000000000+I_AC, 0451000000000+I_AC, 0452000000000+I_AC, 0453000000000+I_AC,
 0454000000000+I_AC, 0455000000000+I_AC, 0456000000000+I_AC, 0457000000000+I_AC,
 0460000000000+I_AC, 0461000000000+I_AC, 0462000000000+I_AC, 0463000000000+I_AC,
 0464000000000+I_AC, 0465000000000+I_AC, 0466000000000+I_AC, 0467000000000+I_AC,
 0470000000000+I_AC, 0471000000000+I_AC, 0472000000000+I_AC, 0473000000000+I_AC,
 0474000000000+I_AC, 0475000000000+I_AC, 0476000000000+I_AC, 0477000000000+I_AC,

 0500000000000+I_AC, 0501000000000+I_AC, 0502000000000+I_AC, 0503000000000+I_AC,
 0504000000000+I_AC, 0505000000000+I_AC, 0506000000000+I_AC, 0507000000000+I_AC,
 0510000000000+I_AC, 0511000000000+I_AC, 0512000000000+I_AC, 0513000000000+I_AC,
 0514000000000+I_AC, 0515000000000+I_AC, 0516000000000+I_AC, 0517000000000+I_AC,
 0520000000000+I_AC, 0521000000000+I_AC, 0522000000000+I_AC, 0523000000000+I_AC,
 0524000000000+I_AC, 0525000000000+I_AC, 0526000000000+I_AC, 0527000000000+I_AC,
 0530000000000+I_AC, 0531000000000+I_AC, 0532000000000+I_AC, 0533000000000+I_AC,
 0534000000000+I_AC, 0535000000000+I_AC, 0536000000000+I_AC, 0537000000000+I_AC,
 0540000000000+I_AC, 0541000000000+I_AC, 0542000000000+I_AC, 0543000000000+I_AC,
 0544000000000+I_AC, 0545000000000+I_AC, 0546000000000+I_AC, 0547000000000+I_AC,
 0550000000000+I_AC, 0551000000000+I_AC, 0552000000000+I_AC, 0553000000000+I_AC,
 0554000000000+I_AC, 0555000000000+I_AC, 0556000000000+I_AC, 0557000000000+I_AC,
 0560000000000+I_AC, 0561000000000+I_AC, 0562000000000+I_AC, 0563000000000+I_AC,
 0564000000000+I_AC, 0565000000000+I_AC, 0566000000000+I_AC, 0567000000000+I_AC,
 0570000000000+I_AC, 0571000000000+I_AC, 0572000000000+I_AC, 0573000000000+I_AC,
 0574000000000+I_AC, 0575000000000+I_AC, 0576000000000+I_AC, 0577000000000+I_AC,

 0600000000000+I_AC, 0601000000000+I_AC, 0602000000000+I_AC, 0603000000000+I_AC,
 0604000000000+I_AC, 0605000000000+I_AC, 0606000000000+I_AC, 0607000000000+I_AC,
 0610000000000+I_AC, 0611000000000+I_AC, 0612000000000+I_AC, 0613000000000+I_AC,
 0614000000000+I_AC, 0615000000000+I_AC, 0616000000000+I_AC, 0617000000000+I_AC,
 0620000000000+I_AC, 0621000000000+I_AC, 0622000000000+I_AC, 0623000000000+I_AC,
 0624000000000+I_AC, 0625000000000+I_AC, 0626000000000+I_AC, 0627000000000+I_AC,
 0630000000000+I_AC, 0631000000000+I_AC, 0632000000000+I_AC, 0633000000000+I_AC,
 0634000000000+I_AC, 0635000000000+I_AC, 0636000000000+I_AC, 0637000000000+I_AC,
 0640000000000+I_AC, 0641000000000+I_AC, 0642000000000+I_AC, 0643000000000+I_AC,
 0644000000000+I_AC, 0645000000000+I_AC, 0646000000000+I_AC, 0647000000000+I_AC,
 0650000000000+I_AC, 0651000000000+I_AC, 0652000000000+I_AC, 0653000000000+I_AC,
 0654000000000+I_AC, 0655000000000+I_AC, 0656000000000+I_AC, 0657000000000+I_AC,
 0660000000000+I_AC, 0661000000000+I_AC, 0662000000000+I_AC, 0663000000000+I_AC,
 0664000000000+I_AC, 0665000000000+I_AC, 0666000000000+I_AC, 0667000000000+I_AC,
 0670000000000+I_AC, 0671000000000+I_AC, 0672000000000+I_AC, 0673000000000+I_AC,
 0674000000000+I_AC, 0675000000000+I_AC, 0676000000000+I_AC, 0677000000000+I_AC,

 0704000000000+I_AC, 0705000000000+I_AC,
 0710000000000+I_AC, 0711000000000+I_AC, 0712000000000+I_AC, 0713000000000+I_AC,
 0714000000000+I_AC, 0715000000000+I_AC, 0716000000000+I_AC, 0717000000000+I_AC,
 0720000000000+I_AC, 0721000000000+I_AC, 0722000000000+I_AC, 0723000000000+I_AC,
 0724000000000+I_AC, 0725000000000+I_AC,

 0700000000000+I_IO, 0700040000000+I_IO, 0700100000000+I_IO, 0700140000000+I_IO,
 0700200000000+I_IO, 0700240000000+I_IO, 0700300000000+I_IO, 0700340000000+I_IO,

 0400000000000+I_AC, 0401000000000+I_AC, 0402000000000+I_AC, 0403000000000+I_AC,
 0434000000000+I_AC, 0435000000000+I_AC, 0436000000000+I_AC, 0437000000000+I_AC,
 0415000000000+I_AC, 0501000000000+I_AC,
 
                     0001000000000+I_AC, 0002000000000+I_AC, 0003000000000+I_AC,
 0004000000000+I_AC, 0005000000000+I_AC, 0006000000000+I_AC, 0007000000000+I_AC,
 0010000000000+I_AC, 0011000000000+I_AC, 0012000000000+I_AC, 0013000000000+I_AC,
 0014000000000+I_AC, 0015000000000+I_AC, 0016000000000+I_AC, 0017000000000+I_AC,
 0020000000000+I_AC, 0021000000000+I_AC, 0022000000000+I_AC, 0023000000000+I_AC,
 0024000000000+I_AC, 0025000000000+I_AC, 0026000000000+I_AC, 0027000000000+I_AC,
 0030000000000+I_AC, 0031000000000+I_AC,
 -1
 };

#define NUMDEV  6

static const char *devnam[NUMDEV] = {
 "APR", "PI", "PAG", "CCA", "TIM", "MTR"
 };

/* Symbolic decode

   Inputs:
        *of     =       output stream
        addr    =       current PC
        *val    =       pointer to values
        *uptr   =       pointer to unit
        sw      =       switches
   Outputs:
        return  =       status code
*/

#define FMTASC(x) ((x) < 040)? "<%03o>": "%c", (x)
#define SIXTOASC(x) ((x) + 040)

t_stat fprint_sym (FILE *of, t_addr addr, t_value *val,
    UNIT *uptr, int32 sw)
{
int32 i, j, c, cflag, ac, xr, y, dev;
d10 inst;

inst = val[0];
cflag = (uptr == NULL) || (uptr == &cpu_unit);
if (sw & SWMASK ('A')) {                                /* ASCII? */
    if (inst > 0377) return SCPE_ARG;
    fprintf (of, FMTASC ((int32) (inst & 0177)));
    return SCPE_OK;
    }
if (sw & SWMASK ('C')) {                                /* character? */
    for (i = 30; i >= 0; i = i - 6) {
        c = (int32) ((inst >> i) & 077);
        fprintf (of, "%c", SIXTOASC (c));
		}    
    return SCPE_OK;
    }
if (sw & SWMASK ('P')) {                                /* packed? */
    for (i = 29; i >= 0; i = i - 7) {
        c = (int32) ((inst >> i) & 0177);
        fprintf (of, FMTASC (c));
		}
    return SCPE_OK;
    }
if (!(sw & SWMASK ('M'))) return SCPE_ARG;

/* Instruction decode */

ac = GET_AC (inst);
xr = GET_XR (inst);
y = GET_ADDR (inst);
dev = GET_DEV (inst);
for (i = 0; opc_val[i] >= 0; i++) {                     /* loop thru ops */
    j = (int32) ((opc_val[i] >> I_V_FL) & I_M_FL);      /* get class */
    if (((opc_val[i] & DMASK) == (inst & masks[j])) &&  /* match? */
            (((opc_val[i] & I_ITS) == 0) || ITS)) {
        fprintf (of, "%s ", opcode[i]);                 /* opcode */
        switch (j) {                                    /* case on class */

        case I_V_AC:                                    /* AC + address */
            fprintf (of, "%-o,", ac);                   /* print AC, fall thru */
        case I_V_OP:                                    /* address only */
            if (inst & INST_IND) fprintf (of, "@");
            if (xr) fprintf (of, "%-o(%-o)", y, xr);
            else fprintf (of, "%-o", y);
            break;

        case I_V_IO:                                    /* I/O */
            if (dev < NUMDEV) fprintf (of, "%s,", devnam[dev]);
            else fprintf (of, "%-o,", dev);
            if (inst & INST_IND) fprintf (of, "@");
            if (xr) fprintf (of, "%-o(%-o)", y, xr);
            else fprintf (of, "%-o", y);
            break;
            }                                           /* end case */
        return SCPE_OK;
        }                                               /* end if */
    }                                                   /* end for */
return SCPE_ARG;
}

/* Get operand, including indirect and index

   Inputs:
        *cptr   =       pointer to input string
        *status =       pointer to error status
   Outputs:
        val     =       output value
*/

t_value get_opnd (char *cptr, t_stat *status)
{
int32 sign = 0;
t_value val, xr = 0, ind = 0;
char *tptr;

*status = SCPE_ARG;                                     /* assume fail */
if (*cptr == '@') {
    ind = INST_IND;
    cptr++;
    }   
if (*cptr == '+') cptr++;
else if (*cptr == '-') {
    sign = 1;
    cptr++;
    }
val = strtotv (cptr, &tptr, 8);
if (val > 0777777) return 0;
if (sign) val = (~val + 1) & 0777777;
cptr = tptr;
if (*cptr == '(') {
    cptr++;
    xr = strtotv (cptr, &tptr, 8);
    if ((cptr == tptr) || (*tptr != ')') ||
        (xr > AC_NUM) || (xr == 0)) return 0;
    cptr = ++tptr;
    }
if (*cptr == 0) *status = SCPE_OK;
return (ind | (xr << 18) | val);
}

/* Symbolic input

   Inputs:
        *cptr   =       pointer to input string
        addr    =       current PC
        uptr    =       pointer to unit
        *val    =       pointer to output values
        sw      =       switches
   Outputs:
        status  =       error status
*/

t_stat parse_sym (char *cptr, t_addr addr, UNIT *uptr, t_value *val, int32 sw)
{
int32 cflag, i, j;
t_value ac, dev;
t_stat r;
char gbuf[CBUFSIZE];

cflag = (uptr == NULL) || (uptr == &cpu_unit);
while (isspace (*cptr)) cptr++;
for (i = 0; i < 6; i++) {
    if (cptr[i] == 0) {
        for (j = i + 1; j <= 6; j++) cptr[j] = 0;
        break;
        }
    }
if ((sw & SWMASK ('A')) || ((*cptr == '\'') && cptr++)) { /* ASCII char? */
    if (cptr[0] == 0) return SCPE_ARG;                  /* must have 1 char */
    val[0] = (t_value) cptr[0];
    return SCPE_OK;
    }
if ((sw & SWMASK ('C')) || ((*cptr == '"') && cptr++)) { /* sixbit string? */
    if (cptr[0] == 0) return SCPE_ARG;                  /* must have 1 char */
    for (i = 0; i < 6; i++) {
        val[0] = (val[0] << 6);
        if (cptr[i]) val[0] = val[0] |
            ((t_value) ((cptr[i] + 040) & 077));
        }
    return SCPE_OK;
    }
if ((sw & SWMASK ('P')) || ((*cptr == '#') && cptr++)) { /* packed string? */
    if (cptr[0] == 0) return SCPE_ARG;                  /* must have 1 char */
    for (i = 0; i < 5; i++) val[0] = (val[0] << 7) | ((t_value) cptr[i]);
    val[0] = val[0] << 1;
    return SCPE_OK;
    }

/* Instruction parse */

cptr = get_glyph (cptr, gbuf, 0);                       /* get opcode */
for (i = 0; (opcode[i] != NULL) && (strcmp (opcode[i], gbuf) != 0) ; i++) ;
if (opcode[i] == NULL) return SCPE_ARG;
val[0] = opc_val[i] & DMASK;                            /* get value */
j = (int32) ((opc_val[i] >> I_V_FL) & I_M_FL);          /* get class */
switch (j) {                                            /* case on class */

    case I_V_AC:                                        /* AC + operand */
        if (strchr (cptr, ',')) {                       /* AC specified? */
            cptr = get_glyph (cptr, gbuf, ',');         /* get glyph */
            if (gbuf[0]) {                              /* can be omitted */
                ac = get_uint (gbuf, 8, AC_NUM - 1, &r);
                if (r != SCPE_OK) return SCPE_ARG;
                val[0] = val[0] | (ac << INST_V_AC);
                }
            }                                           /* fall through */
    case I_V_OP:                                        /* operand */
        cptr = get_glyph (cptr, gbuf, 0);
        val[0] = val[0] | get_opnd (gbuf, &r);
        if (r != SCPE_OK) return SCPE_ARG;
        break;

    case I_V_IO:                                        /* I/O */
        cptr = get_glyph (cptr, gbuf, ',');             /* get glyph */
        for (dev = 0; (dev < NUMDEV) && (strcmp (devnam[dev], gbuf) != 0); dev++);
        if (dev >= NUMDEV) {
            dev = get_uint (gbuf, 8, INST_M_DEV, &r);
            if (r != SCPE_OK) return SCPE_ARG;
            }
        val[0] = val[0] | (dev << INST_V_DEV);
        cptr = get_glyph (cptr, gbuf, 0);
        val[0] = val[0] | get_opnd (gbuf, &r);
        if (r != SCPE_OK) return SCPE_ARG;
        break;
        }                                               /* end case */

if (*cptr != 0) return SCPE_ARG;                        /* junk at end? */
return SCPE_OK;
}
