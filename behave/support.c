/* support.c */

#include <stdio.h>

#include "cpu.h"
#include "support.h"

int support_int_bits;

extern wire assert_int;
extern wire16 assert_int_vec;
extern wire16 assert_int_ipl;

void cpu_int_set(int flag)
{
    printf("cpu_int_set(%o)\n", flag);

    support_int_bits |= flag;
    /* flag cpu here */
    assert_int = 1;
    if (support_int_bits & 1) {
        assert_int_vec = VECTOR_CLK;
        assert_int_ipl = 1 << IPL_CLK;
    }
    if (support_int_bits & 2) {
        assert_int_vec = VECTOR_TTI;
        assert_int_ipl = 1 << IPL_TTI;
    }
    if (support_int_bits & 4) {
        assert_int_vec = VECTOR_TTO;
        assert_int_ipl = 1 << IPL_TTO;
    }
    if (support_int_bits & 8) {
        assert_int_vec = VECTOR_RK;
        assert_int_ipl = 1 << IPL_RK;
    }
}

void cpu_int_clear(int flag)
{
    printf("cpu_int_clear(%o)\n", flag);

    support_int_bits &= ~flag;
    if (support_int_bits == 0)
        assert_int = 0;
    else
        cpu_int_set(0);
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


u16 io_tti_read(u22 addr)
{
    printf("io_tti_read(%o)\n", addr);
    if (addr & 2) {
        tti_csr = tti_csr & ~CSR_DONE;
        cpu_int_clear(1);
        return tti_data;
    } else {
        return tti_csr & (CSR_DONE | CSR_IE);
    }
}

void io_tti_write(u22 addr, u16 data)
{
    printf("io_tti_write() %o\n", data);
    if ((addr & 2) == 0) {
        if ((addr & 1) == 0) {
            if ((data & CSR_IE) == 0)
                cpu_int_clear(1);
            else
                if ((tti_csr & (CSR_DONE | CSR_IE)) == CSR_DONE)
                    cpu_int_set(1);

            tti_csr = (tti_csr & ~CSR_IE) | (data & CSR_IE);
        }
    }
}

u16 io_tto_read(u22 addr)
{
    printf("io_tto_read(%o)\n", addr);
    if (addr & 2) {
        return tto_data;
    } else {
        return tto_csr & (CSR_DONE | CSR_IE);
    }
}

void io_tto_write(u22 addr, u16 data)
{
    printf("io_tto_write(%o) %o\n", addr, data);
    if (addr & 2) {
        if ((addr & 1) == 0) {
            printf("TTO %o %c\n", data, data);
            tto_data = data;
        }
        tto_csr = tto_csr & ~CSR_DONE;
        cpu_int_clear(2);
    } else {
        if ((addr & 1) == 0) {
            if (data & CSR_IE)
                cpu_int_clear(2);
            else
                if ((tto_csr & (CSR_DONE | CSR_IE)) == CSR_DONE)
                    cpu_int_set(2);

            tto_csr = (tto_csr & ~CSR_IE) | (data & CSR_IE);
        }
    }
}

u16 io_sr_read(u22 addr)
{
    return 0;
}

u16 io_psw_read(u22 addr)
{
    extern u16 psw;
    printf("psw: read\n");
    return psw;
}

void io_psw_write(u22 addr, u16 data)
{
    extern u16 psw;
    printf("psw: write\n");
    psw = data;
}

u16 io_clk_read(u22 addr)
{
    return clk_csr;
}

void io_clk_write(u22 addr, u16 data)
{
    if (addr & 1)
        return;

    clk_csr = (clk_csr & ~CSR_IE) | (data & CSR_IE);

    if ((data & CSR_DONE) == 0)
        clk_csr &= ~CSR_DONE;

    if ((clk_csr & CSR_IE) == 0 ||
        (clk_csr & CSR_DONE) == 0)
    {
        cpu_int_clear(0);
    }
}

u16 io_pclk_read(u22 addr)
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

void io_pclk_write(u22 addr, u16 data)
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

u16 io_read(u22 addr)
{
    if (addr >= IOBASE_TTI && addr < IOBASE_TTI+4)
	return io_tti_read(addr);

    if (addr >= IOBASE_TTO && addr < IOBASE_TTO+4)
	return io_tto_read(addr);

    if (addr >= IOBASE_CLK && addr < IOBASE_CLK+4)
	return io_clk_read(addr);

    if (addr >= IOBASE_SR && addr < IOBASE_SR+2)
        return io_sr_read(addr);

    if (addr >= IOBASE_PSW && addr < IOBASE_PSW+2) {
        return io_psw_read(addr);
    }

//    if (addr >= IOBASE_PCLK && addr < IOBASE_PCLK+4)
//	return io_pclk_read(addr);

    if (addr >= IOBASE_RK && addr < IOBASE_RK+32)
	return io_rk_read(addr);

    if (addr == 017777776)
        return 0;

    support_signals_bus_error();

    return 0xffff;
}

void io_write(u22 addr, u16 data, int writeb)
{
    printf("io_write(addr=%o)\n", addr);

    if (addr >= IOBASE_TTI && addr <= IOBASE_TTI+4) {
	return io_tti_write(addr, data);
    }

    if (addr >= IOBASE_TTO && addr < IOBASE_TTO+4) {
	return io_tto_write(addr, data);
    }

    if (addr >= IOBASE_CLK && addr < IOBASE_CLK+4) {
	io_clk_write(addr, data);
        return;
    }

    if (addr >= IOBASE_SR && addr < IOBASE_SR+2) {
        return;
    }

    if (addr >= IOBASE_PSW && addr < IOBASE_PSW+2) {
        io_psw_write(addr, data);
        return;
    }

//    if (addr >= IOBASE_PCLK && addr < IOBASE_PCLK+4) {
//	io_pclk_write(addr, data);
//        return;
//    }

    if (addr >= IOBASE_RK && addr < IOBASE_RK+32) {
	io_rk_write(addr, data, writeb);
        return;
    }

    support_signals_bus_error();
}

void
reset_support(void)
{
    io_rk_reset();
    tto_csr = CSR_DONE;
    tti_csr = CSR_DONE;
    clk_csr = CSR_DONE;
    pclk_csr = 0;
}



/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
