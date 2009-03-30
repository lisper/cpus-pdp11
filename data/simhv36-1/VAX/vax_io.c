/* vax_io.c: VAX 3900 Qbus IO simulator

   Copyright (c) 1998-2005, Robert M Supnik

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

   qba          Qbus adapter

   03-Dec-05    RMS     Added SHOW QBA VIRT and ex/dep via map
   05-Oct-05    RMS     Fixed bug in autoconfiguration (missing XU)
   25-Jul-05    RMS     Revised autoconfiguration algorithm and interface
   30-Sep-04    RMS     Revised Qbus interface
                        Moved mem_err, crd_err interrupts here from vax_cpu.c
   09-Sep-04    RMS     Integrated powerup into RESET (with -p)
   05-Sep-04    RMS     Added CRD interrupt handling
   28-May-04    RMS     Revised I/O dispatching (from John Dundas)
   21-Mar-04    RMS     Added RXV21 support
   21-Dec-03    RMS     Fixed bug in autoconfigure vector assignment; added controls
   21-Nov-03    RMS     Added check for interrupt slot conflict (found by Dave Hittner)
   29-Oct-03    RMS     Fixed WriteX declaration (found by Mark Pizzolato)
   19-Apr-03    RMS     Added optimized byte and word DMA routines
   12-Mar-03    RMS     Added logical name support
   22-Dec-02    RMS     Added console halt support
   12-Oct-02    RMS     Added autoconfigure support
                        Added SHOW IO space routine
   29-Sep-02    RMS     Added dynamic table support
   07-Sep-02    RMS     Added TMSCP and variable vector support
*/

#include "vax_defs.h"

/* CQBIC system configuration register */

#define CQSCR_POK       0x00008000                      /* power ok RO1 */
#define CQSCR_BHL       0x00004000                      /* BHALT enb */
#define CQSCR_AUX       0x00000400                      /* aux mode RONI */
#define CQSCR_DBO       0x0000000C                      /* offset NI */
#define CQSCR_RW        (CQSCR_BHL | CQSCR_DBO)
#define CQSCR_MASK      (CQSCR_RW | CQSCR_POK | CQSCR_AUX)

/* CQBIC DMA system error register - W1C */

#define CQDSER_BHL      0x00008000                      /* BHALT NI */
#define CQDSER_DCN      0x00004000                      /* DC ~OK NI */
#define CQDSER_MNX      0x00000080                      /* master NXM */
#define CQDSER_MPE      0x00000020                      /* master par NI */
#define CQDSER_SME      0x00000010                      /* slv mem err NI */
#define CQDSER_LST      0x00000008                      /* lost err */
#define CQDSER_TMO      0x00000004                      /* no grant NI */
#define CQDSER_SNX      0x00000001                      /* slave NXM */
#define CQDSER_ERR      (CQDSER_MNX | CQDSER_MPE | CQDSER_TMO | CQDSER_SNX)
#define CQDSER_MASK     0x0000C0BD

/* CQBIC master error address register */

#define CQMEAR_MASK     0x00001FFF                      /* Qbus page */

/* CQBIC slave error address register */

#define CQSEAR_MASK     0x000FFFFF                      /* mem page */

/* CQBIC map base register */

#define CQMBR_MASK      0x1FFF8000                      /* 32KB aligned */

/* CQBIC IPC register */

#define CQIPC_QME       0x00008000                      /* Qbus read NXM W1C */
#define CQIPC_INV       0x00004000                      /* CAM inval NIWO */
#define CQIPC_AHLT      0x00000100                      /* aux halt NI */
#define CQIPC_DBIE      0x00000040                      /* dbell int enb NI */
#define CQIPC_LME       0x00000020                      /* local mem enb */
#define CQIPC_DB        0x00000001                      /* doorbell req NI */
#define CQIPC_W1C       CQIPC_QME
#define CQIPC_RW        (CQIPC_AHLT | CQIPC_DBIE | CQIPC_LME | CQIPC_DB)
#define CQIPC_MASK      (CQIPC_RW | CQIPC_QME )

/* CQBIC map entry */

#define CQMAP_VLD       0x80000000                      /* valid */
#define CQMAP_PAG       0x000FFFFF                      /* mem page */

int32 int_req[IPL_HLVL] = { 0 };                        /* intr, IPL 14-17 */
int32 cq_scr = 0;                                       /* SCR */
int32 cq_dser = 0;                                      /* DSER */
int32 cq_mear = 0;                                      /* MEAR */
int32 cq_sear = 0;                                      /* SEAR */
int32 cq_mbr = 0;                                       /* MBR */
int32 cq_ipc = 0;                                       /* IPC */
int32 autcon_enb = 1;                                   /* autoconfig enable */

extern uint32 *M;
extern UNIT cpu_unit;
extern int32 PSL, SISR, trpirq, mem_err, crd_err, hlt_pin;
extern int32 p1;
extern int32 ssc_bto;
extern jmp_buf save_env;
extern int32 sim_switches;
extern DEVICE *sim_devices[];

extern int32 ReadB (uint32 pa);
extern int32 ReadW (uint32 pa);
extern int32 ReadL (uint32 pa);
extern void WriteB (uint32 pa, int32 val);
extern void WriteW (uint32 pa, int32 val);
extern void WriteL (uint32 pa, int32 val);
extern FILE *sim_log;

t_stat dbl_rd (int32 *data, int32 addr, int32 access);
t_stat dbl_wr (int32 data, int32 addr, int32 access);
int32 eval_int (void);
void cq_merr (int32 pa);
void cq_serr (int32 pa);
t_stat qba_reset (DEVICE *dptr);
t_stat qba_ex (t_value *vptr, t_addr exta, UNIT *uptr, int32 sw);
t_stat qba_dep (t_value val, t_addr exta, UNIT *uptr, int32 sw);
t_bool qba_map_addr (uint32 qa, uint32 *ma);
t_bool qba_map_addr_c (uint32 qa, uint32 *ma);
t_stat set_autocon (UNIT *uptr, int32 val, char *cptr, void *desc);
t_stat show_autocon (FILE *st, UNIT *uptr, int32 val, void *desc);
t_stat show_iospace (FILE *st, UNIT *uptr, int32 val, void *desc);
t_stat qba_show_virt (FILE *of, UNIT *uptr, int32 val, void *desc);

/* Qbus adapter data structures

   qba_dev      QBA device descriptor
   qba_unit     QBA units
   qba_reg      QBA register list
*/

DIB qba_dib = { IOBA_DBL, IOLN_DBL, &dbl_rd, &dbl_wr, 0 };

UNIT qba_unit = { UDATA (NULL, 0, 0) };

REG qba_reg[] = {
    { HRDATA (SCR, cq_scr, 16) },
    { HRDATA (DSER, cq_dser, 8) },
    { HRDATA (MEAR, cq_mear, 13) },
    { HRDATA (SEAR, cq_sear, 20) },
    { HRDATA (MBR, cq_mbr, 29) },
    { HRDATA (IPC, cq_ipc, 16) },
    { HRDATA (IPL17, int_req[3], 32), REG_RO },
    { HRDATA (IPL16, int_req[2], 32), REG_RO },
    { HRDATA (IPL15, int_req[1], 32), REG_RO },
    { HRDATA (IPL14, int_req[0], 32), REG_RO },
    { FLDATA (AUTOCON, autcon_enb, 0), REG_HRO },
    { NULL }
    };

MTAB qba_mod[] = {
    { MTAB_XTD|MTAB_VDV|MTAB_NMO, 0, "IOSPACE", NULL,
      NULL, &show_iospace },
    { MTAB_XTD|MTAB_VDV, 1, "AUTOCONFIG", "AUTOCONFIG",
      &set_autocon, &show_autocon },
    { MTAB_XTD|MTAB_VDV, 0, NULL, "NOAUTOCONFIG",
      &set_autocon, NULL },
    { MTAB_XTD|MTAB_VDV|MTAB_NMO|MTAB_SHP, 0, "VIRTUAL", NULL,
      NULL, &qba_show_virt },
    { 0 }
    };

DEVICE qba_dev = {
    "QBA", &qba_unit, qba_reg, qba_mod,
    1, 16, CQMAWIDTH, 2, 16, 16,
    &qba_ex, &qba_dep, &qba_reset,
    NULL, NULL, NULL,
    &qba_dib, DEV_QBUS
    };

/* IO page dispatches */

static t_stat (*iodispR[IOPAGESIZE >> 1])(int32 *dat, int32 ad, int32 md);
static t_stat (*iodispW[IOPAGESIZE >> 1])(int32 dat, int32 ad, int32 md);
static DIB *iodibp[IOPAGESIZE >> 1];

/* Interrupt request to interrupt action map */

int32 (*int_ack[IPL_HLVL][32])();                       /* int ack routines */

/* Interrupt request to vector map */

int32 int_vec[IPL_HLVL][32];                            /* int req to vector */

/* The KA65x handles errors in I/O space as follows

        - read: set DSER<7>, latch addr in MEAR, machine check
        - write: set DSER<7>, latch addr in MEAR, MEMERR interrupt
*/

int32 ReadQb (uint32 pa)
{
int32 idx, val;

idx = (pa & IOPAGEMASK) >> 1;
if (iodispR[idx]) {
    iodispR[idx] (&val, pa, READ);
    return val;
    }
cq_merr (pa);
MACH_CHECK (MCHK_READ);
return 0;
}

void WriteQb (uint32 pa, int32 val, int32 mode)
{
int32 idx;

idx = (pa & IOPAGEMASK) >> 1;
if (iodispW[idx]) {
    iodispW[idx] (val, pa, mode);
    return;
    }
cq_merr (pa);
mem_err = 1;
return;
}

/* ReadIO - read I/O space

   Inputs:
        pa      =       physical address
        lnt     =       length (BWLQ)
   Output:
        longword of data
*/

int32 ReadIO (int32 pa, int32 lnt)
{
int32 iod;

iod = ReadQb (pa);                                      /* wd from Qbus */
if (lnt < L_LONG) iod = iod << ((pa & 2)? 16: 0);       /* bw? position */
else iod = (ReadQb (pa + 2) << 16) | iod;               /* lw, get 2nd wd */
SET_IRQL;
return iod;
}

/* WriteIO - write I/O space

   Inputs:
        pa      =       physical address
        val     =       data to write, right justified in 32b longword
        lnt     =       length (BWLQ)
   Outputs:
        none
*/

void WriteIO (int32 pa, int32 val, int32 lnt)
{
if (lnt == L_BYTE) WriteQb (pa, val, WRITEB);
else if (lnt == L_WORD) WriteQb (pa, val, WRITE);
else {
    WriteQb (pa, val & 0xFFFF, WRITE);
    WriteQb (pa + 2, (val >> 16) & 0xFFFF, WRITE);
    }
SET_IRQL;
return;
}

/* Find highest priority outstanding interrupt */

int32 eval_int (void)
{
int32 ipl = PSL_GETIPL (PSL);
int32 i, t;

static const int32 sw_int_mask[IPL_SMAX] = {
    0xFFFE, 0xFFFC, 0xFFF8, 0xFFF0,                     /* 0 - 3 */
    0xFFE0, 0xFFC0, 0xFF80, 0xFF00,                     /* 4 - 7 */
    0xFE00, 0xFC00, 0xF800, 0xF000,                     /* 8 - B */
    0xE000, 0xC000, 0x8000                              /* C - E */
    };

if (hlt_pin) return IPL_HLTPIN;                         /* hlt pin int */
if ((ipl < IPL_MEMERR) && mem_err) return IPL_MEMERR;   /* mem err int */
if ((ipl < IPL_CRDERR) && crd_err) return IPL_CRDERR;   /* crd err int */
for (i = IPL_HMAX; i >= IPL_HMIN; i--) {                /* chk hwre int */
    if (i <= ipl) return 0;                             /* at ipl? no int */
    if (int_req[i - IPL_HMIN]) return i;                /* req != 0? int */
    }
if (ipl >= IPL_SMAX) return 0;                          /* ipl >= sw max? */
if ((t = SISR & sw_int_mask[ipl]) == 0) return 0;       /* eligible req */
for (i = IPL_SMAX; i > ipl; i--) {                      /* check swre int */
    if ((t >> i) & 1) return i;                         /* req != 0? int */
    }
return 0;
}

/* Return vector for highest priority hardware interrupt at IPL lvl */

int32 get_vector (int32 lvl)
{
int32 i;
int32 l = lvl - IPL_HMIN;

if (lvl == IPL_MEMERR) {                                /* mem error? */
    mem_err = 0;
    return SCB_MEMERR;
    }
if (lvl == IPL_CRDERR) {                                /* CRD error? */
    crd_err = 0;
    return SCB_CRDERR;
    }
if (lvl > IPL_HMAX) {                                   /* error req lvl? */
    ABORT (STOP_UIPL);                                  /* unknown intr */
    }
for (i = 0; int_req[l] && (i < 32); i++) {
    if ((int_req[l] >> i) & 1) {
        int_req[l] = int_req[l] & ~(1u << i);
        if (int_ack[l][i]) return int_ack[l][i]();
        return int_vec[l][i];
        }
    }
return 0;
}

/* CQBIC registers

   SCR          system configuration register
   DSER         DMA system error register (W1C)
   MEAR         master error address register (RO)
   SEAR         slave error address register (RO)
   MBR          map base register
   IPC          inter-processor communication register
*/

int32 cqbic_rd (int32 pa)
{
int32 rg = (pa - CQBICBASE) >> 2;

switch (rg) {

    case 0:                                             /* SCR */
        return (cq_scr | CQSCR_POK) & CQSCR_MASK;

    case 1:                                             /* DSER */
        return cq_dser & CQDSER_MASK;

    case 2:                                             /* MEAR */
        return cq_mear & CQMEAR_MASK;

    case 3:                                             /* SEAR */
        return cq_sear & CQSEAR_MASK;

    case 4:                                             /* MBR */
        return cq_mbr & CQMBR_MASK;
        }

return 0;
}

void cqbic_wr (int32 pa, int32 val, int32 lnt)
{
int32 nval, rg = (pa - CQBICBASE) >> 2;

if (lnt < L_LONG) {
    int32 sc = (pa & 3) << 3;
    int32 mask = (lnt == L_WORD)? 0xFFFF: 0xFF;
    int32 t = cqbic_rd (pa);
    nval = ((val & mask) << sc) | (t & ~(mask << sc));
    val = val << sc;
	}
else nval = val;
switch (rg) {

    case 0:                                             /* SCR */
        cq_scr = ((cq_scr & ~CQSCR_RW) | (nval & CQSCR_RW)) & CQSCR_MASK;
        break;

    case 1:                                             /* DSER */
        cq_dser = (cq_dser & ~val) & CQDSER_MASK;
        if (val & CQDSER_SME) cq_ipc = cq_ipc & ~CQIPC_QME;
        break;

    case 2: case 3:
        cq_merr (pa);                                   /* MEAR, SEAR */
        MACH_CHECK (MCHK_WRITE);
        break;

    case 4:                                             /* MBR */
        cq_mbr = nval & CQMBR_MASK;
        break;
        }
return;
}

/* IPC can be read as local register or as Qbus I/O
   Because of the W1C */

int32 cqipc_rd (int32 pa)
{
return cq_ipc & CQIPC_MASK;                             /* IPC */
}

void cqipc_wr (int32 pa, int32 val, int32 lnt)
{
int32 nval = val;

if (lnt < L_LONG) {
    int32 sc = (pa & 3) << 3;
    nval = val << sc;
    }

cq_ipc = cq_ipc & ~(nval & CQIPC_W1C);                  /* W1C */
if ((pa & 3) == 0)                                      /* low byte only */
    cq_ipc = ((cq_ipc & ~CQIPC_RW) | (val & CQIPC_RW)) & CQIPC_MASK;
return;
}

/* I/O page routines */

t_stat dbl_rd (int32 *data, int32 addr, int32 access)
{
*data = cq_ipc & CQIPC_MASK;
return SCPE_OK;
}

t_stat dbl_wr (int32 data, int32 addr, int32 access)
{
cqipc_wr (addr, data, (access == WRITEB)? L_BYTE: L_WORD);
return SCPE_OK;
}

/* CQBIC map read and write (reflects to main memory)

   Read error: set DSER<0>, latch slave address, machine check
   Write error: set DSER<0>, latch slave address, memory error interrupt
*/

int32 cqmap_rd (int32 pa)
{
int32 ma = (pa & CQMAPAMASK) + cq_mbr;                  /* mem addr */

if (ADDR_IS_MEM (ma)) return M[ma >> 2];
cq_serr (ma);                                           /* set err */
MACH_CHECK (MCHK_READ);                                 /* mcheck */
return 0;
}

void cqmap_wr (int32 pa, int32 val, int32 lnt)
{
int32 ma = (pa & CQMAPAMASK) + cq_mbr;                  /* mem addr */

if (ADDR_IS_MEM (ma)) {
    if (lnt < L_LONG) {
        int32 sc = (pa & 3) << 3;
        int32 mask = (lnt == L_WORD)? 0xFFFF: 0xFF;
        int32 t = M[ma >> 2];
        val = ((val & mask) << sc) | (t & ~(mask << sc));
        }
    M[ma >> 2] = val;
    }
else {
    cq_serr (ma);                                       /* error */
    mem_err = 1;
    }
return;
}

/* CQBIC Qbus memory read and write (reflects to main memory)

   May give master or slave error, depending on where the failure occurs
*/

int32 cqmem_rd (int32 pa)
{
int32 qa = pa & CQMAMASK;                               /* Qbus addr */
uint32 ma;

if (qba_map_addr (qa, &ma)) return M[ma >> 2];          /* map addr */
MACH_CHECK (MCHK_READ);                                 /* err? mcheck */
return 0;
}

void cqmem_wr (int32 pa, int32 val, int32 lnt)
{
int32 qa = pa & CQMAMASK;                               /* Qbus addr */
uint32 ma;

if (qba_map_addr (qa, &ma)) {                           /* map addr */
    if (lnt < L_LONG) {
        int32 sc = (pa & 3) << 3;
        int32 mask = (lnt == L_WORD)? 0xFFFF: 0xFF;
        int32 t = M[ma >> 2];
        val = ((val & mask) << sc) | (t & ~(mask << sc));
        }
    M[ma >> 2] = val;
    }
else mem_err = 1;
return;
}

/* Map an address via the translation map */

t_bool qba_map_addr (uint32 qa, uint32 *ma)
{
int32 qblk = (qa >> VA_V_VPN);                          /* Qbus blk */
int32 qmma = ((qblk << 2) & CQMAPAMASK) + cq_mbr;       /* map entry */

if (ADDR_IS_MEM (qmma)) {                               /* legit? */
    int32 qmap = M[qmma >> 2];                          /* get map */
    if (qmap & CQMAP_VLD) {                             /* valid? */
        *ma = ((qmap & CQMAP_PAG) << VA_V_VPN) + VA_GETOFF (qa);
        if (ADDR_IS_MEM (*ma)) return TRUE;             /* legit addr */
        cq_serr (*ma);                                  /* slave nxm */
        return FALSE;
        }
    cq_merr (qa);                                       /* master nxm */
    return FALSE;
    }
cq_serr (0);                                            /* inv mem */
return FALSE;
}

/* Map an address via the translation map - console version (no status changes) */

t_bool qba_map_addr_c (uint32 qa, uint32 *ma)
{
int32 qblk = (qa >> VA_V_VPN);                          /* Qbus blk */
int32 qmma = ((qblk << 2) & CQMAPAMASK) + cq_mbr;       /* map entry */

if (ADDR_IS_MEM (qmma)) {                               /* legit? */
    int32 qmap = M[qmma >> 2];                          /* get map */
    if (qmap & CQMAP_VLD) {                             /* valid? */
        *ma = ((qmap & CQMAP_PAG) << VA_V_VPN) + VA_GETOFF (qa);
        return TRUE;                                    /* legit addr */
        }
    }
return FALSE;
}

/* Set master error */

void cq_merr (int32 pa)
{
if (cq_dser & CQDSER_ERR) cq_dser = cq_dser | CQDSER_LST;
cq_dser = cq_dser | CQDSER_MNX;                         /* master nxm */
cq_mear = (pa >> VA_V_VPN) & CQMEAR_MASK;               /* page addr */
return;
}

/* Set slave error */

void cq_serr (int32 pa)
{
if (cq_dser & CQDSER_ERR) cq_dser = cq_dser | CQDSER_LST;
cq_dser = cq_dser | CQDSER_SNX;                         /* slave nxm */
cq_sear = (pa >> VA_V_VPN) & CQSEAR_MASK;
return;
}

/* Reset I/O bus */

void ioreset_wr (int32 data)
{
reset_all (5);                                          /* from qba on... */
return;
}

/* Powerup CQBIC */

t_stat qba_powerup (void)
{
cq_mbr = 0;
cq_scr = CQSCR_POK;
return SCPE_OK;
}

/* Reset CQBIC */

t_stat qba_reset (DEVICE *dptr)
{
int32 i;

if (sim_switches & SWMASK ('P')) qba_powerup ();
cq_scr = (cq_scr & CQSCR_BHL) | CQSCR_POK;
cq_dser = cq_mear = cq_sear = cq_ipc = 0;
for (i = 0; i < IPL_HLVL; i++) int_req[i] = 0;
return SCPE_OK;
}

/* Qbus I/O buffer routines, aligned access

   Map_ReadB    -       fetch byte buffer from memory
   Map_ReadW    -       fetch word buffer from memory
   Map_WriteB   -       store byte buffer into memory
   Map_WriteW   -       store word buffer into memory
*/

int32 Map_ReadB (uint32 ba, int32 bc, uint8 *buf)
{
int32 i;
uint32 ma, dat;

if ((ba | bc) & 03) {                                   /* check alignment */
    for (i = ma = 0; i < bc; i++, buf++) {              /* by bytes */
        if ((ma & VA_M_OFF) == 0) {                     /* need map? */
            if (!qba_map_addr (ba + i, &ma))            /* inv or NXM? */
                return (bc - i);
            }
        *buf = ReadB (ma);
        ma = ma + 1;
        }
    }
else {
    for (i = ma = 0; i < bc; i = i + 4, buf++) {        /* by longwords */
        if ((ma & VA_M_OFF) == 0) {                     /* need map? */
            if (!qba_map_addr (ba + i, &ma))            /* inv or NXM? */
                return (bc - i);
            }
        dat = ReadL (ma);                               /* get lw */
        *buf++ = dat & BMASK;                           /* low 8b */
        *buf++ = (dat >> 8) & BMASK;                    /* next 8b */
        *buf++ = (dat >> 16) & BMASK;                   /* next 8b */
        *buf = (dat >> 24) & BMASK;
        ma = ma + 4;
        }
    }
return 0;
}

int32 Map_ReadW (uint32 ba, int32 bc, uint16 *buf)
{
int32 i;
uint32 ma,dat;

ba = ba & ~01;
bc = bc & ~01;
if ((ba | bc) & 03) {                                   /* check alignment */
    for (i = ma = 0; i < bc; i = i + 2, buf++) {        /* by words */
        if ((ma & VA_M_OFF) == 0) {                     /* need map? */
            if (!qba_map_addr (ba + i, &ma))            /* inv or NXM? */
                return (bc - i);
            }
        *buf = ReadW (ma);
        ma = ma + 2;
        }
    }
else {
    for (i = ma = 0; i < bc; i = i + 4, buf++) {        /* by longwords */
        if ((ma & VA_M_OFF) == 0) {                     /* need map? */
            if (!qba_map_addr (ba + i, &ma))            /* inv or NXM? */
                return (bc - i);
            }
        dat = ReadL (ma);                               /* get lw */
        *buf++ = dat & WMASK;                           /* low 16b */
        *buf = (dat >> 16) & WMASK;                     /* high 16b */
        ma = ma + 4;
        }
    }
return 0;
}

int32 Map_WriteB (uint32 ba, int32 bc, uint8 *buf)
{
int32 i;
uint32 ma, dat;

if ((ba | bc) & 03) {                                   /* check alignment */
    for (i = ma = 0; i < bc; i++, buf++) {              /* by bytes */
        if ((ma & VA_M_OFF) == 0) {                     /* need map? */
            if (!qba_map_addr (ba + i, &ma))            /* inv or NXM? */
                return (bc - i);
            }
        WriteB (ma, *buf);
        ma = ma + 1;
        }
    }
else {
    for (i = ma = 0; i < bc; i = i + 4, buf++) {        /* by longwords */
        if ((ma & VA_M_OFF) == 0) {                     /* need map? */
            if (!qba_map_addr (ba + i, &ma))            /* inv or NXM? */
                return (bc - i);
            }
        dat = (uint32) *buf++;                          /* get low 8b */
        dat = dat | (((uint32) *buf++) << 8);           /* merge next 8b */
        dat = dat | (((uint32) *buf++) << 16);          /* merge next 8b */
        dat = dat | (((uint32) *buf) << 24);            /* merge hi 8b */
        WriteL (ma, dat);                               /* store lw */
        ma = ma + 4;
        }
    }
return 0;
}

int32 Map_WriteW (uint32 ba, int32 bc, uint16 *buf)
{
int32 i;
uint32 ma, dat;

ba = ba & ~01;
bc = bc & ~01;
if ((ba | bc) & 03) {                                   /* check alignment */
    for (i = ma = 0; i < bc; i = i + 2, buf++) {        /* by words */
        if ((ma & VA_M_OFF) == 0) {                     /* need map? */
            if (!qba_map_addr (ba + i, &ma))            /* inv or NXM? */
                return (bc - i);
            }
        WriteW (ma, *buf);
        ma = ma + 2;
        }
    }
else {
    for (i = ma = 0; i < bc; i = i + 4, buf++) {        /* by longwords */
        if ((ma & VA_M_OFF) == 0) {                     /* need map? */
            if (!qba_map_addr (ba + i, &ma))            /* inv or NXM? */
                return (bc - i);
            }
        dat = (uint32) *buf++;                          /* get low 16b */
        dat = dat | (((uint32) *buf) << 16);            /* merge hi 16b */
        WriteL (ma, dat);                               /* store lw */
        ma = ma + 4;
        }
    }
return 0;
}

/* Memory examine via map (word only) */

t_stat qba_ex (t_value *vptr, t_addr exta, UNIT *uptr, int32 sw)
{
uint32 qa = (uint32) exta, pa;

if ((vptr == NULL) || (qa >= CQMSIZE)) return SCPE_ARG;
if (qba_map_addr_c (qa, &pa) && ADDR_IS_MEM (pa)) {
    *vptr = (uint32) ReadW (pa);
    return SCPE_OK;
    }
return SCPE_NXM;
}

/* Memory deposit via map (word only) */

t_stat qba_dep (t_value val, t_addr exta, UNIT *uptr, int32 sw)
{
uint32 qa = (uint32) exta, pa;

if (qa >= CQMSIZE) return SCPE_ARG;
if (qba_map_addr_c (qa, &pa) && ADDR_IS_MEM (pa)) {
    WriteW (pa, (int32) val);
    return SCPE_OK;
    }
return SCPE_NXM;
}

/* Enable/disable autoconfiguration */

t_stat set_autocon (UNIT *uptr, int32 val, char *cptr, void *desc)
{
if (cptr != NULL) return SCPE_ARG;
autcon_enb = val;
return auto_config (NULL, 0);
}

/* Show autoconfiguration status */

t_stat show_autocon (FILE *st, UNIT *uptr, int32 val, void *desc)
{
fprintf (st, "autoconfiguration ");
fprintf (st, autcon_enb? "enabled": "disabled");
return SCPE_OK;
}

/* Change device address */

t_stat set_addr (UNIT *uptr, int32 val, char *cptr, void *desc)
{
DEVICE *dptr;
DIB *dibp;
uint32 newba;
t_stat r;

if (cptr == NULL) return SCPE_ARG;
if ((val == 0) || (uptr == NULL)) return SCPE_IERR;
dptr = find_dev_from_unit (uptr);
if (dptr == NULL) return SCPE_IERR;
dibp = (DIB *) dptr->ctxt;
if (dibp == NULL) return SCPE_IERR;
newba = (uint32) get_uint (cptr, 16, IOPAGEBASE+IOPAGEMASK, &r);        /* get new */
if (r != SCPE_OK) return r;
if ((newba <= IOPAGEBASE) ||                            /* must be > 0 */
    (newba % ((uint32) val))) return SCPE_ARG;          /* check modulus */
dibp->ba = newba;                                       /* store */
dptr->flags = dptr->flags & ~DEV_FLTA;                  /* not floating */
autcon_enb = 0;                                         /* autoconfig off */
return SCPE_OK;
}

/* Show device address */

t_stat show_addr (FILE *st, UNIT *uptr, int32 val, void *desc)
{
DEVICE *dptr;
DIB *dibp;

if (uptr == NULL) return SCPE_IERR;
dptr = find_dev_from_unit (uptr);
if (dptr == NULL) return SCPE_IERR;
dibp = (DIB *) dptr->ctxt;
if ((dibp == NULL) || (dibp->ba <= IOPAGEBASE)) return SCPE_IERR;
fprintf (st, "address=%08X", dibp->ba);
if (dibp->lnt > 1)
    fprintf (st, "-%08X", dibp->ba + dibp->lnt - 1);
if (dptr->flags & DEV_FLTA) fprintf (st, "*");
return SCPE_OK;
}

/* Set address floating */

t_stat set_addr_flt (UNIT *uptr, int32 val, char *cptr, void *desc)
{
DEVICE *dptr;

if (cptr == NULL) return SCPE_ARG;
if ((val == 0) || (uptr == NULL)) return SCPE_IERR;
dptr = find_dev_from_unit (uptr);
if (dptr == NULL) return SCPE_IERR;
dptr->flags = dptr->flags | DEV_FLTA;                   /* floating */
return auto_config (NULL, 0);                           /* autoconfigure */
}

/* Change device vector */

t_stat set_vec (UNIT *uptr, int32 arg, char *cptr, void *desc)
{
DEVICE *dptr;
DIB *dibp;
uint32 newvec;
t_stat r;

if (cptr == NULL) return SCPE_ARG;
if (uptr == NULL) return SCPE_IERR;
dptr = find_dev_from_unit (uptr);
if (dptr == NULL) return SCPE_IERR;
dibp = (DIB *) dptr->ctxt;
if (dibp == NULL) return SCPE_IERR;
newvec = (uint32) get_uint (cptr, 16, VEC_Q + 01000, &r);
if ((r != SCPE_OK) || (newvec <= VEC_Q) ||
    ((newvec + (dibp->vnum * 4)) >= (VEC_Q + 01000)) ||
    (newvec & ((dibp->vnum > 1)? 07: 03))) return SCPE_ARG;
dibp->vec = newvec;
dptr->flags = dptr->flags & ~DEV_FLTA;                  /* not floating */
autcon_enb = 0;                                         /* autoconfig off */
return SCPE_OK;
}

/* Show device vector */

t_stat show_vec (FILE *st, UNIT *uptr, int32 arg, void *desc)
{
DEVICE *dptr;
DIB *dibp;
uint32 vec, numvec;

if (uptr == NULL) return SCPE_IERR;
dptr = find_dev_from_unit (uptr);
if (dptr == NULL) return SCPE_IERR;
dibp = (DIB *) dptr->ctxt;
if (dibp == NULL) return SCPE_IERR;
vec = dibp->vec;
if (arg) numvec = arg;
else numvec = dibp->vnum;
if (vec == 0) fprintf (st, "no vector");
else {
    fprintf (st, "vector=%X", vec);
    if (numvec > 1) fprintf (st, "-%X", vec + (4 * (numvec - 1)));
    }
return SCPE_OK;
}

/* Build dispatch tables */

t_stat build_dsp_tab (DEVICE *dptr, DIB *dibp)
{
uint32 i, idx;

if ((dptr == NULL) || (dibp == NULL)) return SCPE_IERR; /* validate args */
for (i = 0; i < dibp->lnt; i = i + 2) {                 /* create entries */
    idx = ((dibp->ba + i) & IOPAGEMASK) >> 1;           /* index into disp */
    if ((iodispR[idx] && dibp->rd &&                    /* conflict? */
        (iodispR[idx] != dibp->rd)) ||
        (iodispW[idx] && dibp->wr &&
        (iodispW[idx] != dibp->wr))) {
        printf ("Device %s address conflict at %08o\n",
            sim_dname (dptr), dibp->ba);
        if (sim_log) fprintf (sim_log,
            "Device %s address conflict at %08o\n",
            sim_dname (dptr), dibp->ba);
        return SCPE_STOP;
        }
    if (dibp->rd) iodispR[idx] = dibp->rd;              /* set rd dispatch */
    if (dibp->wr) iodispW[idx] = dibp->wr;              /* set wr dispatch */
    iodibp[idx] = dibp;                                 /* remember DIB */
    }
return SCPE_OK;
}


/* Build interrupt tables */

t_stat build_int_vec (DEVICE *dptr, DIB *dibp)
{
int32 i, idx, vec, ilvl, ibit;

if ((dptr == NULL) || (dibp == NULL)) return SCPE_IERR; /* validate args */
if (dibp->vnum > VEC_DEVMAX) return SCPE_IERR;
for (i = 0; i < dibp->vnum; i++) {                      /* loop thru vec */
    idx = dibp->vloc + i;                               /* vector index */
    vec = dibp->vec? (dibp->vec + (i * 4)): 0;          /* vector addr */
    ilvl = idx / 32;
    ibit = idx % 32;
    if ((int_ack[ilvl][ibit] && dibp->ack[i] &&         /* conflict? */
        (int_ack[ilvl][ibit] != dibp->ack[i])) ||
        (int_vec[ilvl][ibit] && vec &&
        (int_vec[ilvl][ibit] != vec))) {
        printf ("Device %s interrupt slot conflict at %d\n",
            sim_dname (dptr), idx);
        if (sim_log) fprintf (sim_log,
        "Device %s interrupt slot conflict at %d\n",
            sim_dname (dptr), idx);
        return SCPE_STOP;
        }
    if (dibp->ack[i]) int_ack[ilvl][ibit] = dibp->ack[i];
    else if (vec) int_vec[ilvl][ibit] = vec;
    }
return SCPE_OK;
}

/* Build dib_tab from device list */

t_stat build_dib_tab (void)
{
int32 i, j;
DEVICE *dptr;
DIB *dibp;
t_stat r;

for (i = 0; i < IPL_HLVL; i++) {                        /* clear int tables */
    for (j = 0; j < 32; j++) {
        int_vec[i][j] = 0;
        int_ack[i][j] = NULL;
        }
    }
for (i = 0; i < (IOPAGESIZE >> 1); i++) {               /* clear dispatch tab */
    iodispR[i] = NULL;
    iodispW[i] = NULL;
    iodibp[i] = NULL;
    }
for (i = 0; (dptr = sim_devices[i]) != NULL; i++) {     /* loop thru dev */
    dibp = (DIB *) dptr->ctxt;                          /* get DIB */
    if (dibp && !(dptr->flags & DEV_DIS)) {             /* defined, enabled? */
        if (r = build_int_vec (dptr, dibp))             /* add to intr tab */
            return r;
        if (r = build_dsp_tab (dptr, dibp))             /* add to dispatch tab */
            return r;
        }                                               /* end if enabled */
    }                                                   /* end for */
return SCPE_OK;
}

/* Show IO space */

t_stat show_iospace (FILE *st, UNIT *uptr, int32 val, void *desc)
{
uint32 i, j;
DEVICE *dptr;
DIB *dibp;

if (build_dib_tab ()) return SCPE_OK;                   /* build IO page */
for (i = 0, dibp = NULL; i < (IOPAGESIZE >> 1); i++) {  /* loop thru entries */
    if (iodibp[i] && (iodibp[i] != dibp)) {             /* new block? */
        dibp = iodibp[i];                               /* DIB for block */
        for (j = 0, dptr = NULL; sim_devices[j] != NULL; j++) {
            if (((DIB*) sim_devices[j]->ctxt) == dibp) {
                dptr = sim_devices[j];                  /* locate device */
                break;
                }                                       /* end if */
            }                                           /* end for j */
    fprintf (st, "%08X - %08X%c\t%s\n", dibp->ba,
            dibp->ba + dibp->lnt - 1,
            (dptr && (dptr->flags & DEV_FLTA))? '*': ' ',
            dptr? sim_dname (dptr): "CPU");
        }                                               /* end if */
    }                                                   /* end for i */
return SCPE_OK;
}

/* Show QBA virtual address */

t_stat qba_show_virt (FILE *of, UNIT *uptr, int32 val, void *desc)
{
t_stat r;
char *cptr = (char *) desc;
uint32 qa, pa;

if (cptr) {
    qa = (uint32) get_uint (cptr, 16, CQMSIZE - 1, &r);
    if (r == SCPE_OK) {
        if (qba_map_addr_c (qa, &pa))
            fprintf (of, "Qbus %-X = physical %-X\n", qa, pa);
        else fprintf (of, "Qbus %-X: invalid mapping\n", qa);
        return SCPE_OK;
        }
    }
fprintf (of, "Invalid argument\n");
return SCPE_OK;
}

/* Autoconfiguration

   The table reflects the MicroVAX 3900 microcode, with one addition - the
   number of controllers field handles devices where multiple instances
   are simulated through a single DEVICE structure (e.g., DZ, VH).

   A minus number of vectors indicates a field that should be calculated
   but not placed in the DIB (RQ, TQ dynamic vectors) */

#define AUTO_MAXC       4
#define AUTO_CSRBASE    0010
#define AUTO_VECBASE    0300

typedef struct {
    char        *dnam[AUTO_MAXC];
    int32       numc;
    int32       numv;
    uint32      amod;
    uint32      vmod;
    uint32      fixa[AUTO_MAXC];
    uint32      fixv[AUTO_MAXC];
    } AUTO_CON;

AUTO_CON auto_tab[] = {
    { { NULL }, 1, 2, 0, 8, { 0 } },                    /* DLV11J - fx CSRs */
    { { NULL }, 1, 2, 8, 8 },                           /* DJ11 */
    { { NULL }, 1, 2, 16, 8 },                          /* DH11 */
    { { NULL }, 1, 2, 8, 8 },                           /* DQ11 */
    { { NULL }, 1, 2, 8, 8 },                           /* DU11 */
    { { NULL }, 1, 2, 8, 8 },                           /* DUP11 */
    { { NULL }, 10, 2, 8, 8 },                          /* LK11A */
    { { NULL }, 1, 2, 8, 8 },                           /* DMC11 */
    { { "DZ" }, DZ_MUXES, 2, 8, 8 },                    /* DZ11 */
    { { NULL }, 1, 2, 8, 8 },                           /* KMC11 */
    { { NULL }, 1, 2, 8, 8 },                           /* LPP11 */
    { { NULL }, 1, 2, 8, 8 },                           /* VMV21 */
    { { NULL }, 1, 2, 16, 8 },                          /* VMV31 */
    { { NULL }, 1, 2, 8, 8 },                           /* DWR70 */
    { { "RL", "RLB" }, 1, 1, 8, 4, {IOBA_RL}, {VEC_RL} }, /* RL11 */
    { { "TS", "TSB", "TSC", "TSD" }, 1, 1, 0, 4,        /* TS11 */
        {IOBA_TS, IOBA_TS + 4, IOBA_TS + 8, IOBA_TS + 12},
        {VEC_TS} },
    { { NULL }, 1, 2, 16, 8 },                          /* LPA11K */
    { { NULL }, 1, 2, 8, 8 },                           /* KW11C */
    { { NULL }, 1, 1, 8, 8 },                           /* reserved */
    { { "RX", "RY" }, 1, 1, 8, 4, {IOBA_RX} , {VEC_RX} }, /* RX11/RX211 */
    { { NULL }, 1, 1, 8, 4 },                           /* DR11W */
    { { NULL }, 1, 1, 8, 4, { 0, 0 }, { 0 } },          /* DR11B - fx CSRs,vec */
    { { NULL }, 1, 2, 8, 8 },                           /* DMP11 */
    { { NULL }, 1, 2, 8, 8 },                           /* DPV11 */
    { { NULL }, 1, 2, 8, 8 },                           /* ISB11 */
    { { NULL }, 1, 2, 16, 8 },                          /* DMV11 */
    { { "XU", "XUB" }, 1, 1, 8, 4, {IOBA_XU}, {VEC_XU} }, /* DEUNA */
    { { "XQ", "XQB" }, 1, 1, 0, 4,                      /* DEQNA */
        {IOBA_XQ,IOBA_XQB}, {VEC_XQ} },
    { { "RQ", "RQB", "RQC", "RQD" }, 1, -1, 4, 4,       /* RQDX3 */
        {IOBA_RQ}, {VEC_RQ} },
    { { NULL }, 1, 8, 32, 4 },                          /* DMF32 */
    { { NULL }, 1, 2, 16, 8 },                          /* KMS11 */
    { { NULL }, 1, 1, 16, 4 },                          /* VS100 */
    { { "TQ", "TQB" }, 1, -1, 4, 4, {IOBA_TQ}, {VEC_TQ} }, /* TQK50 */
    { { NULL }, 1, 2, 16, 8 },                          /* KMV11 */
    { { "VH" }, VH_MUXES, 2, 16, 8 },                   /* DHU11/DHQ11 */
    { { NULL }, 1, 6, 32, 4 },                          /* DMZ32 */
    { { NULL }, 1, 6, 32, 4 },                          /* CP132 */
    { { NULL }, 1, 2, 64, 8, { 0 } },                   /* QVSS - fx CSR */
    { { NULL }, 1, 1, 8, 4 },                           /* VS31 */
    { { NULL }, 1, 1, 0, 4, { 0 } },                    /* LNV11 - fx CSR */
    { { NULL }, 1, 1, 16, 4 },                          /* LNV21/QPSS */
    { { NULL }, 1, 1, 8, 4, { 0 } },                    /* QTA - fx CSR */
    { { NULL }, 1, 1, 8, 4 },                           /* DSV11 */
    { { NULL }, 1, 2, 8, 8 },                           /* CSAM */
    { { NULL }, 1, 2, 8, 8 },                           /* ADV11C */
    { { NULL }, 1, 0, 8, 0 },                           /* AAV11C */
    { { NULL }, 1, 2, 8, 8, { 0 }, { 0 } },             /* AXV11C - fx CSR,vec */
    { { NULL }, 1, 2, 4, 8, { 0 } },                    /* KWV11C - fx CSR */
    { { NULL }, 1, 2, 8, 8, { 0 } },                    /* ADV11D - fx CSR */
    { { NULL }, 1, 2, 8, 8, { 0 } },                    /* AAV11D - fx CSR */
    { { "QDSS" }, 1, 3, 0, 16, {IOBA_QDSS} },           /* QDSS - fx CSR */
    { { NULL }, -1 }                                    /* end table */
};

t_stat auto_config (char *name, int32 nctrl)
{
uint32 csr = IOPAGEBASE + AUTO_CSRBASE;
uint32 vec = VEC_Q + AUTO_VECBASE;
AUTO_CON *autp;
DEVICE *dptr;
DIB *dibp;
uint32 j, k, vmask, amask;

if (autcon_enb == 0) return SCPE_OK;                    /* enabled? */
if (name) {                                             /* updating? */
    if (nctrl < 0) return SCPE_ARG;
    for (autp = auto_tab; autp->numc >= 0; autp++) {
        for (j = 0; (j < AUTO_MAXC) && autp->dnam[j]; j++) {
            if (strcmp (name, autp->dnam[j]) == 0)
                autp->numc = nctrl;
            }
        }
    }
for (autp = auto_tab; autp->numc >= 0; autp++) {        /* loop thru table */
    if (autp->amod) {                                   /* floating csr? */
        amask = autp->amod - 1;
        csr = (csr + amask) & ~amask;                   /* align csr */
        }
    for (j = k = 0; (j < AUTO_MAXC) && autp->dnam[j]; j++) {
        dptr = find_dev (autp->dnam[j]);                /* find ctrl */
        if ((dptr == NULL) || (dptr->flags & DEV_DIS) ||
            !(dptr->flags & DEV_FLTA)) continue;        /* enabled, floating? */
        dibp = (DIB *) dptr->ctxt;                      /* get DIB */
        if (dibp == NULL) return SCPE_IERR;             /* not there??? */
        if (autp->amod) {                               /* dyn csr needed? */
            if (autp->fixa[k])                          /* fixed csr avail? */
                dibp->ba = autp->fixa[k];               /* use it */
            else {                                      /* no fixed left */
                dibp->ba = csr;                         /* set CSR */
                csr += (autp->numc * autp->amod);       /* next CSR */
                }                                       /* end else */
            }                                           /* end if dyn csr */
        if (autp->numv && autp->vmod) {                 /* dyn vec needed? */
            uint32 numv = abs (autp->numv);             /* get num vec */
            if (autp->fixv[k]) {                        /* fixed vec avail? */
                if (autp->numv > 0)
                    dibp->vec = autp->fixv[k];          /* use it */
                }
            else {                                      /* no fixed left */
                vmask = autp->vmod - 1;
                vec = (vec + vmask) & ~vmask;           /* align vector */
                if (autp->numv > 0)
                    dibp->vec = vec;                    /* set vector */
                vec += (autp->numc * numv * 4);
                }                                       /* end else */
            }                                           /* end if dyn vec */
        k++;                                            /* next instance */
        }                                               /* end for j */
    if (autp->amod) csr = csr + 2;                      /* flt CSR? gap */
    }                                                   /* end for i */
return SCPE_OK;
}
