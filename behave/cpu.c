/*
 * pdp-11 hardware simulation in c
 * brad@heeltoe.com
 */

#include <stdio.h>

#include "cpu.h"
#include "mem.h"

/* state */
int halted;
int waited;

int trap;
int trap_bpt;
int trap_iot;
int trap_emt;
int trap_trap;
int trap_odd;
int trap_bus;
int trap_ill;
int trap_priv;

int interrupt;
u16 interrupt_vector;

u16 psw;
#define cc_n  ((psw >> 3) & 1)
#define cc_z  ((psw >> 2) & 1)
#define cc_v  ((psw >> 1) & 1)
#define cc_c  ((psw >> 0) & 1)
#define ipl   ((psw >> 5) & 7)

u16 regs[8];
#define sp regs[6]
#define pc regs[7]

/* wires */
wire assert_wait;
wire assert_halt;
wire assert_reset;
wire assert_bpt;
wire assert_iot;
wire assert_trap_odd;
wire assert_trap_ill;
wire assert_trap_priv;
wire assert_trap_emt;
wire assert_trap_trap;
wire assert_trap_bus;

wire assert_int;
wire16 assert_int_vec;
wire16 assert_int_ipl;

wire16 isn_15_12;
wire16 isn_15_9;
wire16 isn_15_6;
wire16 isn_11_6;
wire16 isn_11_9;
wire16 isn_5_0;
wire16 isn_3_0;

wire is_isn_rss;
wire is_isn_rdd;
wire is_isn_rxx;
wire is_isn_r32;
wire is_isn_lowbyte;
wire is_isn_byte;
wire is_illegal;
wire no_operand;
wire store_result;
wire store_result32;
wire store_ss_reg;

wire need_srcspec_dd_word,
    need_srcspec_dd_byte,
    need_srcspec_dd;

wire need_destspec_dd_word,
    need_destspec_dd_byte,
    need_destspec_dd,
    need_pop_reg,
    need_pop_pc_psw,
    need_push_state,
    need_dest_data,
    need_src_data;

wire need_s1,
    need_s2,
    need_s4,
    need_d1,
    need_d2,
    need_d4;

wire22 dd_ea_mux;
wire22 ss_ea_mux;

wire assert_int;
wire16 assert_int_vec;

u22 new_pc;
wire latch_pc;

wire new_cc_n, new_cc_z, new_cc_v, new_cc_c;
wire latch_cc;

/* regs */
u16 isn;
u22 dd_ea;
u22 ss_ea;

int dd_mode,
    dd_reg;

wire dd_dest_mem,
    dd_dest_reg,
    dd_ea_ind,
    dd_post_incr,
    dd_pre_dec;

int ss_mode, ss_reg;

/* regs */
u16 ss_data;	/* result of s states */
u16 dd_data;	/* result of d states */
u16 e1_data;	/* result of e states */

wire ss_dest_mem,
    ss_dest_reg,
    ss_ea_ind,
    ss_post_incr,
    ss_pre_dec;

wire16 pc_mux;
wire16 sp_mux;
wire16 psw_mux;

wire16 ss_data_mux;
wire16 dd_data_mux;
wire16 e1_data_mux;
wire16 ss_mem_data;
wire16 dd_mem_data;

wire16 pop_mem_data;
wire16 pc_mem_data;
wire16 psw_mem_data;

wire16 e1_result;
wire32 e32_result;

/* cpudmodes */
enum {
    mode_kernel = 0,
    mode_super = 1,
    mode_undef = 2,
    mode_user = 3
};
int current_mode;

/*
 * main cpu states
 *
 * f1 fetch;	   clock isn
 * c1 decode;
 *
 * s1 source1;	   clock ss_data
 * s2 source2;	   clock ss_data
 * s3 source3;	   clock ss_data
 * s4 source4;	   clock ss_data
 *
 * d1 dest1;	   clock dd_data
 * d2 dest2;	   clock dd_data
 * d3 dest3;	   clock dd_data
 * d4 dest4;	   clock dd_data
 *
 * e1 execute;	   clock pc, sp & reg+-
 * w1 writeback1;  clock e1_result
 *
 * o1 pop pc	   mem read
 * o2 pop psw	   mem read
 * o3 pop reg	   mem read
 *
 * p1 push sp	   mem write
 *
 * t1 push psw	   mem write
 * t2 push pc	   mem write
 * t3 read pc	   mem read
 * t4 read psw	   mem read
 *
 * i1 interrupt wait
 *
 * minimum states/instructions = 3
 * maximum states/instructions = 10
 *
 * mode	symbol	ea1	ea2		ea3		data		side-eff
 * 
 * 0	R	x	x		x		R		x
 * 1	(R)	R	x		x		M[R]		x
 * 2	(R)+	R	X		x		M[R]		R<-R+n
 * 3	@(R)+	R	M[R]		x		M[M[R]]		R<-R+2
 * 4	-(R)	R-2	x		x		M[R-2]		R<-R-n
 * 5	@-(R)	R-2	M[R-2]		x		M[M[R-2]]	R<-R-2
 * 6	X(R)	PC	M[PC]+R		x		M[M[PC]+R]	x
 * 7	@X(R)	PC	M[PC]+R		M[M[PC]+R]	M[M[M[PC]+R]]	x
 *
 * mode 0 -
 *  f1  pc++
 *  c1
 *  d1  dd_data = r
 *  e1  e1_result
 *             
 * mode 1 -
 *  f1  pc++
 *  c1
 *  d1  dd_ea = r
 *  d4  dd_data = mem_data	optional
 *  e1  e1_result
 *
 * mode 2 -
 *  f1  pc++
 *  c1
 *  d1  dd_ea = r, r++
 *  d4  dd_data = mem_data	optional
 *  e1  e1_result
 *
 * mode 3 -
 *  f1  pc++
 *  c1  dd_ea = r
 *  d1  dd_ea = mem_data, r++
 *  d4  dd_data = mem_data	optional
 *  e1  e1_result
 *
 * mode 4 -
 *  f1  pc++
 *  c1
 *  d1  dd_ea = r-x, r--
 *  d4  dd_data = mem_data	optional
 *  e1  e1_result
 *
 * mode 5 -
 *  f1  pc++
 *  c1
 *  d1  dd_ea = r-x, r--
 *  d2  dd_ea = mem_data
 *  d4  dd_data = mem_data	optional
 *  e1  e1_result
 *
 * mode 6 -
 *  f1  pc++
 *  c1
 *  d1  dd_ea = pc, pc++
 *  d2  dd_ea = mem_data + regs[dd_reg]
 *  d4  dd_data = mem_data	optional
 *  e1  e1_result
 *
 * mode 7 -
 *  f1  pc++
 *  c1
 *  d1  dd_ea = pc, pc++
 *  d2  dd_ea = mem_data + regs[dd_reg]
 *  d3  dd_ea = mem_data
 *  d4  dd_data = mem_data	optional
 *  e1  e1_result
 *
 */
enum {
    h1,
    f1, c1,
    s1, s2, s3, s4, d1, d2, d3, d4,
    e1, w1, o1, o2, o3, p1, t1, t2, t3, t4, i1
};

wire new_istate;
int istate;

int verbose_mux;
int verbose_data;
int verbose_psw;
int verbose_cc;

void fetch(void)
{
    assert_trap_odd = 0;

    if (pc & 1) {
        printf("fetch: odd pc %o\n", pc);
	assert_trap_odd = 1;
	return;
    }

    printf("------\n");
    printf("f1: pc=%o, sp=%o, psw=%o ipl%d n%d z%d v%d c%d\n",
	   pc, sp, psw, ipl, cc_n, cc_z, cc_v, cc_c);
    printf("    trap=%d, interrupt=%d\n", trap, interrupt);
    printf("    regs %6o %6o %6o %6o \n", regs[0], regs[1], regs[2], regs[3]);
    printf("         %6o %6o %6o %6o \n", regs[4], regs[5], regs[6], regs[7]);

    isn = read_mem(pc);
    dis(isn, raw_read_memory(pc+2), raw_read_memory(pc+4));
}

/*
 * effective address mux;
 * set address of next memory operation in various states
 */
void
do_ea_mux(void)
{
    /*
     * source ea calculation:
     * decode - load from register or pc
     * s1 - if mode 6,7 add result of pc+2 fetch to reg
     * s2 - result of fetch
     * t1 - trap vector
     * t4 - incr ea
     */
    ss_ea_mux =
	(istate == c1 || istate == s1) ?
	(ss_mode == 0 ? regs[ss_reg] :
         ss_mode == 1 ? regs[ss_reg] :
	 ss_mode == 2 ? regs[ss_reg] :
	 ss_mode == 3 ? regs[ss_reg] :
	 ss_mode == 4 ? (is_isn_byte ? (regs[ss_reg]-1) : (regs[ss_reg]-2)) :
	 ss_mode == 5 ? (is_isn_byte ? (regs[ss_reg]-1) : (regs[ss_reg]-2)) :
	 ss_mode == 6 ? pc :
	 ss_mode == 7 ? pc :
	 0) :

	istate == s2 ?
	(ss_mode == 3 ? ss_mem_data :
         ss_mode == 5 ? ss_mem_data :
         ss_mode == 6 ? (ss_mem_data + regs[ss_reg]) & 0xffff :
	 ss_mode == 7 ? (ss_mem_data + regs[ss_reg]) & 0xffff :
	 ss_ea) :

        istate == s3 ? ss_mem_data :

	istate == t1 ? ((trap_odd | trap_bus) ? 004 :
                        trap_ill ? 010 :
                        trap_priv ? 010 :
                        trap_bpt ? 014 :
                        trap_iot ? 020 :
                        trap_emt ? 030 :
                        trap_trap ? 034 :
                        interrupt ? interrupt_vector :
                        0) :

        istate == t2 ? ss_ea :
        istate == t3 ? ss_ea + 2 :
        istate == t4 ? ss_ea + 2 :
	0;

    if (verbose_mux &&
        ss_ea != ss_ea_mux && (istate < e1 || istate >= t1))
    {
        printf("  ea_mux: ss_ea_mux %6o\n", ss_ea_mux);

        if (istate == s1) {
            printf("          ss_mem_data %6o, R%d %6o\n",
                   ss_mem_data, ss_reg, regs[ss_reg]);
        }

        if (istate >= t1) {
            printf("          %d%d%d%d %d%d%d%d %d\n",
                   trap_odd, trap_bus, trap_ill, trap_priv,
                   trap_bpt, trap_iot, trap_emt, trap_trap,
                   interrupt);
        }
    }

    /*
     * dest ea calculation:
     * decode - load from register or pc
     * d1 - if mode 6,7 add result of pc+2 fetch to reg
     * d2 - result of fetch
     */
    dd_ea_mux =
	(istate == c1 || istate == d1) ?
	(dd_mode == 0 ? regs[dd_reg] :
         dd_mode == 1 ? regs[dd_reg] :
	 dd_mode == 2 ? regs[dd_reg] :
	 dd_mode == 3 ? regs[dd_reg] :
	 dd_mode == 4 ? (is_isn_byte ? (regs[dd_reg]-1) : (regs[dd_reg]-2)) :
	 dd_mode == 5 ? (is_isn_byte ? (regs[dd_reg]-1) : (regs[dd_reg]-2)) :
	 dd_mode == 6 ? pc :
	 dd_mode == 7 ? pc :
	 0) :

        istate == d2 ?
	(dd_mode == 3 ? dd_mem_data :
         dd_mode == 5 ? dd_mem_data :
         dd_mode == 6 ? (dd_mem_data + regs[dd_reg]) & 0xffff :
	 dd_mode == 7 ? (dd_mem_data + regs[dd_reg]) & 0xffff :
	 dd_ea) :

        istate == d3 ? dd_mem_data :
	dd_ea;

    if (dd_ea != dd_ea_mux && istate < e1 && verbose_mux) {
	printf("  ea_mux: dd_ea_mux %6o\n", dd_ea_mux);
        if (istate == d1) {
            printf("          dd_mem_data %6o, R%d %6o\n",
                   dd_mem_data, dd_reg, regs[dd_reg]);
        }
    }
}

/*
 * mux various data sources into ss, dd & e1 data registers
 */
void
do_data_mux(void)
{
    u16 old_ss_data = ss_data;
    u16 old_dd_data = dd_data;

    ss_data_mux =
	(istate == c1) && (ss_mode == 0 || is_isn_rxx) ? regs[ss_reg] :
	(istate == s4) ? ss_mem_data :
	ss_data_mux;

    dd_data_mux =
	(istate == c1) && dd_mode == 0 ? regs[dd_reg] :
	(istate == c1) && (isn_15_6 == 01067) ? psw & 0377 : /* mfps */
	(istate == d4) ? dd_mem_data :
	dd_data_mux;

    e1_data_mux =
	(istate == e1) ? e1_result :
	e1_data_mux;

    if (ss_data != ss_data_mux && verbose_mux) {
	printf("  data_mux: ss%d reg%d mem %o mux %o\n",
	       ss_mode, ss_reg, ss_mem_data, ss_data_mux);
    }

    if (dd_data != dd_data_mux && verbose_mux) {
	printf("  data_mux: dd%d reg%d mem %o mux %o\n",
	       dd_mode, dd_reg, dd_mem_data, dd_data_mux);
    }
}

/*
 * mux sources of pc changes into new pc
 */
void
do_pc_mux(void)
{
    wire trap_or_int;

    trap_or_int = trap || interrupt;

    pc_mux =
	(istate == f1 && !trap_or_int                  ) ? pc + 2 :
	(istate == s1 && (ss_mode == 6 || ss_mode == 7)) ? pc + 2 :
	(istate == d1 && (dd_mode == 6 || dd_mode == 7)) ? pc + 2 :
	(istate == e1 && latch_pc                      ) ? new_pc :
        (istate == o1 || istate == t3                  ) ? pc_mem_data :
	pc;

    if (pc_mux != pc && verbose_mux) {
	printf("  pc_mux: istate %d, ss %d dd %d, latch_pc %d, pc_mux %o\n",
	       istate, ss_mode, dd_mode, latch_pc, pc_mux);
    }
}

void
do_sp_mux(void)
{
    sp_mux =
	(istate == o1 || istate == o2 || istate == o3) ? sp + 2 :
	(istate == p1		     ) ? sp - 2 :
	(istate == t1		     ) ? sp - 2 :
	(istate == t2		     ) ? sp - 2 :
	sp;

    if (sp_mux != sp && verbose_mux) {
	printf(" sp_mux: sp_mux %o\n", sp_mux);
    }
}

void
do_cc_mux(void)
{
    wire16 new_psw = psw;

    if (latch_cc && istate == e1) {
        if (verbose_cc) printf("  new cc: %d %d %d %d\n", 
                               new_cc_n,new_cc_z,new_cc_v,new_cc_c);

        new_psw &= ~017;
        if (new_cc_n) new_psw |= 1 << 3;
        if (new_cc_z) new_psw |= 1 << 2;
        if (new_cc_v) new_psw |= 1 << 1;
        if (new_cc_c) new_psw |= 1 << 0;
    }

    if (new_psw != psw && verbose_psw)
        printf("  cc_mux: new_psw %o\n", new_psw);

    psw = new_psw;
}

void
do_psw_mux(void)
{
    psw_mux =
        (istate == o2 || istate == t4) ? psw_mem_data :
        psw;

    if (psw_mux != psw && verbose_mux) {
        printf(" psw_mux: mux %o\n", psw_mux);
    }
}

void
do_reg_mux(void)
{
    if (istate == c1 && verbose_mux) {
        printf("  reg_mux: ss post %d pre %d reg %d\n",
               ss_post_incr, ss_pre_dec, ss_reg);
        printf("           dd post %d pre %d reg %d\n",
               dd_post_incr, dd_pre_dec, dd_reg);
    }

    if (istate == s1)
    {
        if (ss_post_incr)
        {
            regs[ss_reg] +=
                (need_srcspec_dd_byte && ss_reg < 6 && ss_mode == 2) ? 1 : 2;
            printf(" R%d <- %o (ss r++)\n", ss_reg, regs[ss_reg]);
        }
        else
            if (ss_pre_dec)
            {
                regs[ss_reg] -=
                    (need_srcspec_dd_byte && ss_reg < 6 && ss_mode == 4) ? 1:2;
                printf(" R%d <- %o (ss r--)\n", ss_reg, regs[ss_reg]);
            }
    }

    if (istate == d1)
    {
        if (dd_post_incr)
        {
            regs[dd_reg] +=
                (need_destspec_dd_byte && dd_reg < 6 && dd_mode == 2) ? 1 : 2;
            printf(" R%d <- %o (dd r++)\n", dd_reg, regs[dd_reg]);
        }
        else
            if (dd_pre_dec)
            {
                regs[dd_reg] -=
                    (need_destspec_dd_byte && dd_reg < 6 && dd_mode == 4) ?1:2;
                printf(" R%d <- %o (dd r--)\n", dd_reg, regs[dd_reg]);
            }
    }
}

void decode1(void)
{
    isn_15_12 = (isn >> 12) & 017;	    /* ir[15:12] */
    isn_15_9  = (isn >>	 9) & 0177;
    isn_15_6  = (isn >>	 6) & 01777;
    isn_11_6  = (isn >>	 6) & 077;
    isn_11_9  = (isn >>	 9) & 07;
    isn_5_0   = isn & 077;
    isn_3_0   = isn & 017;

    need_destspec_dd_word =
	(isn_15_6 == 0001) ||	/* jmp */
	(isn_15_6 == 0003) ||	/* swab */
	(isn_15_12 == 0 && (isn_11_6 >= 040 && isn_11_6 <= 063)) || /* jsr-asl */
	(isn_15_12 == 0 && (isn_11_6 >= 065 && isn_11_6 <= 067)) || /* m*,sxt */
	(isn_15_12 >= 001 && isn_15_12 <= 006) ||		    /* mov-add */
//	(isn_15_12 == 007 && (isn_11_9 >= 0 && isn_11_9 <= 4)) ||   /* mul-xor */
	(isn_15_9 >= 0070 && isn_15_9 <= 0074) ||   		    /* mul-xor */
	(isn_15_12 == 010 && (isn_11_6 >= 065 && isn_11_6 <= 066))|| /* mtpx */
	(isn_15_12 == 016);					     /* sub */

    need_destspec_dd_byte =
	(isn_15_12 == 010 && (isn_11_6 >= 050 && isn_11_6 <= 064)) || /* xxxb */
	(isn_15_12 == 010 && (isn_11_6 == 067)) ||		      /* mfps */
	(isn_15_12 >= 011 && isn_15_12 < 016);			      /* xxxb */

    need_destspec_dd = need_destspec_dd_word | need_destspec_dd_byte;

    need_srcspec_dd_word = 
	(isn_15_12 >= 001 && isn_15_12 <= 006) ||	/* mov-add */
	(isn_15_12 == 016);				/* sub */

    need_srcspec_dd_byte = 
	(isn_15_12 >= 011 && isn_15_12 <= 015);		/* movb-sub */

    need_srcspec_dd = need_srcspec_dd_word | need_srcspec_dd_byte;

    no_operand =
	(isn_15_6 == 0000 && isn_5_0 < 010) ||
	(isn_15_6 == 0002 && (isn_5_0 >= 30 && isn_5_0 <= 77)) ||
	(isn_15_12 == 0 && (isn_11_6 >= 004 && isn_11_6 <= 037)) ||
//	(isn_15_12 == 007 && (isn_11_9 == 7)) ||
	(isn_15_9 == 0104) ||				/* trap/emt */
0/*	(isn_15_6 == 0047) */;

    is_illegal =
	(isn_15_6 == 0000 && isn_5_0 > 007) ||
	(isn_15_6 == 0001 && isn_5_0 < 010) ||		/* jmp rx */
	(isn_15_6 == 0002 && (isn_5_0 >= 010 && isn_5_0 <= 027)) ||
	(isn_15_12 == 007 && (isn_11_9 == 5 || isn_11_9 == 6)) ||
	(isn_15_12 == 017);

    need_pop_reg =
	(isn_15_6 == 0002 && (isn_5_0 < 010)) ||	/* rts */
	(isn_15_6 == 0064) ||				/* mark */
	(isn_15_6 == 0070);				/* csm */

    need_pop_pc_psw =					/* rti, rtt */
	(isn_15_6 == 0 && (isn_5_0 == 002 || isn_5_0 == 006));

    need_push_state =
	(isn_15_9 == 004);				/* jsr */

    assert_trap_ill = is_illegal;

    store_result32 =
	(isn_15_9 == 0070) ||				/* mul */
	(isn_15_9 == 0071) ||				/* div */
	(isn_15_9 == 0072);				/* ashc */

    store_result = !no_operand &&
        !store_result32 &&
	!(isn_15_6 == 00001) &&				/* jmp */
	!(isn_15_6 == 00057) && !(isn_15_6 == 01057) &&	/* tst/tstb */
	!(isn_15_12 == 002) && !(isn_15_12 == 012) &&	/* cmp/cmpb */
	!(isn_15_12 == 003) &&				/* bit */
	!(isn_15_9 == 0004) &&					/* jsr */
	!((isn_15_6 >= 01000) && (isn_15_6 <= 01037)) && 	/* bcs-blo */
	!((isn_15_6 >= 00004) && (isn_15_6 <= 00034)); 		/* br-ble */

    need_dest_data = 
	!(isn_15_12 == 001) &&				/* mov */
	!(isn_15_12 == 011) &&				/* movb */
        !(isn_15_9 == 004) &&				/* jsr */
        !((isn_15_6 == 00050) || (isn_15_6 == 01050)) && /* clr/clrb */
        !(isn_15_6 == 00001);				/* jmp */

    is_isn_byte = (isn & (u16)0100000 ? 1 : 0) &&
	!(isn_15_12 == 016);				/* sub */

    is_isn_rdd = 
	(isn_15_9 == 004) ||				/* jsr */
	(isn_15_9 == 074) ||				/* xor */
	(isn_15_9 == 077);				/* sob */

    is_isn_rss = 
	(isn_15_9 == 070) ||				/* mul */
	(isn_15_9 == 071) ||				/* div */
	(isn_15_9 == 072) ||				/* ashc */
	(isn_15_9 == 073);				/* ash */

    is_isn_rxx = is_isn_rdd || is_isn_rss;

    is_isn_r32 =
	(isn_15_9 == 071);				/* div */

    need_src_data =
        !((isn_15_6 == 00050) || (isn_15_6 == 01050));	/* clr/clrb */

    printf("c1: isn %06o ss %d, dd %d, no_op %d, ill %d, push %d, pop %d\n",
	   isn, need_srcspec_dd, need_destspec_dd,
	   no_operand, is_illegal, need_push_state, need_pop_reg);

    printf("    need_src_data %d, need_dest_data %d\n",
           need_src_data, need_dest_data);

    /* ea setup */
    ss_mode = (isn >> 9) & 7;
    ss_reg = (isn >> 6) & 7;

    ss_dest_mem = ss_mode != 0;
    ss_dest_reg = ss_mode == 0;
    ss_ea_ind = ss_mode == 7;

    store_ss_reg = (isn_15_9 == 004 && ss_reg != 7);	/* jsr */

    ss_post_incr =
        need_srcspec_dd &&
        (ss_mode == 2 || ss_mode == 3);

    ss_pre_dec =
        need_srcspec_dd &&
        (ss_mode == 4 || ss_mode == 5);

    printf(" ss: mode%d reg%d ind%d post %d pre %d\n",
	   ss_mode, ss_reg, ss_ea_ind,
	   ss_post_incr, ss_pre_dec);

    /* */
    dd_mode = (isn >> 3) & 7;
    dd_reg = (isn >> 0) & 7;

    dd_dest_mem = dd_mode != 0;
    dd_dest_reg = dd_mode == 0;
    dd_ea_ind = dd_mode == 7;

    dd_post_incr =
        need_destspec_dd &&
        (dd_mode == 2 || dd_mode == 3);

    dd_pre_dec =
        need_destspec_dd &&
        (dd_mode == 4 || dd_mode == 5);

    printf(" dd: mode%d reg%d ea %06o ind%d post %d pre %d\n",
	   dd_mode, dd_reg, dd_ea, dd_ea_ind,
	   dd_post_incr, dd_pre_dec);

    /* */
    need_s1 = need_srcspec_dd;
    need_d1 = need_destspec_dd;

    need_s2 = need_srcspec_dd && (ss_mode == 3 || ss_mode >= 5);
    need_d2 = need_destspec_dd && (dd_mode == 3 || dd_mode >= 5);

    need_s4 = need_srcspec_dd && ss_mode != 0 && need_src_data;
    need_d4 = need_destspec_dd && dd_mode != 0 && need_dest_data;

    printf(" need: dest_data %d; s1 %d, s2 %d, s4 %d; d1 %d, d2 %d, d4 %d\n", 
           need_dest_data,
           need_s1, need_s2, need_s4, need_d1, need_d2, need_d4);
}

wire22 se_addr(wire22 addr)
{
//    if (addr & 0x8000) return addr | 0x3f0000;
    if (addr >= 0xe000) return addr | 0x3f0000;
    return addr;
}

void source1(void)
{
    printf("s1:\n");
}

void source2(void)
{
    ss_mem_data = read_mem(se_addr(ss_ea_mux));
    printf("s2: ss_ea_mux %6o, [ea]=%6o\n", ss_ea_mux, ss_mem_data);
}

void source3(void)
{
    printf("s3: ss_ea_mux %6o\n", ss_ea_mux);
    ss_mem_data = read_mem(se_addr(ss_ea_mux));
}

void source4(void)
{
    printf("s4: ss_ea_mux %6o\n", ss_ea_mux);
    ss_mem_data = is_isn_byte ?
        read_mem_byte(se_addr(ss_ea_mux)) :
        read_mem(se_addr(ss_ea_mux));
}

void dest1(void)
{
    printf("d1: dd_ea %6o, dd_ea_mux %6o\n", dd_ea, dd_ea_mux);
}

void dest2(void)
{
    printf("d2: dd_ea %6o, dd_ea_mux %6o\n", dd_ea, dd_ea_mux);
    dd_mem_data = read_mem(se_addr(dd_ea));
}

void dest3(void)
{
    printf("d3:\n");
    dd_mem_data = read_mem(se_addr(dd_ea_mux));
}

void dest4(void)
{
    printf("d4:\n");
    dd_mem_data = is_isn_byte ?
        read_mem_byte(se_addr(dd_ea_mux)) :
        read_mem(se_addr(dd_ea_mux));
}

wire sign_l(wire32 v)
{
    wire result;
    result = v & 0x80000000 ? 1 : 0;
    return result;
}

wire sign_w(wire16 v)
{
    wire result;
    result = v & 0x8000 ? 1 : 0;
    return result;
}

wire sign_b(wire16 v)
{
    wire result;
    result = v & 0x80 ? 1 : 0;
    return result;
}

wire zero_l(wire32 v)
{
    wire result;
    result = v & 0xffffffff ? 0 : 1;
    return result;
}

wire zero_w(wire16 v)
{
    wire result;
    result = v & 0xffff ? 0 : 1;
    return result;
}

wire zero_b(wire16 v)
{
    wire result;
    result = v & 0xff ? 0 : 1;
    return result;
}

void execute(void)
{
    printf("e1:\n");

    assert_halt = 0;
    assert_wait = 0;
    assert_trap_priv = 0;
    assert_trap_emt = 0;
    assert_trap_trap = 0;
    assert_bpt = 0;
    assert_iot = 0;
    assert_reset = 0;

    e1_result = 0;

    new_pc = 0;
    latch_pc = 0;

    new_cc_n = cc_n;
    new_cc_z = cc_z;
    new_cc_v = cc_v;
    new_cc_c = cc_c;
    latch_cc = 0;

    if (verbose_data) {
        printf(" ss_data %6o, dd_data %6o\n", ss_data, dd_data);
        printf(" ss_ea   %6o, dd_ea   %6o\n", ss_ea, dd_ea);
    }

    if (isn_15_12 == 0) {
        if (0) printf("e: isn_15_12 == 0\n");

	if (isn_11_6 == 000 && isn_5_0 < 010) {
	    if (0) printf("e: MS\n");
	    switch (isn & 7) {

	    case 0:					    /* halt */
		printf("e: HALT\n");
		if (current_mode == mode_kernel)
		    assert_halt = 1;
		else
		    assert_trap_priv = 1;
		break;

	    case 1:					    /* wait */
		printf("e: WAIT\n");
		assert_wait = 1;
		break;

	    case 3:					    /* bpt */
		assert_bpt = 1;
		break;

	    case 4:					    /* iot */
		assert_iot = 1;
		break;

	    case 5:					    /* reset */
		if (current_mode == mode_kernel)
		    assert_reset = 1;
		break;

	    case 2:					    /* rti */
		printf("e: RTI\n");
                break;

	    case 6:					    /* rtt */
		printf("e: RTT\n");
		break;

	    case 7:					    /* mfpt */
		e1_result = 0x1234;
		break;
	    }
	}
	else {
	    if (0) printf("e: pc & cc %o\n", isn_11_6);

	    switch (isn_11_6) {

	    case 001:					    /* jmp */
		printf("e: JMP; dest_ea %6o\n", dd_ea);
		new_pc = dd_ea;
		latch_pc = 1;
		break;

	    case 002:					    /* rts */
                switch(isn_5_0) {
                case 000: case 001: case 002: case 003:
                case 004: case 005: case 006: case 007:
                    printf("e: RTS\n");
                    new_pc = dd_data;
                    latch_pc = 1;
                    break;

                case 030:				    /* spl */
//xxx
                    break;

                    /* ccc	cc	-	000257 */
                    /* cln	cc	-	000250 */
                    /* clz	cc	-	000244 */
                    /* clv	cc	-	000242 */
                    /* clc	cc	-	000241 */
                    /* c	cc	-	000240 */
                case 040: case 041: case 042: case 043:
                case 044: case 045: case 046: case 047:
                case 050: case 051: case 052: case 053:
                case 054: case 055: case 056: case 057:
                    if (isn_3_0 & 010) new_cc_n = 0;
                    if (isn_3_0 & 004) new_cc_z = 0;
                    if (isn_3_0 & 002) new_cc_v = 0;
                    if (isn_3_0 & 001) new_cc_c = 0;
                    break;
                    /* scc	cc	-	000277 */
                    /* sen	cc	-	000270 */
                    /* sez	cc	-	000264 */
                    /* sev	cc	-	000262 */
                    /* sec	cc	-	000261 */
                    /* s	cc	-	000260 */
                case 060: case 061: case 062: case 063:
                case 064: case 065: case 066: case 067:
                case 070: case 071: case 072: case 073:
                case 074: case 075: case 076: case 077:
                    if (isn_3_0 & 010) new_cc_n = 1;
                    if (isn_3_0 & 004) new_cc_z = 1;
                    if (isn_3_0 & 002) new_cc_v = 1;
                    if (isn_3_0 & 001) new_cc_c = 1;
                    latch_cc = 1;
                    break;
                }
		break;

	    case 003:					    /* swab */
		printf("e: SWAB\n");
		e1_result = ((dd_data & 0xff00) >> 8) | ((dd_data & 0xff) << 8);

		new_cc_n = sign_b(e1_result);
		new_cc_z = zero_b(e1_result);
		new_cc_v = 0;
		new_cc_c = 0;
		latch_cc = 1;
		break;

#define new_pc_w new_pc = (pc + ((isn + isn) & 0377)) & 0177777
#define new_pc_b new_pc = (pc + ((isn + isn) | 0177400)) & 0177777

	    case 004: case 005:				    /* br */
		new_pc_w;
		latch_pc = 1;
		printf("e: br; isn %o, pc %o, new_pc %o\n", isn, pc, new_pc);
		break;

	    case 006: case 007:				    /* br */
		new_pc_b;
		latch_pc = 1;
		break;

	    case 010: case 011:				    /* bne */
		new_pc_w;
		latch_pc = cc_z ? 0 : 1;
		break;

	    case 012: case 013:				    /* bne */
		new_pc_b;
		latch_pc = cc_z ? 0 : 1;
		break;

	    case 014: case 015:				    /* beq */
		new_pc_w;
		latch_pc = cc_z ? 1 : 0;
		break;

	    case 016: case 017:				    /* beq */
		new_pc_b;
		latch_pc = cc_z ? 1 : 0;
		break;

	    case 020: case 021:				    /* bge */
		new_pc_w;
		latch_pc = (cc_n ^ cc_v) ? 0 : 1;
		break;

	    case 022: case 023:				    /* bge */
		new_pc_b;
		latch_pc = (cc_n ^ cc_v) ? 0 : 1;
		break;

	    case 024: case 025:				    /* blt */
		new_pc_w;
		latch_pc = (cc_n ^ cc_v) ? 1 : 0;
		break;

	    case 026: case 027:				    /* blt */
		new_pc_b;
		latch_pc = (cc_n ^ cc_v) ? 1 : 0;
		break;

	    case 030: case 031:				    /* bgt */
		new_pc_w;
		latch_pc = (cc_z | cc_n ^ cc_v) ? 0 : 1;
		break;

	    case 032: case 033:				    /* bgt */
		new_pc_b;
		latch_pc = (cc_z | cc_n ^ cc_v) ? 0 : 1;
		break;

	    case 034: case 035:				    /* ble */
		new_pc_w;
		latch_pc = (cc_z | cc_n ^ cc_v) ? 1 : 0;
		break;

	    case 036: case 037:				    /* ble */
		new_pc_b;
		latch_pc = (cc_z | cc_n ^ cc_v) ? 1 : 0;
		break;

	    case 040: case 041: case 042: case 043:	    /* jsr */
	    case 044: case 045: case 046: case 047:
		printf(" JSR r%d; dd_data %6o, dd_ea %6o\n", ss_reg, dd_data, dd_ea);
		e1_result = pc;
		new_pc = dd_ea;
		latch_pc = 1;
		break;

	    case 050:					    /* clr */
		new_cc_n = 0;
		new_cc_v = 0;
		new_cc_c = 0;
                new_cc_z = 1;
		latch_cc = 1;
		e1_result = 0;
		break;

	    case 051:					    /* com */
		e1_result = dd_data ^ 0177777;
		new_cc_n = sign_w(e1_result);
		new_cc_z = zero_w(e1_result);
		new_cc_v = 0;
		new_cc_c = 1;
		latch_cc = 1;
		break;

	    case 052:					    /* inc */
		e1_result = dd_data + 1;
		new_cc_n = sign_w(e1_result);
		new_cc_z = zero_w(e1_result);
		new_cc_v = (e1_result == 0100000) ? 1 : 0;
		latch_cc = 1;
		break;

	    case 053:					    /* dec */
		e1_result = dd_data - 1;
		new_cc_n = sign_w(e1_result);
		new_cc_z = zero_w(e1_result);
		new_cc_v = (e1_result == 077777) ? 1 : 0;
		latch_cc = 1;
		break;

	    case 054:					    /* neg */
		e1_result = -dd_data;
		new_cc_n = sign_w(e1_result);
		new_cc_z = zero_w(e1_result);
		new_cc_v = (e1_result == 0100000) ? 1 : 0;
		new_cc_c = new_cc_z ^ 1 ? 1 : 0;
		latch_cc = 1;
		break;

	    case 055:					    /* adc */
		e1_result = dd_data + cc_c;
		new_cc_n = sign_w(e1_result);
		new_cc_z = zero_w(e1_result);
		new_cc_v = (cc_c && (e1_result == 0100000));
		new_cc_c = cc_c & new_cc_z ? 1 : 0;
		latch_cc = 1;
		break;

	    case 056:					    /* sbc */
		e1_result = dd_data - cc_c;
		new_cc_n = sign_w(e1_result);
		new_cc_z = zero_w(e1_result);
		new_cc_v = (cc_c && (e1_result == 0100000));
		new_cc_c = cc_c & new_cc_z ? 1 : 0;
		latch_cc = 1;
		break;

	    case 057:					    /* tst */
		e1_result = dd_data;
		new_cc_n = sign_w(e1_result);
		new_cc_z = zero_w(e1_result);
		new_cc_v = 0;
		new_cc_c = 0;
		latch_cc = 1;
		break;

	    case 060:					    /* ror */
		e1_result = (dd_data >> 1) | (cc_c << 15);
		new_cc_n = sign_w(e1_result);
		new_cc_z = zero_w(e1_result);
		new_cc_c = dd_data & 1 ? 1 : 0;
		new_cc_v = new_cc_n ^ new_cc_c ? 1 : 0;
		latch_cc = 1;
		break;

	    case 061:					    /* rol */
		e1_result = (dd_data << 1) | cc_c;
		new_cc_n = sign_w(e1_result);
		new_cc_z = zero_w(e1_result);
		new_cc_c = sign_w(e1_result);
		new_cc_v = new_cc_n ^ new_cc_c ? 1 : 0;
		latch_cc = 1;
		break;

	    case 062:					    /* asr */
		e1_result = (dd_data >> 1) | (dd_data & 0100000);
		new_cc_n = sign_w(e1_result);
		new_cc_z = zero_w(e1_result);
		new_cc_c = (dd_data & 1) ? 1 : 0;
		new_cc_v = (new_cc_n ^ new_cc_c) ? 1 : 0;
		latch_cc = 1;
		break;

	    case 063:					    /* asl */
		e1_result = dd_data << 1;
		new_cc_n = sign_w(e1_result);
		new_cc_z = zero_w(e1_result);
		new_cc_c = sign_w(e1_result);
		new_cc_v = new_cc_n ^ new_cc_c ? 1 : 0;
		latch_cc = 1;
		break;

	    case 064:					    /* mark */
	    case 065:					    /* mfpi */
	    case 066:					    /* mtpi */
	    case 067:					    /* sxt */
	    case 070:					    /* csm */
	    case 072:					    /* tstset */
	    case 073:					    /* wrtlck */
		break;

	    }
	}
    }
    else {
        if (0) printf("e: isn_15_12 != 0 (%o)\n", isn_15_12);
	switch (isn_15_12) {
	case 001:					    /* mov */
	    e1_result = ss_data;
	    new_cc_n = sign_w(e1_result);
	    new_cc_z = zero_w(e1_result);
	    new_cc_v = 0;
	    latch_cc = 1;
	    break;

	case 002:					    /* cmp */
	    //printf(" CMP %6o %6o\n", ss_data, dd_data);
	    e1_result = ss_data - dd_data;
	    new_cc_n = sign_w(e1_result);
	    new_cc_z = zero_w(e1_result);
	    new_cc_v = sign_w((ss_data ^ dd_data) &
                              (~dd_data ^ e1_result));
	    new_cc_c = ss_data < dd_data ? 1 : 0;
	    latch_cc = 1;
	    break;

	case 003:					    /* bit */
	    e1_result = ss_data & dd_data;
	    new_cc_n = sign_w(e1_result);
	    new_cc_z = zero_w(e1_result);
	    new_cc_v = 0;
	    latch_cc = 1;
	    break;

	case 004:					    /* bic */
	    e1_result = ~ss_data & dd_data;
	    new_cc_n = sign_w(e1_result);
	    new_cc_z = zero_w(e1_result);
	    new_cc_v = 0;
	    latch_cc = 1;
	    break;

	case 005:					    /* bis */
	    e1_result = ss_data | dd_data;
	    new_cc_n = sign_w(e1_result);
	    new_cc_z = zero_w(e1_result);
	    new_cc_v = 0;
	    latch_cc = 1;
	    break;

	case 006:					    /* add */
	    e1_result = ss_data + dd_data;
	    new_cc_n = sign_w(e1_result);
	    new_cc_z = zero_w(e1_result);
	    new_cc_v = sign_w((~ss_data ^ dd_data) &
                              (ss_data ^ e1_result));
	    new_cc_c = (e1_result < ss_data) ? 1 : 0;
	    latch_cc = 1;
	    break;

	case 007:
	    switch (isn_11_9) {
		unsigned short temp, sign, shift;

	    case 0:					    /* mul */
                printf(" MUL %o %o\n", ss_data, dd_data);
		e32_result = ss_data * dd_data;
		new_cc_n = e32_result & 0x80000000 ? 1 : 0;
		new_cc_z = e32_result & 0xffffffff ? 0 : 1;
		new_cc_v = 0;
		new_cc_c = (((int)e32_result > 077777) ||
			    ((int)e32_result < -0100000)) ? 1 : 0;
		latch_cc = 1;
		break;

	    case 1:					    /* div */
		if (dd_data == 0) {
		    new_cc_n = 0;
		    new_cc_z = 1;
		    new_cc_v = 1;
		    new_cc_c = 1;
		    latch_cc = 1;
		}
		else if ((regs[ss_reg] == 0x8000 &&
			  regs[ss_reg | 1] == 0) &&
			 (dd_data == 0177777))
		{
		    new_cc_v = 1;
		    new_cc_z = 1;
		    new_cc_n = 1;
		    new_cc_c = 1;
		    latch_cc = 1;
		}
		else {
		    e32_result =
			((regs[ss_reg] << 16) | regs[ss_reg | 1]) /
			dd_data;
		    if ((e32_result > 077777) || (e32_result < -0100000)) {
			new_cc_n = sign_l(e32_result);
			new_cc_v = 1;
			new_cc_z = 0;
			new_cc_c = 0;
			latch_cc = 1;
		    } else {
			new_cc_n = sign_l(e32_result);
			new_cc_z = zero_l(e32_result);
			new_cc_v = 0;
			new_cc_c = 0;
			latch_cc = 1;
		    }
		}
		break;

	    case 2:					    /* ash */
		sign = sign_w(ss_data);
		shift = dd_data & 077;

		if (shift == 0) {			/* [0] */
		    e1_result = ss_data;
		    new_cc_v = new_cc_c = 0;
		    latch_cc = 1;
		}
		else if (shift <= 15) {			/* [1,15] */
		    e1_result = ss_data << shift;
		    temp = ss_data >> (16 - shift);
		    new_cc_v = (temp != ((e1_result & 0100000)? 0177777: 0));
		    new_cc_c = (temp & 1);
		}
		else if (shift <= 31) {			/* [16,31] */
		    e1_result = 0;
		    new_cc_v = (ss_data != 0);
		    new_cc_c = (ss_data << (shift - 16)) & 1;
		}
		else if (shift == 32) {			/* [32] = -32 */
		    e1_result = -sign;
		    new_cc_v = 0;
		    new_cc_c = 0;
		}
		else {					/* [33,63] = -31,-1 */
		    e1_result =
			(ss_data >> (64 - shift)) |
			(-sign << (shift - 32));
		    new_cc_v = 0;
		    new_cc_c = ((ss_data >> (63 - shift)) & 1);
		}

		new_cc_n = sign_w(e1_result);
		new_cc_z = zero_w(e1_result);
		break;

	    case 3:					    /* ashc */
		sign = sign_w(ss_data);
		shift = dd_data & 077;

		if (dd_data == 0) {			/* [0] */
		    e32_result = ss_data;
		    new_cc_v = 0;
		    new_cc_c = 0;
		}
		else if (shift <= 31) {			/* [1,31] */
		    e32_result = ss_data << shift;
		    temp = (ss_data >> (32 - shift)) | (-sign << shift);
		    new_cc_v = (temp != ((e32_result & 0x80000000)? -1 : 0)) ? 1 : 0;
		    new_cc_c = (temp & 1) ? 1 : 0;
		}
		else if (shift == 32) {			/* [32] = -32 */
		    e32_result = -sign;
		    new_cc_v = 0;
		    new_cc_c = (ss_data >> 31) & 1 ? 1 : 0;
		}
		else {					/* [33,63] = -31,-1 */
		    e32_result = (ss_data >> (64 - shift)) | (-sign << (shift - 32));
		    new_cc_v = 0;
		    new_cc_c = ((ss_data >> (63 - shift)) & 1);
		}

		new_cc_n = sign_l(e32_result);
		new_cc_z = sign_l(e32_result);
		break;

	    case 4:					    /* xor */
		e1_result = ss_data ^ dd_data;
		new_cc_n = sign_w(e1_result);
		new_cc_z = zero_w(e1_result);
		new_cc_v = 0;
		latch_cc = 1;
		break;

	    case 5:					    /* fis */
		break;

	    case 6:					    /* cis */
		break;

	    case 7:					    /* sob */
		e1_result = ss_data - 1;
		new_pc = pc - dd_data - dd_data;
		latch_pc = e1_result == 0 ? 0 : 1;
		break;
            }
            break;

        case 010:
            if (1) printf(" e: 010 isn_11_6 %o\n", isn_11_6);
            switch (isn_11_6) {
            case 000: case 001:				/* bpl */
                printf("e: BPL\n"); 
               new_pc_w;
                latch_pc = cc_n == 0 ? 1 : 0;
                break;

            case 002: case 003:				/* bpl */
                printf("e: BPLB\n");
                new_pc_b;
                latch_pc = cc_n == 0 ? 1 : 0;
                break;

            case 004: case 005:				/* bmi */
                new_pc_w;
                latch_pc = cc_n ? 1 : 0;
                break;

            case 006: case 007:				/* bmi */
                new_pc_b;
                latch_pc = cc_n ? 1 : 0;
                break;

            case 010: case 011:				/* bhi */
                new_pc_w;
                latch_pc = (cc_c | cc_z) == 0 ? 1 : 0;
                break;

            case 012: case 013:				/* bhi */
                new_pc_b;
                latch_pc = (cc_c | cc_z) == 0 ? 1 : 0;
                break;

            case 014: case 015:				/* blos */
                new_pc_w;
                latch_pc = (cc_c | cc_z) ? 1 : 0;
                break;

            case 016: case 017:				/* blos */
                new_pc_b;
                latch_pc = (cc_c | cc_z) ? 1 : 0;
                break;

            case 020: case 021:				/* bvc */
                new_pc_w;
                latch_pc = cc_v == 0 ? 1 : 0;
                break;

            case 022: case 023:				/* bvc */
                new_pc_b;
                latch_pc = cc_v == 0 ? 1 : 0;
                break;

            case 024: case 025:				/* bvs */
                new_pc_w;
                latch_pc = cc_v ? 1 : 0;
                break;

            case 026: case 027:				/* bvs */
                new_pc_b;
                latch_pc = cc_v ? 1 : 0;
                break;

            case 030: case 031:				/* bcc */
                new_pc_w;
                latch_pc = cc_c == 0 ? 1 : 0;
                break;

            case 032: case 033:				/* bcc */
                new_pc_b;
                latch_pc = cc_c == 0 ? 1 : 0;
                break;

            case 034: case 035:				/* bcs */
                new_pc_w;
                latch_pc = cc_c ? 1 : 0;
                break;

            case 036: case 037:				/* bcs */
                new_pc_b;
                latch_pc = cc_c ? 1 : 0;
                break;

            case 040: case 041: case 042: case 043:	/* emt */
                printf(" EMT\n");
                assert_trap_emt = 1;
                break;

            case 044: case 045: case 046: case 047:	/* trap */
                printf(" TRAP\n");
                assert_trap_trap = 1;
                break;

            case 050:					/* clrb */
                new_cc_n = 0;
                new_cc_v = 0;
                new_cc_c = 0;
                new_cc_z = 1;
                latch_cc = 1;

                e1_result = dd_data & 0xff00;
                break;

            case 051:					/* comb */
                e1_result = (dd_data ^ 0377) & 0377;
                new_cc_n = sign_b(e1_result);
                new_cc_z = zero_b(e1_result);
                new_cc_v = 0;
                new_cc_c = 1;
                latch_cc = 1;
//note: byte write of src - rmw to memory word
                break;

            case 052:					/* incb */
                e1_result = (dd_data & 0xff00) |
                    ((dd_data + 1) & 0377);
                new_cc_n = sign_b(e1_result);
                new_cc_z = zero_b(e1_result);
                new_cc_v = 0;
                latch_cc = 1;
//note: byte write of src - rmw to memory word
                break;

            case 053:					/* decb */
                e1_result = (dd_data & 0xff00) |
                    ((dd_data - 1) & 0377);
                new_cc_n = sign_b(e1_result);
                new_cc_z = zero_b(e1_result);
                new_cc_v = (e1_result & 0xff) == 0177;
                latch_cc = 1;
//note: byte write of src - rmw to memory word
                break;

            case 054:					/* negb */
                e1_result = (dd_data & 0xff00) |
                    -(dd_data & 0377);
                new_cc_n = sign_b(e1_result);
                new_cc_z = zero_b(e1_result);
                new_cc_v = (e1_result & 0xff) == 0200;
                new_cc_c = new_cc_z ^ 1;
                latch_cc = 1;
//note: byte write of src - rmw to memory word
                break;

            case 055:					/* adcb */
                e1_result = (dd_data & 0xff00) |
                    ((dd_data + cc_c) & 0377);
                new_cc_n = sign_b(e1_result);
                new_cc_z = zero_b(e1_result);
                new_cc_v = cc_c && (e1_result & 0xff) == 0200;
                new_cc_c = cc_c & new_cc_z;
                latch_cc = 1;
//note: byte write of src - rmw to memory word
                break;

            case 056:					/* sbcb */
                e1_result = (dd_data & 0xff00) |
                    ((dd_data - cc_c) & 0377);
                new_cc_n = sign_b(e1_result);
                new_cc_z = zero_b(e1_result);
                new_cc_v = cc_c && (e1_result & 0xff) == 0177;
                new_cc_c = cc_c && (e1_result & 0xff) == 0377;
                latch_cc = 1;
//note: byte write of src - rmw to memory word
                break;

            case 057:					/* tstb */
                printf(" TSTB %o\n", dd_data & 0xff);
                e1_result = dd_data & 0xff;
                printf(" TSTB %o, e1_result 0x%x\n", dd_data & 0xff, e1_result);
                new_cc_n = sign_b(e1_result);
                new_cc_z = zero_b(e1_result);
                new_cc_v = 0;
                new_cc_c = 0;
                latch_cc = 1;
                break;

            case 060:					/* rorb */
                e1_result = (dd_data & 0xff00) |
                    (((dd_data & 0377) >> 1) | cc_c << 7);
                new_cc_n = sign_b(e1_result);
                new_cc_z = zero_b(e1_result);
                new_cc_c = dd_data & 1;
                new_cc_v = new_cc_n ^ new_cc_c;
                latch_cc = 1;
//note: byte write of src - rmw to memory word
                break;

            case 061:					/* rolb */
                e1_result = (dd_data & 0xff00) |
                    (((dd_data & 0377) << 1) | cc_c);
                new_cc_n = sign_b(e1_result);
                new_cc_z = zero_b(e1_result);
                new_cc_c = dd_data & 0x80;
                new_cc_v = new_cc_n ^ new_cc_c;
                latch_cc = 1;
//note: byte write of src - rmw to memory word
                break;

            case 062:					/* asrb */
                e1_result = (dd_data & 0xff00) |
                    (((dd_data & 0377) >> 1) | dd_data & 0200);
                new_cc_n = sign_b(e1_result);
                new_cc_z = zero_b(e1_result);
                new_cc_c = ss_data & 1;
                new_cc_v = new_cc_n ^ new_cc_c;
                latch_cc = 1;
//note: byte write of src - rmw to memory word
                break;

            case 063:					/* aslb */
                e1_result = (dd_data & 0xff00) |
                    ((dd_data & 0377) << 1);
                new_cc_n = sign_b(e1_result);
                new_cc_z = zero_b(e1_result);
                new_cc_c = ss_data & 0x80;
                new_cc_v = new_cc_n ^ new_cc_c;
                latch_cc = 1;
//note: byte write of src - rmw to memory word
                break;

            case 064:					/* mtps */
                if (current_mode == mode_kernel) {
//			  ipl = (dd_data >> psw_v_ipl) & 07;
//			  trap_req = calc_ints (ipl, trap_req);
                }

                new_cc_n = (dd_data >> PSW_N) & 01;
                new_cc_z = (dd_data >> PSW_Z) & 01;
                new_cc_v = (dd_data >> PSW_V) & 01;
                new_cc_c = (dd_data >> PSW_C) & 01;
                latch_cc = 1;
                break;

            case 065:					/* mfpd */
                new_cc_n = sign_w(dd_data);
                new_cc_z = zero_w(dd_data);
                new_cc_v = 0;
                latch_cc = 1;
// push data
// sp <= sp - 2
                break;

            case 066:					/* mtpd */
                new_cc_n = sign_w(dd_data);
                new_cc_z = zero_w(dd_data);
                new_cc_v = 0;
// pop data
                break;

            case 067:					/* mfps */
                new_cc_n = sign_w(dd_data);
                new_cc_z = zero_w(dd_data);
                new_cc_v = 0;
                e1_result = (dd_data & 0200) ? 0177400 | dd_data: dd_data;
                break;

            }
            break;

	case 011:					    /* movb */
	    e1_result = /*(dd_data & 0xff00) |*/
		(ss_data & 0377);
	    new_cc_n = sign_b(e1_result);
	    new_cc_z = zero_b(e1_result);
	    new_cc_v = 0;
	    latch_cc = 1;
	    break;

	case 012:					    /* cmpb */
	    e1_result = (ss_data & 0377) - (dd_data & 0377);
	    new_cc_n = sign_b(e1_result);
	    new_cc_z = zero_b(e1_result);
	    new_cc_v = sign_b( ((ss_data&0xff) ^ (dd_data&0xff)) &
			       (~(dd_data&0xff) ^ (e1_result&0xff)) );
	    new_cc_c = (ss_data & 0xff) < (dd_data & 0xff) ? 1 : 0;
	    latch_cc = 1;
	    break;

	case 013:					    /* bitb */
	    e1_result = (ss_data & 0377) & (dd_data & 0377);
	    new_cc_n = sign_b(e1_result);
	    new_cc_z = zero_w(e1_result);
	    new_cc_v = 0;
	    latch_cc = 1;
	    break;

	case 014:					    /* bicb */
	    e1_result = (ss_data & 0377) & ~(dd_data & 0377);
	    new_cc_n = sign_b(e1_result);
	    new_cc_z = zero_w(e1_result);
	    new_cc_v = 0;
	    latch_cc = 1;
	    break;

	case 015:					    /* bisb */
	    e1_result = (ss_data & 0377) | (dd_data & 0377);
	    new_cc_n = sign_b(e1_result);
	    new_cc_z = zero_w(e1_result);
	    new_cc_v = 0;
	    latch_cc = 1;
	    break;

	case 016:					    /* sub */
	    if (0) printf(" SUB %6o %6o\n", ss_data, dd_data);
	    e1_result = (dd_data - ss_data) & 0xffff;
	    new_cc_n = sign_w(e1_result);
	    new_cc_z = zero_w(e1_result);
	    new_cc_v = sign_w(((int)ss_data ^ (int)dd_data) &
                              (~(int)ss_data ^ (int)e1_result));
	    new_cc_c = (int)dd_data < (int)ss_data ? 1 : 0;
	    latch_cc = 1;
	    break;
	}
    }

    if (verbose_data) {
        printf(" ss_data %6o, dd_data %6o, e1_result %6o\n",
               ss_data, dd_data, e1_result);
        printf(" latch_pc %d, latch_cc %d\n", latch_pc, latch_cc);
        printf(" psw %o\n", psw);
    }
}

void writeback1(void)
{
    printf("w1: dd%d %d, dd_data %o, ss%d %d, ss_data %o, e1_data %o\n",
	   dd_mode, dd_reg, dd_data, ss_mode, ss_reg, ss_data, e1_data);
    printf("    dd_ea_mux %06o, store_result %d, store_ss_reg %d, store_32 %d\n",
	   dd_ea_mux, store_result, store_ss_reg, store_result32);

    if (store_result && dd_dest_mem) {
        if (is_isn_byte)
            write_mem_byte(se_addr(dd_ea_mux), e1_data);
        else
            write_mem(se_addr(dd_ea_mux), e1_data);
    }
    else if (store_result && dd_dest_reg) {
	printf(" r%d <- %06o (dd)\n", dd_reg, e1_data);
//        if (is_isn_byte)
//            regs[dd_reg] = (regs[dd_reg] & 0xff00) | (e1_data & 0xff);
//        else
            regs[dd_reg] = e1_data;
    }
    else if (store_ss_reg) {
	printf(" r%d <- %06o (ss)\n", ss_reg, e1_data);
	regs[ss_reg] = e1_data;
    }
    else if (store_result32) {
	printf(" r%d <- %06o (e32)\n", ss_reg, e32_result >> 16);
	printf(" r%d <- %06o (e32)\n", ss_reg|1, e32_result & 0xffff);
	regs[ss_reg    ] = e32_result >> 16;
	regs[ss_reg | 1] = e32_result & 0xffff;
    }

}

void pop1(void) {
    printf("o1:\n");
    pc_mem_data = read_mem(sp);
}

void pop2(void) {
    printf("o2:\n");
    psw_mem_data = read_mem(sp);
}

void pop3(void) {
    printf("o3:\n");
    pop_mem_data = read_mem(sp);
}

void push1(void) {
    printf("p1:\n");
    write_mem(sp-2, regs[ss_reg]);
}

void trap1(void) {
    printf("t1: sp %o\n", sp);
    write_mem(sp-2, psw);
}

void trap2(void) {
    printf("t2:\n");
    write_mem(sp-2, pc);
}

void trap3(void) {
    printf("t3: ss_ea %o, ss_ea_mux %o\n", ss_ea, ss_ea_mux);
    pc_mem_data = read_mem(ss_ea_mux);
}

void trap4(void) {
    printf("t4: ss_ea %o, ss_ea_mux %o\n", ss_ea, ss_ea_mux);
    psw_mem_data = read_mem(ss_ea_mux);
}

void wait1(void) {
}

void check_for_traps(void)
{
    wire ok_to_assert_trap;
    wire ok_to_reset_trap;

    /* */
    ok_to_assert_trap =
        istate == f1 || istate == c1 || istate == e1 ||
        istate == s4 || istate == d4 || istate == w1;

    ok_to_reset_trap = istate == t4;

    if (ok_to_reset_trap) {
        /* */
        trap_odd = 0; /* f1 */
        trap_ill = 0; /* c1 */

        trap_priv = 0; /* e1 */
        trap_bpt = 0;
        trap_iot = 0;
        trap_emt = 0;
        trap_trap = 0;

        trap_bus = 0;

        trap = 0;
    }

    if (ok_to_assert_trap) {

        if (assert_trap_odd && istate == f1) {
            trap_odd = 1;
        }

        if (assert_trap_bus) {
            trap_bus = 1;
            assert_trap_bus = 0;
        }

        if (assert_trap_ill && istate == c1) {
            trap_ill = 1;
        }

        if (istate == e1) {
            if (assert_trap_priv) {
                trap_priv = 1;
            }

            if (assert_bpt) {
                trap_bpt = 1;
            }

            if (assert_iot) {
                trap_iot = 1;
            }

            if (assert_trap_emt) {
                trap_emt = 1;
            }

            if (assert_trap_trap) {
                trap_trap = 1;
            }
        }

        trap =
            trap_bpt || trap_iot || trap_emt || trap_trap ||
            trap_ill || trap_odd || trap_priv || trap_bus;

        if (trap) {
            printf("trap: asserts ");
            if (assert_trap_priv) printf("PRIV ");
            if (assert_trap_odd) printf("ODD ");
            if (assert_trap_ill) printf("ILL ");
            if (assert_bpt) printf("BPT ");
            if (assert_iot) printf("IOT ");
            if (assert_trap_emt) printf("EMT ");
            if (assert_trap_trap) printf("TRAP ");
            if (assert_trap_bus) printf("BUS ");
            printf("\n");

            printf("trap: %d\n", trap);
        }
    }

    /* */
    if (assert_halt) {
	printf("assert_halt\n");
	halted = 1;
    }

    if (assert_wait) {
	printf("assert_wait\n");
	waited = 1;
    }
}

int ipl_below(int psw_ipl, int ipl_bits)
{
    int mask;

    if (psw_ipl == 7)
        return 0;

    mask = ~((1 << psw_ipl) - 1);
    mask <<= 1;
    printf("ipl: mask 0x%x, bits 0x%x\n", mask, ipl_bits);

    if (ipl_bits & mask)
        return 1;

    return 0;
}

void check_for_interrupts(void)
{
    wire ok_to_assert_int;

    /* */
    ok_to_assert_int =
        istate == f1 || istate == i1 ||
        istate == s4 || istate == d4 | istate == w1;

    if (ok_to_assert_int) {
        if (assert_int & ipl_below(ipl, assert_int_ipl)) {
            interrupt = 1;
            interrupt_vector = assert_int_vec;
            printf("interrupt: asserts; vector %o\n", interrupt_vector);

            /* hack, but ok; */
            assert_int = 0;
            assert_int_vec =0;
            support_clear_int_bits();
        }
    } else {
        interrupt = 0;
    }
}

void
next_state()
{
    check_for_traps();
    check_for_interrupts();

    if (halted) {
	new_istate = h1;
        istate = new_istate;
	return;
    }

    if (waited) {
	new_istate = i1;
        istate = new_istate;
	return;
    }

    new_istate = 0;

    switch (istate) {
    case f1: new_istate = (trap || interrupt) ? t1 :
        		  c1;
                          break;

    case c1: new_istate = is_illegal ? f1 :
        	          no_operand ? e1 :
        		  need_s1 ? s1 :
        		  need_d1 ? d1 :
                          e1;
                          break;

    case s1: new_istate = need_s2 ? s2 :
			  need_s4 ? s4 :
			  d1;
			  break;

    case s2: new_istate = ss_ea_ind ? s3 :
        		  need_s4 ? s4 :
                          d1;
                          break;

    case s3: new_istate = s4; break;

    case s4: new_istate = d1; break;

    case d1: new_istate = need_d2 ? d2 : 
        		  need_d4 ? d4 :
        		  need_push_state ? p1 :
        		  e1;
                          break;

    case d2: new_istate = dd_ea_ind ? d3 :
        		  need_d4 ? d4 :
        		  need_push_state ? p1 :
        		  e1;
                          break;

    case d3: new_istate = need_d4 ? d4 :
        		  need_push_state ? p1 :
        		  e1;
                          break;

    case d4: new_istate = need_push_state ? p1 :
        		  e1;
                          break;

    case e1: new_istate = need_pop_reg ? o3 :
			  need_pop_pc_psw ? o1 :
        		  w1;
                          break;

    case w1: new_istate = f1; break;

    case o1: new_istate = o2; break;
    case o2: new_istate = f1; break;
    case o3: new_istate = w1; break;

    case p1: new_istate = e1; break;

    case t1: new_istate = t2; break;
    case t2: new_istate = t3; break;
    case t3: new_istate = t4; break;
    case t4: new_istate = f1; break;

    case i1: new_istate = interrupt ? f1 : i1; break;
    }


    /* */
    istate = new_istate;
}

void update_muxes(void)
{
    do_pc_mux();
    do_sp_mux();
    do_ea_mux();
    do_data_mux();
    do_cc_mux();
    do_reg_mux();
    do_psw_mux();

    /* do it again since I'm a poor sim writer */
    do_pc_mux();
    do_sp_mux();
}

void clock_registers(void)
{
    update_muxes();

    pc = pc_mux;
    sp = sp_mux;

    if (istate == o2 || istate == t4) {
        psw = psw_mux;
    }

    ss_ea = (istate == c1 ||
	     istate == s1 ||
	     istate == s2 ||
	     istate == s3 ||
	     istate == d1 ||
	     istate == d2 ||
	     istate == d3 ||
	     istate == t1 ||
	     istate == t3) ?
	ss_ea_mux : ss_ea;

    dd_ea = (istate == c1 ||
	     istate == s1 ||
	     istate == s2 ||
	     istate == s3 ||
	     istate == d1 ||
	     istate == d2 ||
	     istate == d3) ?
	dd_ea_mux : dd_ea;

    if (istate == c1 ||
	istate == s4 || istate == d4)
    {
	ss_data = ss_data_mux;
	dd_data = dd_data_mux;
    } else {
	ss_data = ss_data;
	dd_data = dd_data;
    }

    if (istate == e1) {
	e1_data = e1_data_mux;
    }

    if (istate == o3)
        e1_data = pop_mem_data;
    else
	e1_data = e1_data;

    next_state();
}


void step(int *did_trap)
{
    istate = f1;

    fetch();
    clock_registers();

    while (istate != f1) {
	switch (istate) {
	case c1: decode1(); break;
	case s1: source1(); break;
	case s2: source2(); break;
	case s3: source3(); break;
	case s4: source4(); break;
	case d1: dest1(); break;
	case d2: dest2(); break;
	case d3: dest3(); break;
	case d4: dest4(); break;
	case e1: execute(); break;
	case w1: writeback1(); break;
	case o1: pop1(); break;
	case o2: pop2(); break;
	case o3: pop3(); break;
	case p1: push1(); break;
	case t1: trap1(); break;
	case t2: trap2(); break;
	case t3: trap3(); break;
	case t4: trap4(); *did_trap = 1; break;
	case i1: wait1(); break;
	}
	clock_registers();

        if (istate == h1)
            break;
    }
}

void run(void)
{
    int did_trap = 0;
    int trap_sync = 0;

    istate = f1;

    while (1) {
        reset_transactions();

        step(&did_trap);

	if (istate == h1) {
	    printf("halted\n");
	    break;
	}

        /* skip 1 step on a trap to keep in sync */
        if (trap_sync) {
            trap_sync = 0;
            cosim_trap();
        } else {
            if (did_trap) {
                trap_sync = 1;
                did_trap = 0;
            } else {
                cosim_check();
                check_transactions();
            }
        }
    }
}


/* debug */
void debug_set_pc(u16 new_pc)
{
    pc = new_pc;
}

void debug_reset(void)
{
    int i;

    for (i = 0; i < 7; i++)
	regs[i] = -1;

    regs[6] = 0;
    regs[7] = 0;
    psw = 0340;
}

void
support_signals_bus_error(void)
{
    printf("assert_trap_bus = 1\n");
    assert_trap_bus = 1;
}



/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
