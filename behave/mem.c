/* mem.c */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "cpu.h"
#include "mem.h"
#include "support.h"

static u16 *memory;
static int memory_size;

void show(int ma)
{
    printf("memory %o: %06o %06o %06o %06o\n",
           ma, memory[ma/2], memory[ma/2+1], memory[ma/2+2], memory[ma/2+3]);
}

/* use for debug only */
u16
raw_read_memory(u22 addr)
{
    return memory[addr/2];
}

void
raw_write_memory(u22 addr, u16 data)
{
    if (addr/2 > memory_size)
        return;

    memory[addr/2] = data;
}

int mem_init(void)
{
    memory_size = 1 * 1024 * 1024;
    memory = (u16 *)malloc(memory_size);

    return 0;
}

#define	CACHE_LINES	32
#define LINESIZE	8
#define LINESIZE_LOG2	3
#define LINESIZE_MASK	0x0000f8
#define ADDR_MASK	0x3ffff8

struct cache_s {
    int flags;
#define C_FILLED 0x1
#define C_DIRTY	 0x2
    u22 tag;
    u16 data[LINESIZE/2];
} cache[CACHE_LINES];

void flush_line(int line)
{
    if (cache[line].flags & C_DIRTY) {
        int i;
        for (i = 0; i < LINESIZE/2; i++) {
            memory[ cache[line].tag + i ] = cache[line].data[i];
        }
    }
    cache[line].tag = 0;
    cache[line].flags = 0;
}

void fill_line(int line, u22 addr)
{
    int i;
    printf("cache: fill line %d @ %08x\n", line, cache[line].tag);
    for (i = 0; i < LINESIZE/2; i++) {
        cache[line].data[i] = memory[ cache[line].tag + i ];
    }
    cache[line].flags = C_FILLED;
    cache[line].tag = addr & ADDR_MASK;
}

u16 read_mem(u22 addr)
{
    u16 data;

    if (addr >= IOPAGEBASE) {
        data = io_read(addr);
        rtl_record_io_read_word(addr, data);
        return data;
    }

#if 1
    if (addr/2 > memory_size) {
        rtl_record_mem_read_word(addr, 0);
        return 0/*0xffff*/;
    }

    data = memory[addr/2];
    rtl_record_mem_read_word(addr, data);
    return data;
#else
    unsigned int line, off;

    line = (addr & LINESIZE_MASK) >> LINESIZE_LOG2;
    off = addr & ~ADDR_MASK;
    if (cache[line].tag != (addr & ADDR_MASK) ||
        !(cache[line].flags & C_FILLED))
    {
        flush_line(line);
        fill_line(line, addr);
    }

    printf("cache: hit %08x, line %d\n", addr, line);
    return cache[line].data[off];
#endif
}

void write_mem(u22 addr, u16 data)
{
    if (addr >= IOPAGEBASE) {
        rtl_record_io_write_word(addr, data);
        return io_write(addr, data, 0);
    }

#if 1
    rtl_record_mem_write_word(addr, data);

    if (addr/2 > memory_size)
        return;

    memory[addr/2] = data;
#else
    unsigned int line, off;

    line = (addr & LINESIZE_MASK) >> LINESIZE_LOG2;
    off = addr & ~ADDR_MASK;
    if (cache[line].tag != (addr & ADDR_MASK) ||
        !(cache[line].flags & C_FILLED))
    {
        flush_line(line);
        fill_line(line, addr);
    }

    cache[line].data[off] = data;
    cache[line].flags |= C_DIRTY;
#endif    
}

u16 read_mem_byte(u22 addr)
{
    u16 data;

    if (addr >= IOPAGEBASE) {
        data = io_read(addr);
        if (addr & 1)
            data >>= 8;
        else
            data &= 0xff;
        rtl_record_io_read_byte(addr, data);
        return data;
    }

    if (addr & 1) {
        data = raw_read_memory(addr) >> 8;
        rtl_record_mem_read_byte(addr, data);
        return data;
    } else {
        data = raw_read_memory(addr) & 0xff;
        rtl_record_mem_read_byte(addr, data);
        return data;
    }
}

void write_mem_byte(u22 addr, u16 data)
{
    if (addr >= IOPAGEBASE) {
        rtl_record_io_write_byte(addr, data & 0xff);
        return io_write(addr, data, 1);
    }

    if (addr & 1) {
        rtl_record_mem_write_byte(addr, data & 0xff);
        raw_write_memory(addr, (raw_read_memory(addr) & 0xff) | (data << 8));
    } else {
        rtl_record_mem_write_byte(addr, data & 0xff);
        raw_write_memory(addr, (raw_read_memory(addr) & 0xff00) | (data & 0xff));
    }
}




/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
