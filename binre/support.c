/* support.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "binre.h"
#include "mach.h"
#include "isn.h"
#include "support.h"

int support_int_bits;

int support_tto_delay;

extern int assert_int;
extern u16 assert_int_vec;
extern u16 assert_int_ipl_bits;
extern int debug;

void support_clear_int_bits(void)
{
assert_int_vec = 0;
assert_int_ipl_bits = 0;
    support_int_bits = 0;
}

void cpu_int_set(int bit)
{
    printf("cpu_int_set(%d)\n", bit);

    if (bit >= 0)
        support_int_bits |= 1 << bit;

    /* flag cpu here */
    assert_int = 1;
    if (support_int_bits & 1) {
        assert_int_vec = VECTOR_CLK;
        assert_int_ipl_bits = 1 << IPL_CLK;
    } else
    if (support_int_bits & 2) {
        assert_int_vec = VECTOR_TTI;
        assert_int_ipl_bits = 1 << IPL_TTI;
    } else
    if (support_int_bits & 4) {
        assert_int_vec = VECTOR_TTO;
        assert_int_ipl_bits = 1 << IPL_TTO;
    } else
    if (support_int_bits & 8) {
        assert_int_vec = VECTOR_RK;
        assert_int_ipl_bits = 1 << IPL_RK;
    }

    if (assert_int_ipl_bits) {
        printf("cpu_int_set; vector %o, ipl bits 0%o\n", 
               assert_int_vec, assert_int_ipl_bits);
    }
}

void cpu_int_clear(int bit)
{
    printf("cpu_int_clear(%o)\n", bit);

    support_int_bits &= ~(1 << bit);
    if (support_int_bits == 0)
        assert_int = 0;
    else
        cpu_int_set(-1);
}

static u16 clk_csr;
static u16 pclk_csr;
static u16 pclk_ctr;
static u16 pclk_csb;

static u16 tti_csr;
static u16 tto_csr;
static u8 tti_data;
static u8 tto_data;

#define CSR_GO		(1 << 0)
#define CSR_IE		(1 << 6)
#define CSR_DONE	(1 << 7)
#define CSR_BUSY	(1 << 11)
#define CSR_ERR		(1 << 15)

char tti_buffer[256];
int tti_count;
int tti_index;
int tti_polls;

void tti_poll(void)
{
    if (tti_polls == 0) {
        strcpy(tti_buffer, "rkunix.40\r");
        //strcpy(tti_buffer, "START\r01-JAN-85\r10:10\r");
        //strcpy(tti_buffer, "rk(0,0)rknix\r");
        tti_count = strlen(tti_buffer);
        tti_index = 0;
    }
    tti_polls++;

    if (tti_index < tti_count) {
        if ((tti_csr & CSR_DONE) == 0) {
            tti_csr |= CSR_DONE;
            tti_data = tti_buffer[tti_index++];
        }
    }
}

u16 io_tti_read(u32 addr)
{
    printf("io_tti_read(%o)\n", addr);
    tti_poll();
    if (addr & 2) {
        tti_csr = tti_csr & ~CSR_DONE;
        cpu_int_clear(1);
        return tti_data;
    } else {
        return tti_csr & (CSR_DONE | CSR_IE);
    }

}

void io_tti_write(u32 addr, u16 data)
{
    printf("io_tti_write() addr=%o, data=%o\n", addr, data);
    if ((addr & 2) == 0) {
        if (addr & 1)
            return;

        if ((data & CSR_IE) == 0)
            cpu_int_clear(1);
        else
            if ((tti_csr & (CSR_DONE | CSR_IE)) == CSR_DONE)
                cpu_int_set(1);

        tti_csr = (tti_csr & ~CSR_IE) | (data & CSR_IE);
    }
}

void io_tto_count(void)
{
    if (support_tto_delay) {
        support_tto_delay--;
        if (support_tto_delay == 0) {
            printf("io_tto_count: tto delay expired\n");
            tto_csr |= CSR_DONE;
            if (tto_csr & CSR_IE) cpu_int_set(2);
        }
    }
}

u16 io_tto_read(u32 addr)
{
    printf("io_tto_read(%o)\n", addr);
    if (addr & 2) {
        return tto_data;
    } else {
        return tto_csr & (CSR_DONE | CSR_IE);
    }
}

void io_tto_write(u32 addr, u16 data)
{
    printf("io_tto_write(%o) %o\n", addr, data);
    if (addr & 2) {
        if ((addr & 1) == 0) {
//            printf("TTO %o %c\n", data, data);
            printf("tto_data %o %c\n", data, data);
            tto_data = data;
        }
        tto_csr = tto_csr & ~CSR_DONE;
        cpu_int_clear(2);

#if 0
tto_csr |= CSR_DONE;
if (tto_csr & CSR_IE) cpu_int_set(2);
#else
support_tto_delay = 100/*4*/;
#endif
    } else {
        if (addr & 1)
            return;

    set:
        if ((data & CSR_IE) == 0)
            cpu_int_clear(2);
        else
            if ((tto_csr & (CSR_DONE | CSR_IE)) == CSR_DONE)
                cpu_int_set(2);

        tto_csr = (tto_csr & ~CSR_IE) | (data & CSR_IE);
    }
}

u16 io_sr_read(u32 addr)
{
    return 0;
}

u16 io_psw_read(u32 addr)
{
    extern u16 psw;
    printf("psw: read\n");
    return psw;
}

void io_psw_write(u32 addr, u16 data, int writeb)
{
    extern u16 psw;
    u16 data_w_tbit;
    printf("psw: write; addr %o, data %o, writeb %d\n", addr, data, writeb);

    data_w_tbit = (data & ~020) | (psw & 020);

    if (writeb) {
        if (addr & 1)
            psw = (psw & 0xff) | (data << 8);
        else
            psw = (psw & 0xff00) | (data_w_tbit & 0xff);
    } else
        psw = data_w_tbit;

    m_psw_changed();
    printf("psw: new %o\n", psw);
}

u16 io_clk_read(u32 addr)
{
    printf("io_clk_read(%o) -> %o\n", addr, clk_csr);
    return clk_csr;
}

void io_clk_write(u32 addr, u16 data)
{
    if (addr & 1)
        return;

    if (debug) printf("clk: csr <- %o\n", data);

    clk_csr = (clk_csr & ~CSR_IE) | (data & CSR_IE);

    if ((data & CSR_DONE) == 0)
        clk_csr &= ~CSR_DONE;

    if ((clk_csr & CSR_IE) == 0 ||
        (clk_csr & CSR_DONE) == 0)
    {
        cpu_int_clear(0);
    }
}

#define IO_CLK_MAX 8000

unsigned long io_clk_cnt;

void io_clk_count(void)
{
    io_clk_cnt++;
    if (io_clk_cnt > IO_CLK_MAX) {
        io_clk_cnt = 0;
        if (debug) printf("clk: done\n");
        clk_csr |= CSR_DONE;
        if (clk_csr & CSR_IE) {
            cpu_int_set(0);
        }
    }
}

u16 io_pclk_read(u32 addr)
{
    u16 v;
printf("io_pclk_read %o\n", addr);
    switch ((addr >> 1) & 3) {
    case 0:
        v = pclk_csr;
        pclk_csr &= ~(CSR_ERR | CSR_DONE);
        cpu_int_clear(0);
        return v;
    case 1:
        return 0;
    case 2:
        return pclk_ctr & 0100377;
    }
}

void io_pclk_write(u32 addr, u16 data)
{
    switch ((addr >> 1) & 3) {
    case 0:
        pclk_csr = data & 0137;
        cpu_int_clear(0);
        break;
    case 1:
        pclk_csb = data;
        pclk_ctr = data;
        pclk_csr &= ~(CSR_ERR | CSR_DONE);
        cpu_int_clear(0);
        break;
    case 2:
        break;
    }
}

int io_read(u32 addr, u16 *pval)
{
    printf("io_read(addr=%o)\n", addr);

    if (addr >= IOBASE_TTI && addr < IOBASE_TTI+4) {
	*pval = io_tti_read(addr);
        return 0;
    }

    if (addr >= IOBASE_TTO && addr < IOBASE_TTO+4) {
	*pval = io_tto_read(addr);
        return 0;
    }

    if (addr >= IOBASE_CLK && addr < IOBASE_CLK+2) {
	*pval = io_clk_read(addr);
        return 0;
    }

    if (addr >= IOBASE_SR && addr < IOBASE_SR+2) {
        *pval = io_sr_read(addr);
        return 0;
    }

    if (addr >= IOBASE_PSW && addr < IOBASE_PSW+2) {
        *pval = io_psw_read(addr);
        return 0;
    }

#ifdef PCLK
    if (addr >= IOBASE_PCLK && addr < IOBASE_PCLK+4)
	return io_pclk_read(addr);
#endif

    if (addr >= IOBASE_RK && addr < IOBASE_RK+32) {
	*pval = io_rk_read(addr);
        return 0;
    }

    if (addr >= IOBASE_RL && addr < IOBASE_RL+32) {
	*pval = io_rl_read(addr);
        return 0;
    }

    if (addr >= IOBASE_UPARPDR && addr < IOBASE_UPARPDR+0100) {
	*pval = mmu_read_parpdr(addr, 3);
        return 0;
    }

    if (addr >= IOBASE_SPARPDR && addr < IOBASE_SPARPDR+0100) {
	*pval = mmu_read_parpdr(addr, 1);
        return 0;
    }

    if (addr >= IOBASE_KPARPDR && addr < IOBASE_KPARPDR+0100) {
	*pval = mmu_read_parpdr(addr, 0);
        return 0;
    }

    if (addr == IOBASE_MMR0 ||
        addr == IOBASE_MMR1 ||
        addr == IOBASE_MMR2 ||
        addr == IOBASE_MMR3)
    {
	*pval = mmu_read_reg(addr);
        return 0;
    }

    if (addr == 017777776) {
        return 0;
    }

//    if (addr == 017776710) return 0300;    /* rm03? */
//    if (addr == 017777170) return 0100040; /* rx11 */
//    if (addr == 017777460) return 0;  /* rf11 */
//    if (addr == 017777440) return 0200; /* rk611 */

    support_signals_bus_error(addr);
    *pval = 0xffff;

    return -1;
}

int io_write(u32 addr, u16 data, int writeb)
{
    printf("io_write(addr=%o, data=%o, writeb=%d)\n", addr, data, writeb);

    if (addr >= IOBASE_TTI && addr < IOBASE_TTI+4) {
	io_tti_write(addr, data);
        return 0;
    }

    if (addr >= IOBASE_TTO && addr < IOBASE_TTO+4) {
	io_tto_write(addr, data);
        return 0;
    }

    if (addr >= IOBASE_CLK && addr < IOBASE_CLK+2) {
	io_clk_write(addr, data);
        return 0;
    }

    if (addr >= IOBASE_SR && addr < IOBASE_SR+2) {
        return 0;
    }

    if (addr >= IOBASE_PSW && addr < IOBASE_PSW+2) {
        io_psw_write(addr, data, writeb);
        return 0;
    }

#ifdef PCLK
    if (addr >= IOBASE_PCLK && addr < IOBASE_PCLK+4) {
	io_pclk_write(addr, data);
        return;
    }
#endif

    if (addr >= IOBASE_RK && addr < IOBASE_RK+32) {
	io_rk_write(addr, data, writeb);
        return 0;
    }

    if (addr >= IOBASE_RL && addr < IOBASE_RL+32) {
	io_rl_write(addr, data, writeb);
        return 0;
    }

    if (addr >= IOBASE_UPARPDR && addr < (IOBASE_UPARPDR+0100)) {
	mmu_write_parpdr(addr, 3, data, writeb);
        return 0;
    }

    if (addr >= IOBASE_SPARPDR && addr < (IOBASE_SPARPDR+0100)) {
	mmu_write_parpdr(addr, 1, data, writeb);
        return 0;
    }

    if (addr >= IOBASE_KPARPDR && addr < (IOBASE_KPARPDR+0100)) {
	mmu_write_parpdr(addr, 0, data, writeb);
        return 0;
    }

    if (addr == IOBASE_MMR0 ||
        addr == IOBASE_MMR1 ||
        addr == IOBASE_MMR2 ||
        addr == IOBASE_MMR3)
    {
	mmu_write_reg(addr, data, writeb);
        return 0;
    }

    support_signals_bus_error(addr);

    return -1;
}

void
poll_support(void)
{
    io_clk_count();
#ifdef PCLK
    io_pclk_count();
#endif
    io_tto_count();
}

extern char *image_filename;
extern int use_rl02;
extern int use_rk05;

void
reset_support(void)
{
    if (use_rk05)
        io_rk_reset(image_filename);

    if (use_rl02)
        io_rl_reset(image_filename);

    tto_csr = CSR_DONE;
    tti_csr = 0;
    clk_csr = CSR_DONE;
    pclk_csr = 0;
}


/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
