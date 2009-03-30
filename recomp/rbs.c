#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "xlate.h"

typedef unsigned char u8;
typedef unsigned short u16;

typedef unsigned int u22;

u22 pc;
u16 isn;

u16 *memory;
int memory_size;

int dmode, dreg;
int smode, sreg;
u22 src, dest;
int regdelta;
int reg;
int trap;
int offset;

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
#if 0
	return memory[addr/2];
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

void fetch(void)
{
	isn = read_mem(pc);
	pc += 2;
}

#define MAX_XLATE_FIFO 32
u32 xlate_fifo[MAX_XLATE_FIFO];
int xlate_fifo_head;
int xlate_fifo_tail;

u32 get_xlate_fifo(void)
{
	if (xlate_fifo_tail == xlate_fifo_head)
		return 0;

	v = xlate_fifo[xlate_fifo_tail++];
	if (xlate_fifo_tail == MAX_XLATE_FIFO)
		xlate_fifo_tail = 0;
}

void put_xlate_fifo(u32 v)
{
	xlate_fifo[xlate_fifo_head++] = v;
	if (xlate_fifo_head == MAX_XLATE_FIFO)
		xlate_fifo_head = 0;
}

void put_xlate_ins(int op, int rd, int rs, int immed)
{
	u32 v;

	v = (op << 24) | (rd << 20) | (rs << 16) | (immed & 0xffff);
	put_xlate_fifo(v);
}

unsigned int reg_file[16];
#define R_R0 0
#define R_PC 7
#define R_SP 6
#define R_S0 8
#define R_S1 9
#define R_S2 10
#define R_PS 15

void decode(void)
{
	if (maskmatch(isn, 0000000, 0177770)) {
		switch (isn & 7) {
		/* HALT	MS	-	000000 */
		case 0:
			put_xlate_fifo(X_HALT);
			break;
		/* WAIT	MS	-	000001 */
		case 1:
			put_xlate_fifo(X_WAIT);
			break;
		/* RTI	MS	-	000002 */
		case 2:
			put_xlate_ins(X_LI, R_PC, R_SP, 0);
			put_xlate_ins(X_LI, R_PS, R_SP, 2);
			put_xlate_ins(X_ADDIM, R_SP, 0, 4);
			break;
		/* BPT	PC	-	000003 */
		case 3:
			break;
		/* IOT	PC	-	000004 */
		case 4:
			break;
		/* RESET MS	-	000005 */
		case 5:
			put_xlate_fifo(X_RESET);
			break;
		/* RTT	MS	-	000006 */
		case 6:
			break;
		/* MFPT	MS	-	000007 */
		case 7:
			break;
		}
	}

	if (maskmatch(isn, 0000000, 0177000)) {

		switch (isn & 0777) {
		/* JMP	PC	DD	000100 */
		case 0100:
			put_xlate_ins(X_ADDA, R_PC, R_PC, isn & 077);
			break;
		/* RTS	PC	R	000200 */
		case 0200:
			reg = (isn >> 0) & 7;
			put_xlate_ins(X_MOVR, R_PC, reg, 0);
			put_xlate_ins(X_LI, reg, R_PC, 0);
			put_xlate_ins(X_ADDIM, R_PC, 0, 2);
			break;
		/* SPL	PC	N	000230 */
		case 0230:
			break;
		/* SCC	CC	-	000277 */
		/* SEN	CC	-	000270 */
		/* SEZ	CC	-	000264 */
		/* SEV	CC	-	000262 */
		/* SEC	CC	-	000261 */
		/* S	CC	-	000260 */
		/* CCC	CC	-	000257 */
		/* CLN	CC	-	000250 */
		/* CLZ	CC	-	000244 */
		/* CLV	CC	-	000242 */
		/* CLC	CC	-	000241 */
		/* C	CC	-	000240 */
			break;
		}
	}

/* ASLB	SOB	DD	1063MD */
/* ASRB	SOB	DD	1062MD */
/* ROLB	DOB	DD	1061MD */
/* RORB	DOB	DD	1060MD */
/* TSTB	SOB	DD	1057MD */
/* SBCB	SOB	DD	1056MD */
/* ADCB	SOB	DD	1055MD */
/* NEGB	SOB	DD	1054MD */
/* DECB	SOB	DD	1053MD */
/* INCB	SOB	DD	1052MD */
/* COMB	SOB	DD	1051MD */
/* CLRB	SOB	DD	1050MD */
/* CSM	PC	DD	0070MD */
/* SXT	SO	DD	0067MD */
/* MTPI	MS	DD	0066MD */
/* ASR	SO	DD	0062MD */
/* ROL	DO	DD	0061MD */
/* ROR	DO	DD	0060MD */
/* TST	SO	DD	0057MD */
/* SBC	SO	DD	0056MD */
/* ADC	SO	DD	0055MD */
/* NEG	SO	DD	0054MD */
/* DEC	SO	DD	0053MD */
/* INC	SO	DD	0052MD */
/* COM	SO	DD	0051MD */
/* CLR	SO	DD	0050MD */
/* SWAB	SO	DD	0003MD */

/* MFPS	MS	DD	1067MD */
	if (maskmatch(isn, 0106700, 0177700)) {
	}

/* MTPD	MS	DD	1066MD */
	if (maskmatch(isn, 0106600, 0177700)) {
	}

	    maskmatch(isn, 0106300, 0177700) ||
	    maskmatch(isn, 0106200, 0177700) ||
	    maskmatch(isn, 0106100, 0177700) ||
	    maskmatch(isn, 0106000, 0177700) ||
	    maskmatch(isn, 0105700, 0177700) ||
	    maskmatch(isn, 0105600, 0177700) ||
	    maskmatch(isn, 0105500, 0177700) ||
	    maskmatch(isn, 0105400, 0177700) ||
	    maskmatch(isn, 0105300, 0177700) ||
	    maskmatch(isn, 0105200, 0177700) ||
	    maskmatch(isn, 0105100, 0177700) ||
	    maskmatch(isn, 0105000, 0177700) ||
	    maskmatch(isn, 0007000, 0177700) ||
	    maskmatch(isn, 0006700, 0177700) ||
	    maskmatch(isn, 0006600, 0177700) ||

/* ASL	SO	DD	0063MD */
	if (maskmatch(isn, 0006300, 0177700)) {
		put_xlate_dd(X_ASL, isn);
	}

	    maskmatch(isn, 0006200, 0177700) ||
	    maskmatch(isn, 0006100, 0177700) ||
	    maskmatch(isn, 0006000, 0177700) ||
	    maskmatch(isn, 0005700, 0177700) ||
	    maskmatch(isn, 0005600, 0177700) ||
	    maskmatch(isn, 0005500, 0177700) ||
	    maskmatch(isn, 0005400, 0177700) ||
	    maskmatch(isn, 0005300, 0177700) ||
	    maskmatch(isn, 0005200, 0177700) ||
	    maskmatch(isn, 0005100, 0177700) ||
	    maskmatch(isn, 0005000, 0177700) ||
	    maskmatch(isn, 0000300, 0177700) ||
	    0)
	{
            dmode = (isn >> 3) & 7;
            dreg = (isn >> 0) & 7;
            if (dreg > 5) regdelta = 2; else regdelta = 0;
            switch (dmode) {
            case 0:
                dest = &regs[dreg]; break;
            case 1:
                dest = memory + regs[dreg] / 2; break;
            case 2:
                dest = memory + regs[dreg] / 2;
                regs[dreg] += regdelta;
                break;
            case 3:
                dest = memory + memory[ regs[dreg] / 2 ] / 2;
                regs[dreg] += regdelta;
                break;
            case 4:
                regs[dreg] -= regdelta;
                dest = memory + regs[dreg];
                break;
            case 5: 
                regs[dreg] -= regdelta;
                dest = memory + memory[ regs[dreg] / 2 ];
                break;
            case 6:
                dest = memory + regs[dreg] + memory[ pc / 2 ];
                pc += 2;
                break;
            case 7:
                dest = memory + memory[ regs[dreg] + memory[ pc / 2 ] ];
                pc += 2;
                break;
            }
	}

/* XOR	DO	RDD	074RMD */
/* JSR	PC	RDD	004RMD */
	if (maskmatch(isn, 0074000, 0177000) ||
	    maskmatch(isn, 0004000, 0177000))
	{
            reg = (isn >> 6) & 7;
            dmode = (isn >> 3) & 7;
            dreg = (isn >> 0) & 7;
	}

/* ASH	DO	RSS	072RMS */
/* ASHC	DO	RSS	073RMS */
/* DIV	DO	RSS	071RMS */
/* MUL	DO	RSS	070RMS */
	if (maskmatch(isn, 0073000, 0177000) ||
	    maskmatch(isn, 0072000, 0177000) ||
	    maskmatch(isn, 0071000, 0177000) ||
	    maskmatch(isn, 0070000, 0177000))
	{
            reg = (isn >> 6) & 7;
            smode = (isn >> 3) & 7;
            sreg = (isn >> 0) & 7;
	}

/* MFPD	MS	SS	1065SS */
/* MTPS	MS	SS	1064SS */
/* MFPI	MS	SS	0065SS */
	if (maskmatch(isn, 0106500, 0177700) ||
	    maskmatch(isn, 0106400, 0177700) ||
	    maskmatch(isn, 0006500, 0177700))
	{
            smode = (isn >> 3) & 7;
            sreg = (isn >> 0) & 7;
	}

/* SUB	DO	SSDD	16MSMD */
/* BISB	DOB	SSDD	15MSMD */
/* BICB	DOB	SSDD	14MSMD */
/* BITB	DOB	SSDD	13MSMD */
/* CPMB	DOB	SSDD	12MSMD */
/* MOVB	DOB	SSDD	11MSMD */
/* ADD	DO	SSDD	06MSMD */
/* BIS	DO	SSDD	05MSMD */
/* BIC	DO	SSDD	04MSMD */
/* BIT	DO	SSDD	03MSMD */
/* CMP	DO	SSDD	02MSMD */
/* MOV	DO	SSDD	01MSMD */
	if (maskmatch(isn, 0160000, 0170000) ||
	    maskmatch(isn, 0150000, 0170000) ||
	    maskmatch(isn, 0140000, 0170000) ||
	    maskmatch(isn, 0130000, 0170000) ||
	    maskmatch(isn, 0120000, 0170000) ||
	    maskmatch(isn, 0110000, 0170000) ||
	    maskmatch(isn, 0060000, 0170000) ||
	    maskmatch(isn, 0050000, 0170000) ||
	    maskmatch(isn, 0040000, 0170000) ||
	    maskmatch(isn, 0030000, 0170000) ||
	    maskmatch(isn, 0020000, 0170000) ||
	    maskmatch(isn, 0010000, 0170000))
	{
            smode = (isn >> 9) & 7;
            sreg = (isn >> 6) & 7;
            dmode = (isn >> 3) & 7;
            dreg = (isn >> 0) & 7;

            if (sreg > 5) regdelta = 2;
            switch (smode) {
            case 0:
                src = &regs[sreg]; break;
            case 1:
                src = memory + regs[sreg] / 2; break;
            case 2:
                src = memory + regs[sreg] / 2;
                regs[sreg] += regdelta;
                break;
            case 3:
                src = memory + memory[ regs[sreg] / 2 ] / 2;
                regs[sreg] += regdelta;
                break;
            case 4:
                regs[sreg] -= regdelta;
                src = memory + regs[sreg];
                break;
            case 5: 
                regs[sreg] -= regdelta;
                src = memory + memory[ regs[sreg] / 2 ];
                break;
            case 6:
                src = memory + regs[sreg] + memory[ pc / 2 ];
                pc += 2;
                break;
            case 7:
                src = memory + memory[ regs[sreg] + memory[ pc / 2 ] ];
                pc += 2;
                break;
            }

            if (dreg > 5) regdelta = 2;
            switch (dmode) {
            case 0:
                dest = &regs[dreg]; break;
            case 1:
                dest = memory + regs[dreg] / 2; break;
            case 2:
                dest = memory + regs[dreg] / 2;
                regs[dreg] += regdelta;
                break;
            case 3:
                dest = memory + memory[ regs[dreg] / 2 ] / 2;
                regs[dreg] += regdelta;
                break;
            case 4:
                regs[dreg] -= regdelta;
                dest = memory + regs[dreg];
                break;
            case 5: 
                regs[dreg] -= regdelta;
                dest = memory + memory[ regs[dreg] / 2 ];
                break;
            case 6:
                dest = memory + regs[dreg] + memory[ pc / 2 ];
                pc += 2;
                break;
            case 7:
                dest = memory + memory[ regs[dreg] + memory[ pc / 2 ] ];
                pc += 2;
                break;
            }
	}

/* TRAP	PC	-	104400	104777 */
/* EMT	PC	-	104000	104377 */
	if (maskmatch(isn, 0104400, 0177400) ||
	    maskmatch(isn, 0104000, 0177400))
	{
		trap = isn & 0377;
	}

/* BCS	PC	8OFF	1034OO */
/* BCC	PC	8OFF	1030OO */
/* BLO	PC	8OFF	1034OO */
/* BHIS	PC	8OFF	1030OO */
/* BVS	PC	8OFF	1024OO */
/* BVC	PC	8OFF	1020OO */
/* BLOS	PC	8OFF	1014OO */
/* BHI	PC	8OFF	1010OO */
/* BMI	PC	8OFF	1004OO */
/* BPL	PC	8OFF	1000OO */
/* SOB	PC	R8OFF	0770OO */
	if (maskmatch(isn, 0103400, 0177700) ||
	    maskmatch(isn, 0103000, 0177700) ||
	    maskmatch(isn, 0102400, 0177700) ||
	    maskmatch(isn, 0102000, 0177700) ||
	    maskmatch(isn, 0101400, 0177700) ||
	    maskmatch(isn, 0101000, 0177700) ||
	    maskmatch(isn, 0100000, 0177700) ||
	    maskmatch(isn, 0077000, 0177700))
	{
		offset = isn & 077;
	}

/* MARK	PC	NN	0064NN */
/* BLE	PC	8OFF	0034OO */
/* BGT	PC	8OFF	0030OO */
/* BLT	PC	8OFF	0024OO */
/* BGE	PC	8OFF	0020OO */
/* BEQ	PC	8OFF	0014OO */
/* BNE	PC	8OFF	0010OO */
/* BR	PC	8OFF	0004OO */
	if (maskmatch(isn, 0006400, 0177700) ||
	    maskmatch(isn, 0003400, 0177700) ||
	    maskmatch(isn, 0002400, 0177700) ||
	    maskmatch(isn, 0002000, 0177700) ||
	    maskmatch(isn, 0001400, 0177700) ||
	    maskmatch(isn, 0001000, 0177700) ||
	    maskmatch(isn, 0000400, 0177700))
	{
		offset = isn & 077;
	}

}

void execute(void)
{
	u32 si;

	while (si = get_xlate_fifo()) {
		u8 op, dreg, sreg;
		int immed;

		op = si >> 24;
		dreg = (si >> 20) & 0xf;
		sreg = (si >> 16) & 0xf;
		immed = (int)(sh & 0xffff);

		switch (op) {
		case X_LI:
			reg_file[dreg] = memory[ reg_file[sreg] ];
			break;
		case X_SI:
			memory[ reg_file[sreg] ] = reg_file[dreg];
			break;
		case X_ADDA:
			reg_file[dreg] += reg_file[sreg];
			break;
		case X_ADDIM:
			reg_file[dreg] = ((int)reg_file[dreg] + immed);
		case X_SWAB:
			reg_file[dreg] = swab(reg_file[dreg]);
			break;
		}
	}
}

void step(void)
{
	fetch();
	translate();
	execute();
}

void run(void)
{
	step();
}

main()
{
	int c;

	mem_init();
	for (c = 0; c < 10; c++) {
		run();
	}
}
