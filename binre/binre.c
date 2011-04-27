/*
 * binre.c
 *
 * pdp-11 binary recompilation
 * Brad Parker <brad@heeltoe.com>
 * 2/2011
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "binre.h"
#include "mach.h"
#include "isn.h"
#include "support.h"

extern u_short isn_dispatch[0x10000];
extern raw_isn_t *isn_decode[0x10000];
extern raw_isn_t raw_isns[];

u16 fetch[3];
u_char fetch_valid[3];

int halted;
int waiting;
int reset;
int debug;
unsigned int max_cycles;
unsigned int cycles;
int selftest;
char *image_filename;
char *mem_filename;
int use_rl02;
int use_rk05;
int initial_pc;

u_char cc_c, cc_n, cc_z, cc_v;

/* wires */
int trap_odd;
int trap_bus;
int trap_res;
int trap_ill;
int trap_iot;
int trap_emt;
int trap_priv;
int trap_bpt;
int trap_trap;
int trap_interrupt;
int trap_abort;
int trap_oflo;
int trap_trace;

int trace_inhibit;

int assert_wait;
int assert_halt;
int assert_reset;
int assert_bpt;
int assert_iot;
int assert_int;
int assert_trap_odd;
int assert_trap_ill;
int assert_trap_res;
int assert_trap_priv;
int assert_trap_emt;
int assert_trap_trap;
int assert_trap_bus;
int assert_trap_abort;
int assert_trap_oflo;

int assert_trace_inhibit;

u16 assert_int_vec;
u16 assert_int_ipl_bits;

u16 regs[8];
u16 memory[4 * 1024 * 1024];
u16 psw;

u_char r_none, r_8off, r_r, r_n, r_nn, r_ss, r_dd, r_rss, r_rdd, r_ssdd;
u_char r_illegal, r_reserved;

void mem_signals_bus_error(u32 addr)
{
    printf("assert_trap_bus = 1 (pc %o, addr %o)\n", pc, addr);
    assert_trap_bus = 1;
    m_pipe_flush();
}

void support_signals_bus_error(u32 addr)
{
    printf("assert_trap_bus = 1 (pc %o, addr %o)\n", pc, addr);
    assert_trap_bus = 1;
    m_pipe_flush();
}

void mmu_signals_abort(void)
{
    assert_trap_abort = 1;
    m_pipe_flush();
}

void mmu_signals_trap(void)
{
    assert_trap_abort = 1;
    m_pipe_flush();
}

void mach_signals_odd(void)
{
    assert_trap_odd = 1;
    m_pipe_flush();
}

void mach_signals_oflo(void)
{
    if (debug) printf("mach_signals_oflo\n");
    assert_trap_oflo = 1;
    m_pipe_flush();
}

void mach_signals_oflo_next(void)
{
    if (debug) printf("mach_signals_oflo_next\n");
    assert_trap_oflo = 1;
}

void mach_trace_inhibit(void)
{
    if (debug) printf("mach_trace_inhibit\n");
    assert_trace_inhibit = 1;
}

void
tb_pc_flush(void)
{
}

void
tb_pc_set(int new_pc)
{
    if (debug) printf("tb_set_pc: pc %o\n", new_pc);
    pc = new_pc;
}

static int ipl_below(int psw_ipl, int ipl_bits)
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

int
is_exception(void)
{
    if (pc & 1) {
        if (debug) printf("is_exception: odd pc\n");
        trap_odd = 1;
        return 1;
    }

    /* trace */
    if (assert_trace_inhibit) {
        if (debug) printf("is_exception: assert_trace_inhibit\n");
        trace_inhibit = 1;
        assert_trace_inhibit = 0;
    }

    if (psw & 020) {
        if (!trace_inhibit) {
            if (debug) printf("is_exception: trace\n");
            trap_trace = 1;
            return 1;
        }
    }

    /* interrupt */
    int ipl = (psw >> 5) & 7;

    if (assert_int && ipl_below(ipl, assert_int_ipl_bits)) {
        printf("assert_int = 1 (pc %o)\n", pc);

        trap_interrupt = 1;
        assert_int = 0;
        return 1;
    }

    if (assert_int && !ipl_below(ipl, assert_int_ipl_bits)) {
        printf("assert_int = 1 but !ipl (ipl mask %o assert_int_ipl %o)\n",
               (u_short)~((1 << ipl) - 1), assert_int_ipl_bits);
    }

    /* other exceptions */
    if (assert_trap_bus) { 
        if (debug) printf("is_exception: assert_trap_bus\n");
        trap_bus = 1;
        assert_trap_bus = 0;
        return 1;
    }

    if (assert_trap_ill) {
        if (debug) printf("is_exception: assert_trap_ill\n");
        trap_ill = 1;
        assert_trap_ill = 0;
        return 1;
    }

    if (assert_trap_res) { 
        if (debug) printf("is_exception: assert_trap_res\n");
        trap_res = 1;
        assert_trap_res = 0;
        return 1;
    }

    if (assert_trap_abort) {
        if (debug) printf("is_exception: assert_trap_abort\n");
        trap_abort = 1;
        assert_trap_abort = 0;
        return 1;
    }

    if (assert_trap_odd) {
        if (debug) printf("is_exception: assert_trap_odd\n");
        trap_odd = 1;
        assert_trap_odd = 0;
        return 1;
    }

    if (assert_trap_oflo) {
        if (debug) printf("is_exception: assert_trap_oflo\n");
        trap_oflo = 1;
        assert_trap_oflo = 0;
        return 1;
    }

    return 0;
}

void
tb_exception(void)
{
    int vector = 0;

    if (debug) {
        printf("tb_exception: sp %o\n", regs[6]);
        printf("tb_exception: odd %d, bus %d, res %d, oflo %d, ill %d, priv %d\n",
               trap_odd, trap_bus, trap_res, trap_oflo, trap_ill, trap_priv);
    }

    if (trap_odd || trap_bus || trap_res || trap_oflo) {
        vector = 004;
    }
    else
    if (trap_ill || trap_priv) {
        vector = 010;
    }
    else
    if (trap_bpt || trap_trace) {
        vector = 014;
    }
    else
    if (trap_iot) {
        vector = 020;
    }
    else
    if (trap_emt) {
        vector = 030;
    }
    else
    if (trap_trap) {
        vector = 034;
    }
    else
    if (trap_abort) {
        vector = 0250;
    }
    else
    if (trap_interrupt) {
        vector = assert_int_vec;
support_clear_int_bits();
    }

do_exception:
    printf("encoding exception; vector %o\n", vector);
    encode_exception(vector);

    trap_odd = 0;
    trap_bus = 0;
    trap_res = 0;
    trap_ill = 0;
    trap_priv = 0;
    trap_bpt = 0;
    trap_iot = 0;
    trap_emt = 0;
    trap_trap = 0;
    trap_interrupt = 0;
    trap_abort = 0;
    trap_oflo = 0;
    trap_trace = 0;

    trace_inhibit = 0;
}

void
tb_decode(void)
{
    u16 op;
    u8 oph, opl;
    u8 op_15_12;
    u16 op_15_9;
    u16 op_15_6;

    if (!fetch_valid[0])
        return;

    op = fetch[0];
    oph = op >> 8;
    opl = op & 0xff;

    r_none =
        (oph == 0 &&
         (opl == 00 || opl == 01 || opl == 02 ||
          opl == 03 || opl == 04 || opl == 05 ||
          opl == 06 || opl == 07 ||
          (opl & 0360) == 0240 ||
          (opl & 0360) == 0260)) ||
        (oph == 0104000>>8) ||
        (oph == 0104400>>8);

    r_r =
        oph == 0 &&
        (opl >= 0200 && opl <= 0207);

    r_n =
        oph == 0 &&
        (opl >= 0230 && opl <= 0237);

    r_nn =
        op >= 06400 && op <= 06477;

    op_15_6 = (op & 0177700) >> 6;

    r_8off =
        (op_15_6 >= 00004 && op_15_6 <= 00037) ||
        (op_15_6 >= 00770 && op_15_6 <= 00777) ||
        (op_15_6 >= 01000 && op_15_6 <= 01037);

    r_ss =
        op_15_6 == 00065 ||
        op_15_6 == 01064 ||
        op_15_6 == 01065;

    r_dd =
        op_15_6 == 00001 ||
        op_15_6 == 00003 ||
        (op_15_6 >= 00050 && op_15_6 <= 00063) ||
        (op_15_6 >= 00066 && op_15_6 <= 00070) ||
        (op_15_6 >= 01050 && op_15_6 <= 01063) ||
        (op_15_6 >= 01066 && op_15_6 <= 01067);

    op_15_9 = (op & 0177000) >> 9;

    r_rss =
        (op_15_9 >= 0070 && op_15_9 <= 0073);

    r_rdd =
        op_15_9 == 0004 ||
        op_15_9 == 0074;

    op_15_12 = (op & 0170000) >> 12;

    r_ssdd =
        (op_15_12 >= 001 && op_15_12 <= 006) ||
        (op_15_12 >= 011 && op_15_12 <= 016);

    /* illegal opcodes */
    int op_5_0 = op & 077;
    int op_11_9 = (op & 0007000) >> 9;

    r_illegal =
	(op_15_6 == 0000 && op_5_0 > 007) ||
	(op_15_6 == 0000 && op_5_0 == 007) ||		// trapa/mfpt
	(op_15_6 == 0002 && (op_5_0 >= 010 && op_5_0 <= 027)) ||
	(op_15_12 == 007 && (op_11_9 == 5 || op_11_9 == 6)) ||
	(op_15_12 == 017) ||
   	(op_15_6 == 00047 && op_5_0 <= 007) ||		// jsp rx
        (op_15_6 >= 0071 && op_15_6 <= 0077);

    r_reserved =
	(op_15_6 == 0001 && op_5_0 <= 007) ||		/* jmp rx */
   	(op_15_9 == 0004 && op_5_0 <= 007);		// jsp rx

    if (r_illegal)
        assert_trap_ill++;
    else
    if (r_reserved)
        assert_trap_res++;

    if (debug) {
        printf("decode: ");
        if (r_none) printf("r_none\n");
        if (r_r) printf("r_r\n");
        if (r_n) printf("r_n\n");
        if (r_nn) printf("r_nn\n");
        if (r_8off) printf("r_8off\n");
        if (r_ss) printf("r_ss\n");
        if (r_dd) printf("r_dd\n");
        if (r_rss) printf("r_rss\n");
        if (r_rdd) printf("r_rdd\n");
        if (r_ssdd) printf("r_ssdd\n");
        if (r_illegal) printf("r_illegal\n");
    }

    if (!r_none &&
        !r_r &&
        !r_n &&
        !r_nn &&
        !r_8off &&
        !r_ss &&
        !r_dd &&
        !r_rss &&
        !r_rdd &&
        !r_ssdd &&
        !r_illegal)
    {
        printf("decode: failed; op=%o oph=%o\n", op, oph);
    }
}

void
tb_recompile(void)
{
    u16 op;
    u8 oph, opl;
    u8 op_15_12;
    u16 op_15_9;
    u16 op_15_6;
    int fetch_pc;

    if (!fetch_valid[0])
        return;

    op = fetch[0];
    oph = op >> 8;
    opl = op & 0xff;

    fetch_pc = pc;

    if (r_none) {

        if (debug) printf("r_none %o\n", op);

        switch (op) {
        case 00: /*halt*/ encode0(M_HALT); break;
        case 01: /*wait*/ encode0(M_WAIT); break;
        case 02: /*rti*/  encode_rti(); break;
        case 03: /*bpt*/  encode_trap(014); break;
        case 04: /*iot*/  encode_trace_inhibit(); encode_trap(020); break;
        case 05: /*reset*/encode0(M_RESET); break;
        case 06: /*rtt*/  encode_rtt(); break;
        case 07: /*mfpt*/ encode_mfpt(); break;
        case 0240: /*c*/
        case 0241: /*clc*/
        case 0242: /*clv*/
        case 0243:
        case 0244: /*clz*/
        case 0245:
        case 0246:
        case 0247:
        case 0250: /*cln*/
        case 0251:
        case 0252:
        case 0253:
        case 0254:
        case 0255:
        case 0256:
        case 0257: /*ccc*/
            encode_new_cc(op & 017, 0);
            break;
        case 0260: /*s*/
        case 0261: /*sec*/
        case 0262: /*sev*/
        case 0263:
        case 0264: /*sez*/
        case 0265:
        case 0266:
        case 0267:
        case 0270: /*sen*/
        case 0271:
        case 0272:
        case 0273:
        case 0274:
        case 0275:
        case 0276:
        case 0277:
            encode_new_cc(op & 017, 1);
            break;

        default:
            if (op >= 0104000 && op <= 0104377) { /*emt*/
                encode_trap(030, op & 077);
                break;
            }
            else
            if (op >= 0104400 && op <= 0104777) { /*trap*/
                encode_trap(034, op & 077);
                break;
            }
            else {
                printf("r_none? %o\n", op);
            }
            break;
        }
    }

    if (r_nn) {
        if (debug) printf("r_nn %o\n", op);

        if (op >= 0006400 && op <= 0006477) { /*mark*/
            encode_mark(op & 077);
        }
    }

    if (r_r) {
        if (debug) printf("r_r %o\n", op);

        switch (op) {
        case 0200: /*rts*/
        case 0201:
        case 0202:
        case 0203:
        case 0204:
        case 0205:
        case 0206:
        case 0207:
            encode_rts(op & 07);
            break;

        default:
            printf("r_r? %o\n", op);
        }
    }

    if (r_n) {
        if (debug) printf("r_n %o\n", op);

        switch (op) {
        case 0230: /*spl*/
        case 0231:
        case 0232:
        case 0233:
        case 0234:
        case 0235:
        case 0236:
        case 0237:
            encode_spl(op & 07);
            break;

        default:
            printf("r_n? %o\n", op);
        }
    }

    if (r_8off) {
        op_15_6 = (op & 0177700) >> 6;

        if (debug) printf("r_8off %o %o\n", op, op_15_6);

        switch (op_15_6) {
        case 00004: /*br*/
        case 00005:
        case 00006:
        case 00007:
            encode_branch(B_ALWAYS, op & 0377);
            break;
        case 00010: /*bne*/
        case 00011:
        case 00012:
        case 00013:
            encode_branch(B_NE, op & 0377);
            break;
        case 00014: /*beq*/
        case 00015:
        case 00016:
        case 00017:
            encode_branch(B_EQ, op & 0377);
            break;
        case 00020: /*bge*/
        case 00021:
        case 00022:
        case 00023:
            encode_branch(B_GE, op & 0377);
            break;
        case 00024: /*blt*/
        case 00025:
        case 00026:
        case 00027:
            encode_branch(B_LT, op & 0377);
            break;
        case 00030: /*bgt*/
        case 00031:
        case 00032:
        case 00033:
            encode_branch(B_GT, op & 0377);
            break;
        case 00034: /*ble*/
        case 00035:
        case 00036:
        case 00037:
            encode_branch(B_LE, op & 0377);
            break;

        case 01000: /*bpl*/
        case 01001:
        case 01002:
        case 01003:
            encode_branch(B_PL, op & 0377);
            break;

        case 01004: /*bmi*/
        case 01005:
        case 01006:
        case 01007:
            encode_branch(B_MI, op & 0377);
            break;

        case 01010: /*bhi*/
        case 01011:
        case 01012:
        case 01013:
            encode_branch(B_HI, op & 0377);
            break;

        case 01014: /*blo*/
        case 01015:
        case 01016:
        case 01017:
            encode_branch(B_LO, op & 0377);
            break;

        case 01020: /*bvc*/
        case 01021:
        case 01022:
        case 01023:
            encode_branch(B_VC, op & 0377);
            break;

        case 01024: /*bvs*/
        case 01025:
        case 01026:
        case 01027:
            encode_branch(B_VS, op & 0377);
            break;

        case 01030: /*bcc*/
        case 01031:
        case 01032:
        case 01033:
            encode_branch(B_CC, op & 0377);
            break;

        case 01034: /*bcs*/
        case 01035:
        case 01036:
        case 01037:
            encode_branch(B_CS, op & 0377);
            break;

        case 00770: /*sob*/
        case 00771:
        case 00772:
        case 00773:
        case 00774:
        case 00775:
        case 00776:
        case 00777:
            encode_sob((op & 0700) >> 6, op & 077);
            break;

        default:
            printf("r8off? %o\n", op);
            break;
        }
    }

    if (r_ss) {
        op_15_6 = (op & 0177700) >> 6;

        int smode, sreg;
        int offset;
        u16 src;

        smode = (op >> 3) & 7;
        sreg = (op >> 0) & 7;
        offset = 1;
        src = 0;

        /* ss - only worry about pc here */
        if (sreg == 7) {
            src = fetch[offset];
            if (smode == 2 || smode == 3) {
                fetch_pc += 2;
                offset++;
            }
            if (smode == 4 || smode == 5) {
                fetch_pc -= 2;
                offset++;
            }
        }

        if (smode == 6 || smode == 7) {
            src = fetch[offset];
            fetch_pc += 2;
            offset++;
        }

        switch (op_15_6) {
        case 00065: /*mfpi*/
            encode_mfpi(smode, sreg, src);
            break;
        case 01064: /*mtps*/
            encode_mtps(smode, sreg, src);
            break;
        case 01065: /*mfpd*/
            encode_mfpd(smode, sreg, src);
            break;
        default:
            printf("r_ss? %o\n", op);
            break;
        }
    }

    if (r_dd) {
        op_15_6 = (op & 0177700) >> 6;

        int dmode, dreg;
        int offset;
        u16 dst;

        dmode = (op >> 3) & 7;
        dreg = (op >> 0) & 7;
        offset = 1;
        dst = 0;

        if (debug) printf("r_dd %o %o\n", op, op_15_6);

        /* dd - only worry about pc here */
        if (dreg == 7) {
            dst = fetch[offset];
            if (dmode == 2 || dmode == 3) {
                fetch_pc += 2;
                offset++;
            }
            if (dmode == 4 || dmode == 5) {
                fetch_pc -= 2;
                offset++;
            }
        }

        if (dmode == 6 || dmode == 7) {
            dst = fetch[offset];
            fetch_pc += 2;
            offset++;
        }

        switch (op_15_6) {
        case 00001: /*jmp*/
            encode_jmp(dmode, dreg, dst);
            break;
        case 00003: /*swab*/
            encode_swab(dmode, dreg, dst);
            break;
        case 00050: /*clr*/
            encode_clr(dmode, dreg, dst);
            break;
        case 00051: /*com*/
            encode_com(dmode, dreg, dst);
            break;
        case 00052: /*inc*/
            encode_inc(dmode, dreg, dst);
            break;
        case 00053: /*dec*/
            encode_dec(dmode, dreg, dst);
            break;
        case 00054: /*neg*/
            encode_neg(dmode, dreg, dst);
            break;
        case 00055: /*adc*/
            encode_adc(dmode, dreg, dst);
            break;
        case 00056: /*sbc*/
            encode_sbc(dmode, dreg, dst);
            break;
        case 00057: /*tst*/
            encode_tst(dmode, dreg, dst);
            break;
        case 00060: /*ror*/
            encode_ror(dmode, dreg, dst);
            break;
        case 00061: /*rol*/
            encode_rol(dmode, dreg, dst);
            break;
        case 00062: /*asr*/
            encode_asr(dmode, dreg, dst);
            break;
        case 00063: /*asl*/
            encode_asl(dmode, dreg, dst);
            break;

        case 00066: /*mtpi*/
            encode_mtpi(dmode, dreg, dst);
            break;
        case 00067: /*sxt*/
            encode_sxt(dmode, dreg, dst);
            break;
        case 00070: /*csm*/
            encode_csm(dmode, dreg, dst);
            break;

        case 01050: /*clrb*/
            encode_clrb(dmode, dreg, dst);
            break;
        case 01051: /*comb*/
            encode_comb(dmode, dreg, dst);
            break;
        case 01052: /*incb*/
            encode_incb(dmode, dreg, dst);
            break;
        case 01053: /*decb*/
            encode_decb(dmode, dreg, dst);
            break;
        case 01054: /*negb*/
            encode_negb(dmode, dreg, dst);
            break;
        case 01055: /*adcb*/
            encode_adcb(dmode, dreg, dst);
            break;
        case 01056: /*sbcb*/
            encode_sbcb(dmode, dreg, dst);
            break;
        case 01057: /*tstb*/
            encode_tstb(dmode, dreg, dst);
            break;
        case 01060: /*rorb*/
            encode_rorb(dmode, dreg, dst);
            break;
        case 01061: /*rolb*/
            encode_rolb(dmode, dreg, dst);
            break;
        case 01062: /*asrb*/
            encode_arsb(dmode, dreg, dst);
            break;
        case 01063: /*aslb*/
            encode_aslb(dmode, dreg, dst);
            break;

        case 01066: /*mtpd*/
            encode_mtpd(dmode, dreg, dst);
            break;
        case 01067: /*mfps*/
            encode_mfps(dmode, dreg, dst);
            break;
        default:
            printf("r_dd? %o\n", op);
            break;
        }
    }

    if (r_rss) {
        op_15_9 = (op & 0177000) >> 9;

        int smode, sreg, reg;
        int offset;
        u16 src;

        if (debug) printf("r_rss %o %o\n", op, op_15_9);

        smode = (op >> 3) & 7;
        sreg = (op >> 0) & 7;
        reg = (op >> 6) & 7;
        offset = 1;

        /* dd - only worry about pc here */
        if (sreg == 7) {
            src = fetch[offset];
            if (smode == 2 || smode == 3) {
                fetch_pc += 2;
                offset++;
            }
            if (smode == 4 || smode == 5) {
                fetch_pc -= 2;
                offset++;
            }
        }

        if (smode == 6 || smode == 7) {
            src = fetch[offset];
            fetch_pc += 2;
            offset++;
        }

        switch (op_15_9) {
        case 0070: /*mul*/
            encode_mul(reg, smode, sreg, src);
            break;
        case 0071: /*div*/
            encode_div(reg, smode, sreg, src);
            break;
        case 0072: /*ash*/
            encode_ash(reg, smode, sreg, src);
            break;
        case 0073: /*ashc*/
            encode_ashc(reg, smode, sreg, src);
            break;
        default:
            printf("r_rss? %o\n", op);
            break;
        }
    }

    if (r_rdd) {
        op_15_9 = (op & 0177000) >> 9;

        int dmode, dreg, reg;
        int offset;
        u16 dst;

        if (debug) printf("r_rdd %o %o\n", op, op_15_9);

        dmode = (op >> 3) & 7;
        dreg = (op >> 0) & 7;
        reg = (op >> 6) & 7;
        offset = 1;
        dst = 0;

        /* dd - only worry about pc here */
        if (dreg == 7) {
            dst = fetch[offset];
            if (dmode == 2 || dmode == 3) {
                fetch_pc += 2;
                offset++;
            }
            if (dmode == 4 || dmode == 5) {
                fetch_pc -= 2;
                offset++;
            }
        }

        if (dmode == 6 || dmode == 7) {
            dst = fetch[offset];
            fetch_pc += 2;
            offset++;
        }

        switch (op_15_9) {
        case 004: /*jsr*/
            encode_jsr(reg, dmode, dreg, dst);
            break;
        case 074: /*xor*/
            encode_xor(reg, dmode, dreg, dst);
            break;
        default:
            printf("r_rdd? %o\n", op);
            break;
        }
    }

    if (r_ssdd) {
        oph = op >> 8;
        opl = op & 0xff;
        op_15_12 = (op & 0170000) >> 12;

        if (debug) printf("r_ssdd %o %o\n", op, op_15_12);

        int smode, dmode;
        int sreg, dreg;
        u16 src, dst;
        int offset;

        smode = (op >> 9) & 7;
        sreg = (op >> 6) & 7;
        dmode = (op >> 3) & 7;
        dreg = (op >> 0) & 7;

        src = 0;
        dst = 0;
        offset = 1;

        /* ss - only worry about pc here */
        if (sreg == 7) {
            src = fetch[offset];
            if (smode == 2 || smode == 3) {
                fetch_pc += 2;
                offset++;
            }
            if (smode == 4 || smode == 5) {
                fetch_pc -= 2;
                offset++;
            }
        }

        if (smode == 6 || smode == 7) {
            src = fetch[offset];
            fetch_pc += 2;
            offset++;
        }

        /* dd - only worry about pc here */
        if (dreg == 7) {
            dst = fetch[offset];
            if (dmode == 2 || dmode == 3) {
                fetch_pc += 2;
                offset++;
            }
            if (dmode == 4 || dmode == 5) {
                fetch_pc -= 2;
                offset++;
            }
        }

        if (dmode == 6 || dmode == 7) {
            dst = fetch[offset];
            fetch_pc += 2;
            offset++;
        }

        switch (op_15_12) {
        case 001: /*mov*/
            encode_mov(smode, sreg, dmode, dreg, src, dst);
            break;
        case 002: /*cmp*/
            encode_cmp(smode, sreg, dmode, dreg, src, dst);
            break;
        case 003: /*bit*/
            encode_bit(smode, sreg, dmode, dreg, src, dst);
            break;
        case 004: /*bic*/
            encode_bic(smode, sreg, dmode, dreg, src, dst);
            break;
        case 005: /*bis*/
            encode_bis(smode, sreg, dmode, dreg, src, dst);
            break;
        case 006: /*add*/
            encode_add(smode, sreg, dmode, dreg, src, dst);
            break;

        case 011: /*movb*/
            encode_movb(smode, sreg, dmode, dreg, src, dst);
            break;
        case 012: /*cmpb*/
            encode_cmpb(smode, sreg, dmode, dreg, src, dst);
            break;
        case 013: /*bitb*/
            encode_bitb(smode, sreg, dmode, dreg, src, dst);
            break;
        case 014: /*bicb*/
            encode_bicb(smode, sreg, dmode, dreg, src, dst);
            break;
        case 015: /*bisb*/
            encode_bisb(smode, sreg, dmode, dreg, src, dst);
            break;
        case 016: /*sub*/
            encode_sub(smode, sreg, dmode, dreg, src, dst);
            break;
        default:
            printf("r_ssdd? %o\n", op);
            break;
        }
    }
}

int mmu_map(int mode, int fetch, int write, int trap, int odd,
            int vaddr, int *ppaddr);

/* convert 16 bit addr to 22 bit address */
u32 se_addr(u32 addr)
{
    if (addr >= 0xe000) return addr | 0x3f0000;
    return addr;
}

int
cpu_read(int mode, int fetch, int addr, u16 *pval)
{
    int addr2;

    addr = se_addr(addr);

    if (mmu_map(mode, fetch, 0, 0, 0, addr, &addr2))
        return -1;

    if (addr2 >= IOPAGEBASE) {
        if (io_read(addr2, pval))
            return -1;

        return 0;
    }

    /* 256k */
    if (addr2 >= 01000000) {
        mem_signals_bus_error(addr);
        return -1;
    }

    *pval = memory[addr2/2];

    if (debug) {
        printf("mem: read %o -> %o (%c)\n",
               addr2, *pval,
               mode == 0 ? 'k' : mode == 1 ? 's' : mode == 3 ? 'u' : '?');
    }

    return 0;
}

int
cpu_write_byte(int mode, int addr, u8 val)
{
    int addr2;
    u16 old, new;

    addr = se_addr(addr);

    if (mmu_map(mode, 0, 1, 0, 0, addr, &addr2))
        return -1;

    if (addr2 >= IOPAGEBASE) {
        return io_write(addr2, val, 1);
    }

    old = memory[addr2/2];

    if (addr & 1) {
        new = (old & 0x00ff) | ((val & 0xff) << 8);
    } else {
        new = (old & 0xff00) | (val & 0xff);
    }

    if (debug) {
        printf("mem: writeb %o <- %o (loc %o=%o)\n", addr, val, (addr2/2)*2, new);
    }

    /* 256k */
    if (addr2 >= 01000000) {
        mem_signals_bus_error(addr);
        return -1;
    }

    memory[addr2/2] = new;
    return 0;
}

int
cpu_write(int mode, int addr, u16 val)
{
    int addr2;

    addr = se_addr(addr);

    if (mmu_map(mode, 0, 1, 0, 0, addr, &addr2))
        return -1;

    if (addr2 >= IOPAGEBASE) {
        return io_write(addr2, val, 0);
    }

    if (debug) {
        printf("mem: write %o <- %o (%c)\n",
               addr2, val,
               mode == 0 ? 'k' : mode == 1 ? 's' : mode == 3 ? 'u' : '?');
    }

    /* 256k */
    if (addr2 >= 01000000) {
        mem_signals_bus_error(addr);
        return -1;
    }

    memory[addr2/2] = val;
    return 0;
}

u16 raw_read_memory(u32 addr)
{
    return memory[addr/2];
}

void raw_write_memory(u32 addr, u16 data)
{
    memory[addr/2] = data;
}


void
isn_fetch(void)
{
    int mode = m_current_mode();

    fetch_valid[0] = fetch_valid[1] = fetch_valid[2] = 0;
    
    if (cpu_read(mode, 1, pc+0, &fetch[0]))
        return;
    fetch_valid[0] = 1;

    if (cpu_read(mode, 0, pc+2, &fetch[1]))
        return;
    fetch_valid[1] = 1;

    if (cpu_read(mode, 0, pc+4, &fetch[2]))
        return;
    fetch_valid[2] = 1;

    if (debug) {
        char txt[128];
        printf("read %o %07o %07o %07o\n", pc, fetch[0], fetch[1], fetch[2]);
        pdp11_dis(fetch[0], fetch[1], fetch[2], txt);
        printf("fetch pc %o %s\n", pc, txt);
    }
    pc += 2;
}

void
tb_show(void)
{
    m_fifo_dump();
}

void
tb_dump(void)
{
    if (debug) {
        printf("\n");
        m_state_dump();
    }
}

void
tb_execute(void)
{
    m_fifo_execute();

    if (psw & 020) {
        if (debug) printf("tb_execute: reset trace_inhibit\n");
        trace_inhibit = 0;
    }
}

void
run(void)
{
    cycles = 0;

    while (!halted) {
        m_fifo_reset();

        tb_dump();

	if (is_exception()) {
            tb_exception();
        } else {
            isn_fetch();
            tb_decode();

            if (is_exception())
                tb_exception();
            else
                tb_recompile();
        }

        if (0) tb_show();
        tb_execute();
        tb_show();

        cycles++;
        if (cycles >= max_cycles) {
            printf("max cycles (%d) exceeded\n", max_cycles);
            break;
        }

        poll_support();

        if (reset) {
            reset = 0;
            mmu_reset();
            support_clear_int_bits();
            reset_support();
        }

        if (waiting) {
            printf("waiting...\n");

            while (!assert_int) {
                poll_support();
            }

            printf("done waiting!");
            waiting = 0;
        }
    }

    if (halted) {
        printf("halted, pc %o\n", pc);
    }
}

int load_memfile(char *mem_filename)
{
    FILE *f;
    char line[1024];

    printf("loading memory from %s\n", mem_filename);

    f = fopen(mem_filename, "r");
    if (f) {
        while (fgets(line, sizeof(line), f)) {
            int n, maddr, mval;
            n = sscanf(line, "%o %o", &maddr, &mval);
            if (n == 2) {
                memory[maddr/2] = mval;
            }
        }
        fclose(f);
    }

    memory[042/2] = 040000;

    return 0;
}

void
init(void)
{
    make_isn_table();

    if (selftest) {
        fill_test_code();
    } else
        if (mem_filename) {
            load_memfile(mem_filename);
        } else
            if (image_filename) {
                if (use_rk05)
                    io_rk_bootrom();
                if (use_rl02)
                    io_rl_bootrom();
            }

    reset_support();

    psw = 0340;
    pc = initial_pc;
}

extern int optind;
extern char *optarg;

main(int argc, char *argv[])
{
    int c;

    debug = 1;
    max_cycles = 5;
    image_filename = NULL;
    mem_filename = NULL;
    use_rk05 = 1;
    use_rl02 = 0;

    while ((c = getopt(argc, argv, "c:dm:f:m:p:r:")) != -1) {
        switch (c) {
        case 'd':
            debug++;
            break;
        case 'c':
            max_cycles = atoi(optarg);
            break;
        case 'm':
            mem_filename = strdup(optarg);
            break;
        case 's':
            selftest++;
            break;
        case 'f':
            image_filename = strdup(optarg);
            break;
        case 'p':
            sscanf(optarg, "%o", &initial_pc);
            break;
        case 'r':
            if (optarg && optarg[0] == 'k') {
                use_rl02 = 0;
                use_rk05 = 1;
            }
            if (optarg && optarg[0] == 'l') {
                use_rl02 = 1;
                use_rk05 = 0;
            }
            break;
	}
    }

    init();
    run();
    exit(0);
}


/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
