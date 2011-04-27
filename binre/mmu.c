/* mmu.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "binre.h"
#include "mach.h"
#include "isn.h"
#include "support.h"

#define MMU_1134
//#define MMU_1170

#ifdef MMU_1134
 // 11/34a values
 #define NO_SUPER
 #define PDR_MASK	0077716 /* acf low bit gone */
 #define PDR_W_MASK	0077416
 #define PAR_MASK	0007777 /* 12 bits */
 #define PAR_W_MASK	0177777
#endif

extern int debug;

u16 mmr0, mmr1, mmr2, mmr3;

u16 par[64];
u16 pdr[64];

#define byte_place(addr, old, byte) \
    (((addr) & 1) ? ((old) & 0377) | ((byte) << 8) : ((old) & ~0377) | (byte))

u16 mmu_read_parpdr(int addr, int index)
{
    int pxr_addr;

    pxr_addr =
        (index << 4) |
        ((addr >> 1) & 017);

    if (addr & 040) {
        if (debug) printf("mmu: read par/pdr %o (%o); par[%o] -> %o\n",
                          addr, index, pxr_addr, par[pxr_addr]);
        return par[pxr_addr] & PAR_MASK;
    } else {
        if (debug) printf("mmu: read par/pdr %o (%o); pdr[%o] -> %o\n",
                          addr, index, pxr_addr, pdr[pxr_addr]);
        return pdr[pxr_addr] & PDR_MASK;
    }

    return 0;
}

void mmu_write_parpdr(int addr, int index, u16 data, int writeb)
{
    int pxr_addr;

    if (debug) printf("mmu: write par/pdr %o (%o) <- %o\n",
                      addr, index, data);

    pxr_addr =
        (index << 4) |
        ((addr >> 1) & 017);

    if (addr & 040) {
        if (debug) printf("mmu: write par/pdr %o (%o); par[%o] <- %o\n",
                          addr, index, pxr_addr, data);

        if (writeb) {
            u16 old = par[pxr_addr];

            if (addr & 1)
                data = (old & 0xff) | (data << 8);
            else
                data = (old & 0xff00) | (data & 0xff);
        }

        par[pxr_addr] = data & PAR_W_MASK;;
    } else {
        if (debug) printf("mmu: write par/pdr %o (%o); pdr[%o] -> %o\n",
                          addr, index, pxr_addr, data);


        if (writeb) {
            u16 old = pdr[pxr_addr];

            if (addr & 1)
                data = (old & 0xff) | (data << 8);
            else
                data = (old & 0xff00) | (data & 0xff);
        }

        pdr[pxr_addr] = data & PDR_W_MASK;
    }
}

u16 mmu_read_reg(int addr)
{
    if (debug) printf("mmu: read reg %o \n", addr);

    switch (addr) {
    case IOBASE_MMR0: return mmr0;
    case IOBASE_MMR1: return mmr1;
    case IOBASE_MMR2: return mmr2;
    case IOBASE_MMR3: return mmr3;
    }

    return 0;
}

void mmu_write_reg(int addr, u16 data, int writeb)
{
    if (debug) printf("mmu: write reg %o <- %o\n", addr, data);

    if (writeb) {
        data &= 0377;
        switch (addr) {
        case IOBASE_MMR0: mmr0 = byte_place(addr, mmr0, data); break;
        case IOBASE_MMR3: mmr3 = byte_place(addr, mmr3, data); break;
        }
        return;
    }

    switch (addr) {
    case IOBASE_MMR0: mmr0 = data; break;
    case IOBASE_MMR3: mmr3 = data; break;
    }
}

#define mmu_on		(mmr0&(1<<0))
#define maint_mode	(mmr0&(1<<8))

#ifdef MMU_1170
#define traps_enabled	(mmr0&(1<<9))
#else
#define traps_enabled (1)
#endif

int
mmu_map(int cpu_mode, int cpu_fetch, int cpu_write, int cpu_trap, int trap_odd,
        int vaddr, int *ppaddr)
{
    unsigned cpu_apf, cpu_df, cpu_bn;
    unsigned pxr_index;
    char enable_d_space, map_address, va_is_iopage, pa_is_iopage;
    
    unsigned pdr_value, par_value, cpu_paf;
    unsigned map_adder_22, map_adder, mapped_pa_22, unmapped_pa_22, cpu_pa;
    
    int pdr_plf, pdr_ed, pdr_w, pdr_acf;
    char pg_len_err, update_pdr;
    char update_mmr0_page, update_mmr0_nonres, update_mmr0_ple;
    char update_mmr0_ro, update_mmr0_trap_flag;
    char pdr_update_a, pdr_update_w;
    char signal_abort, signal_trap;

#ifdef NO_SUPER
    if (cpu_fetch &&
        !( (mmr0&(1<<15)) | (mmr0&(1<<14)) | (mmr0&(1<<13)) || cpu_mode == 1) )
        mmr2 = vaddr;
#else
    if (cpu_fetch &&
        !((mmr0&(1<<15) | (mmr0&(1<<14)) | (mmr0&(1<<13)))))
        mmr2 = vaddr;
#endif

    int cpu_i_access = cpu_fetch;
    int cpu_d_access = !cpu_fetch;
//need to fix mach.c to generate proper cpu_trap

    if (!mmu_on) {
        *ppaddr = vaddr;
        return 0;
    }

    cpu_apf = (vaddr >> 13) & 7;
    cpu_df = vaddr & 017777;
    cpu_bn = (cpu_df >> 6) & 0177;

    // allow for split i & d
    enable_d_space = 
        cpu_mode == 0 ? (mmr3&(1<<2)) :
        cpu_mode == 1 ? (mmr3&(1<<1)) :
        cpu_mode == 3 ? (mmr3&(1<<0)) :
        0;

    pxr_index =
        ((cpu_trap ? 0 : cpu_mode) << 4) |
        ((enable_d_space ? !cpu_i_access : 0) << 3) |
        ((cpu_apf) << 0);

    int map22 = 0;

    // enable mapping if mmu on or using maint-mode w/destination access
    map_address = mmu_on || (maint_mode & cpu_d_access);

    pdr_value = pdr[pxr_index] & PDR_MASK;
    par_value = par[pxr_index] & PAR_MASK;

    cpu_paf = par_value;

    // form 22 bit physical address
    map_adder_22 = (cpu_paf << 6) + cpu_df;

    // compensate if doing 18 bit mapping
    map_adder = map22 ? map_adder_22 : (map_adder_22&0777777);

    // decide if va is in iopage
    va_is_iopage = ((vaddr >> 13)&7) == 3;	/* 8k io page */
   
    // complete translation of 16 bit va to 22 bit pa, unmapped
    unmapped_pa_22 = va_is_iopage ? ((077<<16) | vaddr) : vaddr;

    // map 18 bit iopage to 22 bit iopage if only doing 18 bit mapping
    // (iopage.v expects full 22 bit mapping - see bus.v)
    pa_is_iopage = !map22 && (((map_adder>>13)&037) == 037);
   
    // complete translation of 16 bit va to 22 bit pa, mapped
    mapped_pa_22 = pa_is_iopage ? ((077<<16) | (map_adder&0xffff)) : map_adder;
			 
    // pick va or mapped address
    cpu_pa = map_address ? mapped_pa_22 : unmapped_pa_22;
   
    pdr_plf = (pdr_value>>8)&0177;
    pdr_ed = (pdr_value>>3)&1;
    pdr_w = (pdr_value>>6)&1;
    pdr_acf = pdr_value&7;
   
    // check bn against page length
    pg_len_err = pdr_ed ? cpu_bn < pdr_plf : cpu_bn > pdr_plf;

#if 1
    printf("mmu_map: pxr_index %o pdr_value %o\n", pxr_index, pdr_value);
    printf("mmu_map: pg_len_err %d; pdr_ed %d, cpu_bn %o, pdf_plf %o\n",
           pg_len_err, pdr_ed, cpu_bn, pdr_plf);
#endif

    //
    update_pdr = 0;

    pdr_update_a = 0;
    pdr_update_w = 0;

    update_mmr0_nonres = 0;
    update_mmr0_ple = 0;
    update_mmr0_ro = 0;
    update_mmr0_trap_flag = 0;
    update_mmr0_page = 0;

    signal_abort = 0;
    signal_trap = 0;


    printf("zzz: vaddr %o, pxr_index %o, cpu_write %d, mapped_pa_22 %o, acf %o (cpu_paf<<6 %o, cpu_df %o)\n",
           vaddr, pxr_index, cpu_write, mapped_pa_22, pdr_acf, cpu_paf << 6, cpu_df);
    fflush(stdout);

    if (cpu_write) {
        switch (pdr_acf) {
        case 0:	// non-res, unused, unused
        case 3:
        case 7:
            update_pdr = 1;
            update_mmr0_page = 1;
            update_mmr0_nonres = 1;
            if (pg_len_err)
                update_mmr0_ple = 1;
            signal_abort = 1;
            if (debug) printf("zzz: acf=%o, signal abort, wr non-res\n", pdr_acf);
            break;

        case 1:	// read-only
        case 2:
            update_pdr = 1;
            update_mmr0_page = 1;
            update_mmr0_ro = 1;
            if (pg_len_err)
                update_mmr0_ple = 1;
            signal_abort = 1;
            if (debug) printf("zzz: acf=%o, signal abort, wr r-o\n", pdr_acf);
            break;

        case 4: // read/write
            update_pdr = 1;
            pdr_update_a = 1;	// set a bit
#ifdef MMU_1134
            //on 11/34, abort non-res
            update_mmr0_nonres = 1;
            update_mmr0_page = 1;
            signal_trap = 1;
            if (debug) printf("zzz: acf=%o, signal trap, wr unused\n", pdr_acf);
#endif
#ifdef MMU_1170
            if (traps_enabled)	// trap enable
            {
                update_mmr0_page = 1;
                update_mmr0_trap_flag = 1;
                signal_trap = 1;
                if (debug)
                    printf("zzz: acf=%o, signal trap, wr unused\n", pdr_acf);
            }
#endif
            break;

        case 5:		// read/write
#ifdef MMU_1170
            update_pdr = 1;
            pdr_update_a = 1;	// set a bit
            if (traps_enabled)	// trap enable
            {
                update_mmr0_page = 1;
                update_mmr0_trap_flag = 1;
                signal_trap = 1;
                if (debug)
                    printf("zzz: acf=%o, signal trap, wr r/w\n", pdr_acf);
            }
#endif
            break;
	    
        case 6:		// read/write (ok)
            if (debug)
                printf("zzz: acf=6, set w; index %o, trap %o, cm %o\n",
                       pxr_index, cpu_trap, cpu_mode);

            update_pdr = 1;
            update_mmr0_page = 1;
//XXX added, not in rtl, for diag
            if (~trap_odd && (mapped_pa_22 != IOBASE_MMR0))
                pdr_update_w = 1;	// set w bit
            if (pg_len_err)
            {
                update_mmr0_ple = 1;
                signal_trap = 1;
                if (debug)
                    printf("zzz: signal trap, wr len\n");
            }
            break;
        }
    }

    if (!cpu_write) {
        switch (pdr_acf){
        case 0:
        case 3:
        case 7:		// non-res, unused, unused
            update_mmr0_nonres = 1;
            update_mmr0_page = 1;
            if (pg_len_err)
            {
                update_mmr0_ple = 1;
                signal_abort = 1;
                if (debug)
                    printf("zzz: acf=%o, signal abort, rd non-res + ple\n",
                           pdr_acf);
            }
            else
            {
                signal_abort = 1;
                if (debug) {
                    printf("zzz: acf=%o, signal abort, rd non-res "
                           "(pxr_index=%o, cm %o, d_space %o, apf %o)\n",
                           pdr_acf,
                           pxr_index, cpu_mode, enable_d_space, cpu_apf);
                    printf("ZZZ: va %o, pa %o, cm %o, i %o, pxr_index %o\n",
                           vaddr, cpu_pa, cpu_mode, cpu_i_access, pxr_index);
                }
            }
            break;

        case 1:		// read-only
#ifdef MMU_1170
            update_pdr = 1;
            pdr_update_a = 1;	// set a bit
            if (traps_enabled) 	// trap enable
            {
                update_mmr0_page = 1;
                update_mmr0_trap_flag = 1;
                signal_trap = 1;
                if (debug)
                    printf("zzz: acf=%o, signal trap, rd r-o\n", pdr_acf);
            }
#endif
            break;

        case 4:		// read/write
            update_pdr = 1;
            pdr_update_a = 1;	// set a bit

#if 1
            //XXXX rtl doesn't have this; diags FKABD0 wanted it
            if (pg_len_err) {
                update_mmr0_ple = 1;
            }
#endif

#ifdef MMU_1134
            update_mmr0_page = 1;
            update_mmr0_nonres = 1;
            signal_trap = 1;
            if (debug)
                printf("zzz: acf=%o, signal trap, rd r-w\n", pdr_acf);
#endif
#ifdef MMU_1170
            if (traps_enabled) 	// trap enable
            {
                update_mmr0_page = 1;
                update_mmr0_trap_flag = 1;
                signal_trap = 1;
                if (debug)
                    printf("zzz: acf=%o, signal trap, rd r-w\n", pdr_acf);
            }
#endif
            break;

        case 2:
        case 5:
        case 6:		// read-only, read/write, read/write
            update_mmr0_page = 1;
		   
            if (pg_len_err)
            {
                update_mmr0_ple = 1;
                signal_abort = 1;
#ifdef DEBUG_MMU
                printf("zzz: acf=%o, signal abort, rd len; ", pdr_acf);
                printf("pdr_ed %b, cpu_bn %o, plr_plf %o; <%b >%b\n",
                       pdr_ed, cpu_bn, pdr_plf,
                       cpu_bn < pdr_plf ? 1 : 0,
                       cpu_bn > pdr_plf ? 1 : 0);
#endif
            }
            break;
        }
    }

    if (update_pdr) {
        unsigned pdr_update_value =
            pdr_value | (pdr_update_a << 7) | (pdr_update_w << 6);

        pdr[pxr_index] = pdr_update_value;
    }

#if 1
    printf("mmu_map() nonres %d ple %d ro %d trap %d page %o, mmr0 %o\n",
           update_mmr0_nonres, update_mmr0_ple, update_mmr0_ro,
           update_mmr0_trap_flag, update_mmr0_page,
           (mmr0&(1<<15)) | (mmr0&(1<<14)) | (mmr0&(1<<13)));
#endif

    // update mmr0 if requested,
    //  but only if there are no error bits set
    if ((update_mmr0_nonres ||
         update_mmr0_ple ||
         update_mmr0_ro ||
         update_mmr0_trap_flag ||
         update_mmr0_page) &&
        !((mmr0&(1<<15)) | (mmr0&(1<<14)) | (mmr0&(1<<13))))
    {
        u16 old_mmr0 = mmr0;

        mmr0 =
            (update_mmr0_nonres ?    (1<<15) : (mmr0&(1<<15))) |
            (update_mmr0_ple ?       (1<<14) : (mmr0&(1<<14))) |
            (update_mmr0_ro ?        (1<<13) : (mmr0&(1<<13))) |
            (update_mmr0_trap_flag ? (1<<12) : (mmr0&(1<<12))) |
            (mmr0&(037<<7)) |
            (update_mmr0_page ?      (cpu_mode<<5) : (mmr0&(2<<5))) |
            (mmr0&(1<<4)) |
            (update_mmr0_page ?      (cpu_apf<<1) : (mmr0&(7<<1))) |
            (mmr0&(1<<0));


        if (debug) {
            if (mmr0 != old_mmr0)
                printf("mmu: update mmr0 <- %o\n", mmr0);
        }
    }

    if (signal_abort) {
        mmu_signals_abort();
        return -1;
    }

    if (signal_trap) {
        mmu_signals_trap();
        return -1;
    }

    *ppaddr = cpu_pa;

    return 0;
}

void
mmu_reset(void)
{
    mmr0 &= ~((1<<15) | (1<<14) | (1<<13));
    mmr0 &= ~(1<<8);
}


/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
