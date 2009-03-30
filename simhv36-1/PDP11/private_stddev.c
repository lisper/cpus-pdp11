
#include <stdio.h>
#include <unistd.h>

#include "pdp11_defs.h"
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u22;

struct stddev_context_s {
    char *name;
    u16 clk_csr;
    u16 pclk_csr;
    u16 pclk_ctr;
    u16 pclk_csb;

    u16 tti_csr;
    u16 tto_csr;
    u8 tti_data;
    u8 tto_data;
} stddev_context[2];

static int _io_has_reset;

void
_io_reset(void)
{
    printf("_io_reset()\n");
    stddev_context[0].tto_csr = CSR_DONE;
    stddev_context[1].tto_csr = CSR_DONE;

    stddev_context[0].tti_csr = CSR_DONE;
    stddev_context[1].tti_csr = CSR_DONE;

    stddev_context[0].clk_csr = CSR_DONE;
    stddev_context[1].clk_csr = CSR_DONE;

    stddev_context[0].pclk_csr = 0;
    stddev_context[1].pclk_csr = 0;

    stddev_context[0].name = "simh";
    stddev_context[1].name = "rtl";
}

void _io_check_reset(void)
{
    if (_io_has_reset == 0) {
        _io_has_reset = 1;
        _io_reset();
    }
}

extern int int_req[];

extern void cpu_int_clear(int);
extern void cpu_int_set(int);

void _io_cpu_int_clear(struct stddev_context_s *s, int bit)
{
    if (0) printf("_io_cpu_int_clear(%s %d)\n", s->name, bit);

    switch (bit) {
    case 0:
        if (s == &stddev_context[1])
            cpu_int_clear(0);
        if (s == &stddev_context[0])
            CLR_INT (CLK);
        break;
    case 1:
        if (s == &stddev_context[1])
            cpu_int_clear(1);
        if (s == &stddev_context[0])
            CLR_INT (TTI);
        break;
    case 2:
        if (s == &stddev_context[1])
            cpu_int_clear(2);
        if (s == &stddev_context[0])
            CLR_INT (TTO);
        break;
    }
}

void _io_cpu_int_set(struct stddev_context_s *s, int bit)
{
    if (0) printf("_io_cpu_int_set(%s %d)\n", s->name, bit);

    switch (bit) {
    case 0:
        if (s == &stddev_context[1])
            cpu_int_set(0);
        if (s == &stddev_context[0])
            SET_INT (CLK);
        break;
    case 1:
        if (s == &stddev_context[1])
            cpu_int_set(1);
        if (s == &stddev_context[0])
            SET_INT (TTI);
        break;
    case 2:
        if (s == &stddev_context[1])
            cpu_int_set(2);
        if (s == &stddev_context[0])
            SET_INT (TTO);
        break;
    }
}

u16 _io_tti_read(struct stddev_context_s *s, u22 addr)
{
    _io_check_reset();
    printf("_io_tti_read(%o) %s\n", addr, s->name);

    if (addr & 2) {
        s->tti_csr &= ~CSR_DONE;
        _io_cpu_int_clear(s, 1);
        return s->tti_data;
    } else {
        return s->tti_csr & (CSR_DONE | CSR_IE);
    }
}

void _io_tti_write(struct stddev_context_s *s, u22 addr, u16 data)
{
    _io_check_reset();
    printf("_io_tti_write() addr=%o, data=%o; %s\n", addr, data, s->name);

    if ((addr & 2) == 0) {
        if (addr & 1)
            return;

        if ((data & CSR_IE) == 0)
            _io_cpu_int_clear(s, 1);
        else
            if ((s->tti_csr & (CSR_DONE | CSR_IE)) == CSR_DONE)
                _io_cpu_int_set(s, 1);

        s->tti_csr = (s->tti_csr & ~CSR_IE) | (data & CSR_IE);
    }
}

u16 _io_tto_read(struct stddev_context_s *s, u22 addr)
{
    _io_check_reset();
    printf("_io_tto_read(%o) %s\n", addr, s->name);

    if (addr & 2) {
        return s->tto_data;
    } else {
        return s->tto_csr & (CSR_DONE | CSR_IE);
    }
}

void _io_tto_write(struct stddev_context_s *s, u22 addr, u16 data)
{
    _io_check_reset();
    if (1) printf("_io_tto_write() addr=%o data=%o; %s\n",
                  addr, data, s->name);

    if (addr & 2) {
        if ((addr & 1) == 0) {
            //printf("TTO %o %c\n", data, data);
            printf("%c", data); fflush(stdout);
            s->tto_data = data;
        }
        s->tto_csr &= ~CSR_DONE;
        _io_cpu_int_clear(s, 2);

s->tto_csr |= CSR_DONE;
if (s->tto_csr & CSR_IE) _io_cpu_int_set(s, 2);

    } else {
        if (addr & 1)
            return;

        if (0) printf("tto_csr %o\n", s->tto_csr);
        if ((data & CSR_IE) == 0)
            _io_cpu_int_clear(s, 2);
        else
            if ((s->tto_csr & (CSR_DONE | CSR_IE)) == CSR_DONE)
                _io_cpu_int_set(s, 2);

        s->tto_csr = (s->tto_csr & ~CSR_IE) | (data & CSR_IE);
    }
}

u16 _io_clk_read(struct stddev_context_s *s, u22 addr)
{
    _io_check_reset();
    return s->clk_csr;
}

void _io_clk_write(struct stddev_context_s *s, u22 addr, u16 data)
{
    _io_check_reset();
    if (addr & 1)
        return;

    s->clk_csr = (s->clk_csr & ~CSR_IE) | (data & CSR_IE);

    if ((data & CSR_DONE) == 0)
        s->clk_csr &= ~CSR_DONE;

    if ((s->clk_csr & CSR_IE) == 0 ||
        (s->clk_csr & CSR_DONE) == 0)
    {
        _io_cpu_int_clear(s, 0);
    }
}

/* ---------------------------------------- */

void io_clk_write(u22 addr, u16 data)
{
	_io_clk_write(&stddev_context[1], addr, data);
}

u16 io_clk_read(u22 addr)
{
	return _io_clk_read(&stddev_context[1], addr);
}

u16 io_tti_read(u22 addr)
{
	return _io_tti_read(&stddev_context[1], addr);
}

void io_tti_write(u22 addr, u16 data)
{
	_io_tti_write(&stddev_context[1], addr, data);
	return;
}

u16 io_tto_read(u22 addr)
{
	return _io_tto_read(&stddev_context[1], addr);
}

void io_tto_write(u22 addr, u16 data)
{
	_io_tto_write(&stddev_context[1], addr, data);
	return;
}

/* ---------------------------------------- */

int private_clk_rd (int32 *data, int32 PA, int32 access)
{
	int byte;
	byte = access == 4 ? 1 : 0;
        *data = _io_clk_read(&stddev_context[0], PA);
	return 0;
}

int private_clk_wr (int32 data, int32 PA, int32 access)
{
	int byte;
	byte = access == 4 ? 1 : 0;
	_io_clk_write(&stddev_context[0], PA, data);
	return 0;
}

int private_tti_rd (int32 *data, int32 PA, int32 access)
{
	int byte;
	byte = access == 4 ? 1 : 0;
        *data = _io_tti_read(&stddev_context[0], PA);
	return 0;
}

int private_tti_wr (int32 data, int32 PA, int32 access)
{
	int byte;
	byte = access == 4 ? 1 : 0;
	_io_tti_write(&stddev_context[0], PA, data);
	return 0;
}


int private_tto_rd (int32 *data, int32 PA, int32 access)
{
	int byte;
	byte = access == 4 ? 1 : 0;
        *data = _io_tto_read(&stddev_context[0], PA);
	return 0;
}

int private_tto_wr (int32 data, int32 PA, int32 access)
{
	int byte;
	byte = access == 4 ? 1 : 0;
	_io_tto_write(&stddev_context[0], PA, data);
	return 0;
}



/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
