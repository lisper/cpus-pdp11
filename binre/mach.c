/*
 * mach.c
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "binre.h"
#include "mach.h"

int m_fifo_depth;
m_fifo_t m_fifo[32];
m_fifo_t *m_current;

extern int halted;
extern int debug;

u16 regs[32];
u16 p_regs[32];

#define R_SP(mode)	(16 + mode)

#define current_mode	((psw >> 14) & 3)
#define previous_mode	((psw >> 12) & 3)

#define mode_kernel 0
#define mode_super  1
#define mode_undef  2
#define mode_user   3

#define CC_N 010
#define CC_Z 004
#define CC_V 002
#define CC_C 001

int div_overflow, div_result_sign, div_result;
int mul_overflow, mul_result_sign, mul_result;

int shift_sign;
int shift_sign_change16;
int shift_sign_change32;
int shift_out;

void div32by16(u16 *pr0, u16 *pr1, u16 r0, u16 r1, u16 r2)
{
    u32 v = (r0 << 16) | r1;

    *pr0 = v / r2;
    *pr1 = v % r2;

    div_overflow = 0;
    div_result = v / r2;
    div_result_sign = ((int)v / (int)r2) < 0 ? 1 : 0;;
}

void mul16by16(u16 *pr0, u16 *pr1, u16 r0, u16 r2)
{
    u32 v = r0 * r2;

    *pr0 = v >> 16;
    *pr1 = v;

    mul_overflow = 0;
    mul_result = v;
    mul_result_sign = (int)v < 0 ? 1 : 0;;
}

void xor16(u16 *pr0, u16 r0, u16 r2)
{
    *pr0 = r0 ^ r2;
}

int m_current_mode(void)
{
    return current_mode;
}


/*
 *
 *   opcode   dr   sr   constant
 *  3......2 2..2 1..1 1..............0
 *  1      4 3  0 9  6 5
 *
 */

void m_state_dump(void)
{
    u16 r6 = regs[R_SP(current_mode)];

    printf("f1: pc=%o, sp=%o, psw=%o ipl%o n%d z%d v%d c%d "
           "(%o %o %o %o %o %o %o %o)\n",
           pc, r6, psw,
           (psw>>5)&7,
           psw&8?1:0, psw&4?1:0, psw&2?1:0, psw&1?1:0,
           regs[0], regs[1], regs[2], regs[3],
           regs[4], regs[5], r6, regs[7]);
    printf("r0 %07o r1 %07o r2 %07o r3 %07o   r6-00 %07o\n",
           regs[0], regs[1], regs[2], regs[3], regs[R_SP(0)]);
    printf("r4 %07o r5 %07o r6 %07o r7 %07o   r6-01 %07o\n",
           regs[4], regs[5], regs[6], regs[7], regs[R_SP(1)]);
    printf("S0 %07o S1 %07o S2 %07o              r6-10 %07o\n",
           regs[8], regs[9], regs[10], regs[R_SP(2)]);
    printf("D0 %07o D1 %07o D2 %07o              r6-11 %07o\n",
           regs[11], regs[12], regs[13], regs[R_SP(3)]);
    printf("R0 %07o R1 %07o\n",
           regs[14], regs[15]);
}

void m_dis_op(m_fifo_t *m, char *str)
{
    u32 op = m->op;
    int d =  m->d;
    int s1 = m->s1;
    int s2 = m->s2;
    int r2 = (m->s2 != 0xff) ? 1 : 0;
    int v =  m->v;

    str[0] = 0;
    switch (op) {
    case M_NOP:       sprintf(str, "nop"); break;
    case M_HALT:      sprintf(str, "halt"); break;
    case M_WAIT:      sprintf(str, "wait"); break;
    case M_RESET:     sprintf(str, "reset"); break;

    case M_LOAD:      sprintf(str, "load    %d,%d", d, s1); break;
    case M_LOADB:     sprintf(str, "loadb   %d,%d", d, s1); break;
    case M_LOADI:     sprintf(str, "load    %d,#0%o", d, v); break;
    case M_LOADIB:    sprintf(str, "loadb   %d,#0%o", d, v); break;
    case M_LOADIND:   sprintf(str, "load    %d,@%d", d, s1); break;
    case M_LOADINDPM: sprintf(str, "load    %d,pm-@%d", d, s1); break;
    case M_LOADINDB:  sprintf(str, "loadb   %d,@%d", d, s1); break;
    case M_LOADPSW:   sprintf(str, "load    psw,%d,%d", s1, v); break;

    case M_STOREB:    sprintf(str, "storeb  %d,%d", d, s1); break;
    case M_STOREIND:  sprintf(str, "store   @%d,%d", d, s1); break;
    case M_STOREINDPM:sprintf(str, "store   pm-@%d,%d", d, s1); break;
    case M_STOREINDB: sprintf(str, "storeb  @%d,%d", d, s1); break;
    case M_STOREPSW:  sprintf(str, "store   %d,psw", d); break;
    case M_STORESP:   sprintf(str, "storesp r6-%d", v, s1); break;

    case M_ADD:       sprintf(str, "add     %d,%d,%d", d, s1, s2); break;
    case M_ADD1:      sprintf(str, "add     %d,%d", d, s1); break;

    case M_ADDI:      sprintf(str, "addi    %d,#0%o", d, v); break;
    case M_ADDIB:     sprintf(str, "addib   %d,#0%o", d, v); break;

    case M_ADDC:      sprintf(str, "addc    %d", d); break;
    case M_ADDCB:     sprintf(str, "addcb   %d", d); break;
        
    case M_SUB:       sprintf(str, "sub     %d,%d,%d", d, s1, s2); break;

    case M_SUBB:      sprintf(str, "subb    %d,%d,%d", d, s1, s2); break;

    case M_SUBI:      sprintf(str, "subi    %d,#0%o", d, v); break;
    case M_SUBIB:     sprintf(str, "subib   %d,#0%o", d, v); break;
    case M_SUBC:      sprintf(str, "subc    %d", d); break;

    case M_AND:       sprintf(str, "and     %d,%d,%d", d, s1, s2); break;

    case M_ANDB:      sprintf(str, "andb    %d,%d,%d", d, s1, s2); break;

    case M_OR:        sprintf(str, "or      %d,%d,%d", d, s1, s2); break;

    case M_ORB:       sprintf(str, "orb     %d,%d,%d", d, s1, s2); break;

    case M_NOT:       sprintf(str, "not     %d,%d", d, s1); break;
    case M_NOTB:      sprintf(str, "notb    %d,%d", d, s1); break;

    case M_SXT:       sprintf(str, "sxt     %d,%d", d, s1); break;

    case M_DIV:       sprintf(str, "div     %d,%d", d, s1); break;
    case M_MUL:       sprintf(str, "mul     %d,%d", d, s1); break;

    case M_XOR:       sprintf(str, "xor     %d,%d", d, s1); break;

    case M_FLAGS:     sprintf(str, "flags   %d,%d,#0%o", d, s1, v); break;
    case M_FLAGMUX:   sprintf(str, "flgmux  %d,%d,#0%o", d, s1, v); break;
    case M_CHECKSP:   sprintf(str, "checksp %d", v); break;
    case M_INHIBIT:   sprintf(str, "inhibit %d", v); break;

    case M_BR:        sprintf(str, "br      %d,#0%o", d, v); break;
    case M_JMP:       sprintf(str, "jmp     %d", d); break;
    case M_SWAB:      sprintf(str, "swab    %d,%d", d, s1); break;

    case M_SHIFT:     sprintf(str, "shift   %d,%d,%d", d, s1, s2); break;
    case M_SHIFTI:    sprintf(str, "shift   %d,%d,#%d", d, s1, v); break;

    case M_SHIFT32:   sprintf(str, "shift   %d,%d,%d", d, s1, s2); break;

    case M_ROTATE:    sprintf(str, "rot     %d,%d,#%d", d, s1, v); break;
    case M_ROTATEB:   sprintf(str, "rotb    %d,%d,#%d", d, s1, v); break;
    case M_ASR:       sprintf(str, "ashift  %d,%d,#%d", d, s1, v); break;

    default:
        strcat(str, "???"); break;
    }
}

static int new_r6;

void m_psw_changed(void)
{
    new_r6 = 0;
}

void m_post_reg(m_fifo_t *m, int reg, int val)
{
    p_regs[reg] = val;

#if 0
    if (reg > 5) {
        regs[reg] = val;

        if (reg == 6)
            regs[R_SP(current_mode)] = val;
        return;
    }
#else
    regs[reg] = val;

    if (reg == 6)
        regs[R_SP(current_mode)] = val;
#endif

    if (debug > 1) printf("post r%d <- %06o\n", reg, val);

    if (m->r_valid && m->r_valid2) {
        printf("r_valid overflow\n");
        exit(3);
    }

    if (m->r_valid) {
        m->r_valid2 = 1;
        m->r_reg2 = reg;
        m->r_val2 = val;
    } else {
        m->r_valid = 1;
        m->r_reg = reg;
        m->r_val = val;
    }
}

void m_execute_isn(m_fifo_t *m)
{
    u32 op = m->op;
    int d = m->d;
    int s = m->s1;
    int s2 = m->s2;
    int r2 = (m->s2 != 0xff) ? 1 : 0;
    int v = m->v;
    int vs = (short)m->v;
    int offset, take_jump;
    int new_cc_n, new_cc_z, new_cc_v, new_cc_c;
    int cc_c, cc_n, cc_v, cc_z;
    int count;
    u16 r0, r1;

    /* in hardware this would be a mux for reads */
    regs[6] = regs[R_SP(current_mode)];
    new_r6 = 1;

    switch (op) {
    case M_NOP:
        break;
    case M_HALT:
        halted = 1;
        break;
    case M_WAIT:
        break;
    case M_RESET:
        break;

    case M_LOAD:
        // p_regs[d] = regs[s];
        m_post_reg(m, d, regs[s]);
        if (debug) printf("r%d <- r%d (0%o)\n", d, s, regs[s]);
        break;

    case M_LOADB:
        // regs[d] = (regs[d] & 0xff00) | (regs[s] & 0xff);
        m_post_reg(m, d, (regs[d] & 0xff00) | (regs[s] & 0xff));
        if (debug) printf("r%d <- r%d (byte 0%o)\n", d, s, regs[s] & 0xff);
        break;

    case M_LOADI:
        // regs[d] = v;
        m_post_reg(m, d, v);
        break;

    case M_LOADIB:
        // regs[d] = (regs[d] & 0xff00) | (v & 0xff);
        m_post_reg(m, d, (regs[d] & 0xff00) | (v & 0xff));
        break;

    case M_LOADIND:
        if (regs[s] & 1) {
            mach_signals_odd();
            break;
        }

        if (cpu_read(current_mode, 0, regs[s], &v) == 0) {
            // regs[d] = v;
            m_post_reg(m, d, v);
            if (debug) printf("r%d <- (@%o) 0%o\n", d, regs[s], v);
        } else {
            if (debug) printf("r%d <- (@%o) bus-error %o\n", d, regs[s], v);
            // regs[d] = v;
            m_post_reg(m, d, v);
        }
        break;

    case M_LOADINDPM:
        if (regs[s] & 1) {
            mach_signals_odd();
            break;
        }
        if (cpu_read(previous_mode, 0, regs[s], &v) == 0) {
            // regs[d] = v;
            m_post_reg(m, d, v);
            if (debug) printf("r%d <- (pm-@%o) 0%o\n", d, regs[s], v);
        } else {
            if (debug) printf("r%d <- (pm-@%o) bus-error %o\n", d, regs[s], v);
            // regs[d] = v;
            m_post_reg(m, d, v);
        }
        break;

    case M_LOADINDB:
        if (cpu_read(current_mode, 0, regs[s], &v) == 0) {
            if (regs[s] & 1)
                // regs[d] = (v & 0xff00) >> 8;
                m_post_reg(m, d, (v & 0xff00) >> 8);
            else
                // regs[d] = v & 0xff;
                m_post_reg(m, d, v & 0xff);
        }
        break;

    case M_LOADPSW:
        switch (v) {
        case 0:
            // in user mode don't allow rti to pop mode or ipl
            if (current_mode == mode_user)
                psw = (regs[s] & 000037) | (0170000) | (psw&0340) | (psw&020);
            else
                psw = regs[s] | (psw & 020);
            break;

            //xxx changing current mode may break store of r6 below
            //new_r6=0; should fix this

        case 1:
            // exception
            psw = (regs[s] & ~0030000) | (current_mode << 12);
            break;

        case 2:
            // mtps
            if (current_mode == mode_user)
                psw = (regs[s] & 000037) | (0170000) | (psw&0340) | (psw&020);
            else {
                psw = ((regs[s] & 0xff) & ~020) | (psw & 020);
                if (debug) printf("new psw %o (from %o)\n", psw, regs[s]);
            }
            break;
        }

        new_r6 = 0;
        break;

    case M_STOREB:
        // regs[d] = (signed char)(regs[s] & 0xff);
        m_post_reg(m, d, (signed char)(regs[s] & 0xff));
        if (debug) printf("r%d <- r%d (byte 0%o)\n", d, s, regs[s] & 0xff);
        break;

    case M_STOREIND:
        if (cpu_write(current_mode, regs[d], regs[s])) {
        }
        break;

    case M_STOREINDPM:
        if (cpu_write(previous_mode, regs[d], regs[s])) {
        }
        break;

    case M_STOREINDB:
        if (cpu_write_byte(current_mode, regs[d], regs[s])) {
        }
        break;

    case M_STOREPSW:
        // regs[d] = psw;
        m_post_reg(m, d, psw);
        break;

    case M_STORESP:
        // regs[R_SP(v)] = regs[s];
        m_post_reg(m, R_SP(v), regs[s]);
        break;

    case M_ADD:
        if (debug) printf("r%d <- %o (%o + %o)\n",
                          d, regs[s] + regs[s2], regs[s], regs[s2]);
        // regs[d] = regs[s] + regs[s2];
        m_post_reg(m, d, regs[s] + regs[s2]);
        break;

    case M_ADD1:
        if (debug) printf("r%d <- %o (%o + %o)\n",
                          d, regs[d] + regs[s], regs[d], regs[s]);
        // regs[d] += regs[s];
        m_post_reg(m, d, regs[d] + regs[s]);
        break;

    case M_ADDI:
        // regs[d] += v;
        m_post_reg(m, d, regs[d] + v);
        break;

    case M_ADDIB:
        if (debug) printf("addib %o <- %o\n",
                          (regs[d] & 0xff00) | ((regs[d] + v) & 0xff),
                          regs[d]);
        // regs[d] = (regs[d] & 0xff00) | ((regs[d] + v) & 0xff);
        m_post_reg(m, d, (regs[d] & 0xff00) | ((regs[d] + v) & 0xff));
        break;

    case M_ADDC:
        if (debug) printf("addc %o <- %o + %o\n",
                          regs[d] + (psw & CC_C ? 1 : 0),
                          regs[d], psw & CC_C ? 1 : 0);
        // regs[d] += (psw & CC_C) ? 1 : 0;
        m_post_reg(m, d, regs[d] + (psw & CC_C ? 1 : 0));
        break;

    case M_ADDCB:
        // regs[d] =
        // (regs[d] & 0xff00) |
        // (((regs[d] & 0x00ff) + (psw & CC_C ? 1 : 0)) & 0xff);
        m_post_reg(m, d, 
                   (regs[d] & 0xff00) |
                   (((regs[d] & 0x00ff) + (psw & CC_C ? 1 : 0)) & 0xff));
        break;

    case M_SUB:
        // regs[d] = regs[s] - regs[s2];
        m_post_reg(m, d, regs[s] - regs[s2]);
        break;

    case M_SUBB:
        // regs[d] = (regs[d] & 0xff00) | ((regs[s] - regs[s2]) & 0xff);
        m_post_reg(m, d, (regs[d] & 0xff00) | ((regs[s] - regs[s2]) & 0xff));
        break;

    case M_SUBI:
        // regs[d] -= v;
        m_post_reg(m, d, regs[d] - v);
        break;

    case M_SUBIB:
        // regs[d] = (regs[d] & 0xff00) | ((regs[d] - v) & 0xff);
        m_post_reg(m, d, (regs[d] & 0xff00) | ((regs[d] - v) & 0xff));
        break;

    case M_SUBC:
        // regs[d] -= (psw & CC_C) ? 1 : 0;
        m_post_reg(m, d, regs[d] - (psw & CC_C ? 1 : 0));
        break;

    case M_AND:
        // regs[d] = regs[s] & regs[s2];
        m_post_reg(m, d, regs[s] & regs[s2]);
        break;

    case M_ANDB:
        // regs[d] = (regs[d] & 0xff00) | ((regs[s]&0xff) & (regs[s2]&0xff));
        m_post_reg(m, d,
                   (regs[d] & 0xff00) | ((regs[s]&0xff) & (regs[s2]&0xff)));
        break;

    case M_OR:
        // regs[d] = regs[s] | regs[s2];
        m_post_reg(m, d, regs[s] | regs[s2]);
        break;

    case M_ORB:
        // regs[d] = (regs[d] & 0xff00) | ((regs[s]&0xff) | (regs[s2]&0xff));
        m_post_reg(m, d,
                   (regs[d] & 0xff00) | ((regs[s]&0xff) | (regs[s2]&0xff)));
        break;

    case M_NOT:
        // regs[d] = ~regs[s];
        m_post_reg(m, d, ~regs[s]);
        break;

    case M_NOTB:
        if (debug) printf("notb d %o s %o result %o\n",
                          regs[d], regs[s], 
                          (regs[d] & 0xff00) | ((~regs[s]) & 0xff));

        // regs[d] = (regs[d] & 0xff00) | ((~regs[s]) & 0xff);
        m_post_reg(m, d, (regs[d] & 0xff00) | ((~regs[s]) & 0xff));
        break;

    case M_SXT:
        // regs[d] = (psw & CC_N) ? 0xffff : 0;
        m_post_reg(m, d, (psw & CC_N) ? 0xffff : 0);
        break;

    case M_DIV:
        // div32by16(&regs[d], &regs[d+1], regs[s], regs[s+1], regs[s2]);
        div32by16(&r0, &r1, regs[s], regs[s+1], regs[s2]);
        m_post_reg(m, d, r0);
        m_post_reg(m, d+1, r1);
        break;

    case M_MUL:
        // mul16by16(&regs[d], &regs[d+1], regs[d], regs[s]);
        mul16by16(&r0, &r1, regs[d], regs[s]);
        m_post_reg(m, d, r0);
        m_post_reg(m, d+1, r1);
        break;

    case M_XOR:
        // xor16(&regs[d], regs[d], regs[s]);
        xor16(&r0, regs[d], regs[s]);
        m_post_reg(m, d, r0);
        break;

    case M_FLAGS:
        if (v & 0100) {
            /* set */
            if (v & CC_N) psw |= CC_N;
            if (v & CC_Z) psw |= CC_Z;
            if (v & CC_V) psw |= CC_V;
            if (v & CC_C) psw |= CC_C;
        }
         
        if (v & 040) {
            /* clear */
            if (v & CC_N) psw &= ~CC_N;
            if (v & CC_Z) psw &= ~CC_Z;
            if (v & CC_V) psw &= ~CC_V;
            if (v & CC_C) psw &= ~CC_C;
        }
        break;

    case M_FLAGMUX:
        cc_n = psw & CC_N ? 1 : 0;
        cc_c = psw & CC_C ? 1 : 0;
        cc_v = psw & CC_V ? 1 : 0;
        cc_z = psw & CC_Z ? 1 : 0;
        psw &= ~017;

        if (v & 0x80) {
            /* byte */
            new_cc_n = (regs[d] & 0x80) ? 1 : 0;
            new_cc_z = ((regs[d] & 0xff) == 0) ? 1 : 0;
            new_cc_v = 0;
            new_cc_c = 0;
        } else {
            /* word */
            new_cc_n = (regs[d] & 0x8000) ? 1 : 0;
            new_cc_z = (regs[d] == 0) ? 1 : 0;
            new_cc_v = 0;
            new_cc_c = 0;
        }

        switch (v) {
        case FM_ADD:
            new_cc_v =
                (~(regs[R_S0]&0x8000) ^ (regs[R_D0]&0x8000)) &
                ( (regs[R_S0]&0x8000) ^ (regs[R_R0]&0x8000)) ? 1 : 0;
            new_cc_c = (u16)regs[R_R0] < (u16)regs[R_S0] ? 1 : 0;
            break;
        case FM_ADC:
            new_cc_v = (cc_c && (regs[d] == 0100000)) ? 1 : 0;
            new_cc_c = cc_c & new_cc_z;
            break;

        case FM_ASH:
            new_cc_v = regs[s] == 0 ? 0 : shift_sign_change16;
            new_cc_c = shift_out;
            printf("regs[s=%d] %o shift_out %d\n", s, regs[d], shift_out);
            break;
        case FM_ASHC:
            new_cc_v = regs[s] == 0 ? 0 : shift_sign_change32;
            new_cc_c = shift_out;
            break;
        case FM_ASL:
            if (debug) printf("FM_ASL: shift_out %d, new_cc_n %d\n",
                              shift_out, new_cc_n);
            new_cc_c = shift_out ? 1 : 0;
            new_cc_v = new_cc_n ^ new_cc_c;
            break;
        case FM_ASR:
        case FM_ASRB:
            new_cc_c = regs[s]&1 ? 1 : 0;
            new_cc_v = new_cc_n ^ new_cc_c;
            break;
        case FM_BIC:
        case FM_BICB:
        case FM_BIS:
        case FM_BISB:
        case FM_BIT:
        case FM_BITB:
            new_cc_c = cc_c;            /* unchanged */
            break;
        case FM_CLR:
        case FM_CLRB:
            new_cc_n = 0;
            new_cc_z = 1;
            break;
        case FM_CMP:
            new_cc_v =
                ( (regs[R_S0]&0x8000) ^ (regs[R_D0]&0x8000)) &
                (~(regs[R_D0]&0x8000) ^ (regs[R_R0]&0x8000)) ? 1 : 0;
            new_cc_c = (u16)regs[R_S0] < (u16)regs[R_D0] ? 1 : 0;
            break;
        case FM_COM:
        case FM_COMB:
            new_cc_v = 0;
            new_cc_c = 1;
            break;
        case FM_DEC:
            new_cc_v = (u16)regs[d] == 077777 ? 1 : 0;
            new_cc_c = cc_c;            /* unchanged */
            break;
        case FM_DIV:
            if (debug)
                printf("div_overflow %d, div_result %o, div_result_sign %d\n",
                       div_overflow, div_result, div_result_sign);

            if (div_overflow) {
                new_cc_n = div_result_sign;
                new_cc_z = 0;
                new_cc_v = 1;
                new_cc_c = 0;
            } else {
                new_cc_n = div_result_sign;
                new_cc_z = div_result == 0;
            }
            new_cc_c = 0;
            break;
        case FM_INC:
            new_cc_v = (regs[d] == 0100000) ? 1 : 0;
            new_cc_c = cc_c;            /* unchanged */
            break;
        case FM_INCB:
            /* doesn't set v? */
            break;
        case FM_MFPI:
        case FM_MFPD:
            new_cc_v = 0;
            new_cc_c = cc_c;            /* unchanged */
            break;
        case FM_MFPS:
            new_cc_n = (psw & 0x80) ? 1 : 0;
            new_cc_z = (u8)psw == 0 ? 1 : 0;
            new_cc_v = 0;
            new_cc_c = cc_c;            /* unchanged */
            break;
        case FM_MOV:
            new_cc_v = 0;
            new_cc_c = cc_c;            /* unchanged */
            break;
        case FM_MTPS:
            if (debug) printf("old psw %o\n", psw);
            new_cc_n = cc_n;
            new_cc_z = cc_z;
            new_cc_v = cc_v;
            new_cc_c = cc_c;
            break;
        case FM_MTPD:
        case FM_MTPI:
            new_cc_c = cc_c;            /* unchanged */
            break;
        case FM_MUL:
            new_cc_n = mul_result_sign;
            new_cc_z = mul_result == 0;
            new_cc_v = 0;
            new_cc_c = mul_overflow;
            break;
        case FM_NEG:
            new_cc_v = (regs[R_R0] == 0100000) ? 1 : 0;
            new_cc_c = new_cc_z ^ 1;
            break;
        case FM_ROL:
            new_cc_c = regs[s]&0x8000 ? 1 : 0;
            new_cc_v = new_cc_n ^ new_cc_c;
            break;
        case FM_ROR:
        case FM_RORB:
            new_cc_c = regs[s]&1 ? 1 : 0;
            new_cc_v = new_cc_n ^ new_cc_c;
            break;
        case FM_SBC:
            new_cc_v = (cc_c && (regs[R_S0] == 0077777)) ? 1 : 0;
            new_cc_c = (cc_c && (regs[R_S0] == 0177777)) ? 1 : 0;
            if (debug) printf("FM_SBC: cc_c %d, regs[R_S0] %o; new_cc_c %d\n",
                              cc_c, regs[R_S0], new_cc_c);
            break;
        case FM_SUB:
            new_cc_v =
                ( (regs[R_S0]&0x8000) ^ (regs[R_D0]&0x8000)) &
                (~(regs[R_S0]&0x8000) ^ (regs[R_R0]&0x8000)) ? 1 : 0;
            new_cc_c = (u16)regs[R_D0] < (u16)regs[R_S0] ? 1 : 0;
            break;
        case FM_SXT:
            new_cc_z = cc_n ^ 1;
            new_cc_v = 0;
            new_cc_c = cc_c;            /* unchanged */
            break;
        case FM_TST:
        case FM_TSTB:
            break;
        case FM_XOR:
            new_cc_v = 0;
            new_cc_c = cc_c;            /* unchanged */
            break;

        case FM_ASLB:
            new_cc_c = shift_out ? 1 : 0;
            new_cc_v = new_cc_n ^ new_cc_c;
            break;
        case FM_ADCB:
            new_cc_v = (cc_c && ((regs[d]&0xff) == 0200)) ? 1 : 0;
            new_cc_c = cc_c & new_cc_z;
            break;
        case FM_CMPB:
            new_cc_v =
                ( (regs[R_S0]&0x80) ^ (regs[R_D0]&0x80)) &
                (~(regs[R_D0]&0x80) ^ (regs[R_R0]&0x80)) ? 1 : 0;
            new_cc_c = (u8)regs[R_S0] < (u8)regs[R_D0] ? 1 : 0;
            break;
        case FM_DECB:
            new_cc_v = (u8)regs[d] == 0177 ? 1 : 0;
            break;
        case FM_MOVB:
            new_cc_v = 0;
            new_cc_c = cc_c;            /* unchanged */
            break;
        case FM_NEGB:
            new_cc_v = ((u8)regs[R_R0] == 0200) ? 1 : 0;
            new_cc_c = new_cc_z ^ 1;
            break;
        case FM_ROLB:
            new_cc_c = (regs[s] & 0x80) ? 1 : 0;
            new_cc_v = new_cc_n ^ new_cc_c;
            break;
        case FM_SBCB:
            new_cc_v = (cc_c && ((u8)regs[R_S0] == 0177)) ? 1 : 0;
            new_cc_c = (cc_c && ((u8)regs[R_S0] == 0377)) ? 1 : 0;
            break;
        case FM_SWAB:
            new_cc_v = 0;
            new_cc_c = 0;
            break;
        }

        if (new_cc_n) psw |= CC_N; /* cc_n */
        if (new_cc_z) psw |= CC_Z; /* cc_z */
        if (new_cc_v) psw |= CC_V; /* cc_v */
        if (new_cc_c) psw |= CC_C; /* cc_c */
        break;

    case M_CHECKSP:
        switch (v) {
        case 0:
            if (regs[6] < 0400) {
                mach_signals_oflo_next();
            }
            break;
        case 1:
            if (regs[6] < 0400) {
                mach_signals_oflo();
            }
            break;
        case 2:
            if (regs[6] <= 0400) {
                mach_signals_oflo();
            }
            break;
        }
        break;

    case M_INHIBIT:
        switch (v) {
        case 1:
            mach_trace_inhibit();
            break;
        }
        break;

    case M_JMP:
        bpred_inform(1, pc, regs[d]);
        tb_pc_flush();
        tb_pc_set(regs[d]);
        break;

    case M_BR:
        /* sign extend offset */
        if (v & 0x8000)
            offset = v | 0xffff0000;
        else
            offset = v;

        switch (d) {
        case B_ALWAYS: take_jump = 1; break;

#define BITSET(v, mask)	( ((v) & mask) ? 1 : 0 )

        case B_NE: take_jump = (psw & CC_Z) ? 0 : 1; break;
        case B_EQ: take_jump = (psw & CC_Z) ? 1 : 0; break;

        case B_GT: take_jump = (BITSET(psw, CC_Z) ||
                                (BITSET(psw, CC_N) ^ BITSET(psw, CC_V))) ? 0 : 1; break;

        case B_GE: take_jump = (BITSET(psw, CC_N) ^ BITSET(psw, CC_V)) ? 0 : 1; break;

        case B_LT: take_jump = (BITSET(psw, CC_N) ^ BITSET(psw, CC_V)) ? 1 : 0; break;

        case B_LE: take_jump = (BITSET(psw, CC_Z) ||
                                (BITSET(psw, CC_N) ^ BITSET(psw, CC_V))) ? 1 : 0; break;

        case B_PL: take_jump = (psw & CC_N) ? 0 : 1; break;
        case B_MI: take_jump = (psw & CC_N) ? 1 : 0; break;
        case B_HI: take_jump = (BITSET(psw, CC_C) | BITSET(psw, CC_Z)) ? 0 : 1; break;
        case B_LO: take_jump = (BITSET(psw, CC_C) | BITSET(psw, CC_Z)) ? 1 : 0; break;
        case B_VC: take_jump = (psw & CC_V) ? 0 : 1; break;
        case B_VS: take_jump = (psw & CC_V) ? 1 : 0; break;
        case B_CC: take_jump = (psw & CC_C) ? 0 : 1; break;
        case B_CS: take_jump = (psw & CC_C) ? 1 : 0; break;

        case B_REGZERO: take_jump = regs[s] == 0 ? 0 : 1; break;
        }

        if (debug)
            printf("branch %staken to %o\n",
                   take_jump ? "" : "not ",
                   pc + offset*2);

        bpred_inform(take_jump, pc, pc + offset*2); 

        if (take_jump) {
            tb_pc_flush();
            tb_pc_set(pc + offset*2);
        }
        break;

    case M_SWAB:
        // regs[d] = ((regs[s] & 0xff) << 8) | ((regs[s] >> 8) & 0xff);
        m_post_reg(m, d, ((regs[s] & 0xff) << 8) | ((regs[s] >> 8) & 0xff));
        break;

    case M_SHIFT:
        count = regs[s2] & 077;
        if (count & 040)
            count |= ~077;

        if (count > 0) {
            u32 r;
            if (debug)
                printf("r%o <- r%o (%o) << %d", d, s, regs[s], count);
            r = ((u32)regs[s]) << count;
            shift_out = (r & 0x10000) ? 1 : 0;
            // regs[d] = r;
            m_post_reg(m, d, r);
        } else {
            if (debug)
                printf("r%o <- r%o (%o) >> %d", d, s, regs[s], -count);
            shift_out = (regs[s] >> (-count - 1)) & 1;
            shift_sign = regs[s] & 0x8000;
            // regs[d] = regs[s] >> -count;
            r0 = regs[s] >> -count;

            if (shift_sign)
                // regs[d] |= 0177777 << (16 - -count);
                r0 |= 0177777 << (16 - -count);

            m_post_reg(m, d, r0);

            if (debug) printf("shift_out %d, regs[s=%o] %o, shift %o\n",
                              shift_out, s, regs[s],
                              regs[s] >> (-count - 1));
        }

        if (debug) printf(" (result %o)\n", regs[d]);
        break;

    case M_SHIFTI:
        if (vs > 0) {
            if (debug)
                printf("r%o <- r%o (%o) << %d", d, s, regs[s], v);
            shift_out = (((u32)regs[s]) << v) & 0x10000;
            // regs[d] = regs[s] << v;
            r0 = regs[s] << v;
        } else {
            if (debug)
                printf("r%o <- r%o (%o) >> %d", d, s, regs[s], -vs);
            shift_out = (regs[s] >> (-vs - 1)) & 1;
            // regs[d] = regs[s] >> -vs;
            r0 = regs[s] >> -vs;
        }

        m_post_reg(m, d, r0);
        if (debug) printf(" (result %o)\n", r0);
        break;

    case M_SHIFT32:
	{
            int count = regs[s2] & 077;
            u32 l32;
            if (count & 040)
                count |= ~077;

            l32 = (regs[s] << 16) | regs[s+1];

            if (count > 0) {
                if (debug)
                    printf("r%o <- r%o (%o) << %d", d, s, l32, count);
                shift_out = (l32 & 0x80000000) ? 1 : 0;
                l32 <<= count;
            } else {
                count = -count;
                if (debug)
                    printf("r%o <- r%o (%o) >> %d", d, s, l32, count);
                shift_out = (l32 & (1 << (count-1))) ? 1 : 0;
                l32 >>= count;
            }
            // regs[d] = (l32 >> 16) & 0xffff;
            // regs[d+1] = l32 & 0xffff;;
            r0 = (l32 >> 16) & 0xffff;
            r1 = l32 & 0xffff;;
        }

        m_post_reg(m, d, r0);
        m_post_reg(m, d+1, r1);
        //if (debug) printf(" (result %o %o)\n", regs[d], regs[d+1]);
        if (debug) printf(" (result %o %o)\n", r0, r1);
        break;

    case M_ROTATE:
        if (vs > 0) {
            // regs[d] = (regs[s] << 1) | (psw & CC_C ? 1 : 0);
            r0 = (regs[s] << 1) | (psw & CC_C ? 1 : 0);
            new_cc_c = regs[s] & 0x8000;
        } else {
            // regs[d] = (psw & CC_C ? 0x8000 : 0) | (regs[s] >> 1);
            r0 = (psw & CC_C ? 0x8000 : 0) | (regs[s] >> 1);
            new_cc_c = regs[s] & 1;
        }

        m_post_reg(m, d, r0);

        psw = (psw & ~017);
        // if (regs[d] & 0x8000)    psw |= CC_N;
        // if (regs[d] == 0)        psw |= CC_Z;
        if (r0 & 0x8000)         psw |= CC_N;
        if (r0 == 0)             psw |= CC_Z;
        if (new_cc_c)            psw |= CC_C;
        if (((psw & CC_N)?1:0) ^ ((psw & CC_C)?1:0)) psw |= CC_V;
        break;

    case M_ROTATEB:
        regs[d] &= 0xff00;
        if (vs > 0) {
            // regs[d] |= ((regs[s]&0x7f) << 1) | (psw & CC_C ? 1 : 0);
            r0 = regs[d] | ((regs[s]&0x7f) << 1) | (psw & CC_C ? 1 : 0);
            new_cc_c = regs[s] & 0x80;
        } else {
            // regs[d] |= (psw & CC_C ? 0x80 : 0) | ((regs[s]&0xff) >> 1);
            r0 = regs[d] | (psw & CC_C ? 0x80 : 0) | ((regs[s]&0xff) >> 1);
            new_cc_c = regs[s] & 1;
        }

        m_post_reg(m, d, r0);

        psw = (psw & ~017);
        // if (regs[d] & 0x80)      psw |= CC_N;
        // if ((regs[d]&0xff) == 0) psw |= CC_Z;
        if (r0 & 0x80)           psw |= CC_N;
        if ((r0 & 0xff) == 0)    psw |= CC_Z;
        if (new_cc_c)            psw |= CC_C;
        if (((psw & CC_N)?1:0) ^ ((psw & CC_C)?1:0)) psw |= CC_V;
        break;

    case M_ASR:
        if (vs > 0) {
            // regs[d] = (regs[s] << 1) | (psw & CC_C ? 1 : 0);
            r0 = (regs[s] << 1) | (psw & CC_C ? 1 : 0);
            new_cc_c = regs[s] & 0x8000;
        } else {
            // regs[d] = (regs[s] & 0x8000) | (regs[s] >> 1);
            r0 = (regs[s] & 0x8000) | (regs[s] >> 1);
            new_cc_c = regs[s] & 1;
        }

        m_post_reg(m, d, r0);
        break;

    default:
        printf("unknown op code 0x%x\n", op);
        break;
    }

//    if (new_r6) {
//        // regs[R_SP(current_mode)] = regs[6];
//        m_post_reg(m, R_SP(current_mode), regs[6]);
//    }
}

void m_commit_isn(m_fifo_t *m)
{
#if 0
    if (m->r_valid) {
        regs[m->r_reg] = m->r_val;
        if (debug) printf("m_commit_isn: r%d <- %06o\n", m->r_reg, m->r_val);
    }

    if (m->r_valid2) {
        regs[m->r_reg2] = m->r_val2;
        if (debug) printf("m_commit_isn: r%d <- %06o\n", m->r_reg, m->r_val);
    }
#endif
}

void m_pipe_flush(void)
{
    if (m_current) {
        m_current->flush = 1;
    } else {
        printf("m_pipe_flush: no current instruction!\n");
        exit(3);
    }
}

void m_fifo_execute(void)
{
    int i;

    if (m_fifo_depth == 0) {
        printf("no instructions!\n");
        exit(1);
    }

    for (i = 0; i < m_fifo_depth; i++)  {
        m_current = &m_fifo[i];
        m_execute_isn(m_current);

        if (m_current->flush) {
            if (debug) printf("flushing pipe after entry %d\n", i);
            break;
        }
    }

    for (i = 0; i < m_fifo_depth; i++)  {
        m_current = &m_fifo[i];
        m_commit_isn(m_current);

        if (m_current->flush)
            break;
    }

    m_current = NULL;
}


void m_fifo_dump(void)
{
    int i;
    u32 isn;

    for (i = 0; i < m_fifo_depth; i++)  {
        char str[128];
        m_fifo_t *m = &m_fifo[i];
        m_dis_op(m, str);
        printf("%02d %02x-%02x-%02x-%02x-%04x %s",
               i,
               m->op, m->d, m->s1, m->s2, m->v,
               str);

        int l = 30 - strlen(str);
        while (l-- > 0) printf(" ");

        if (m->r_valid) {
            printf("  r%d <- %06o", m->r_reg, m->r_val);
        }
        if (m->r_valid2) {
            printf("  r%d <- %06o", m->r_reg2, m->r_val2);
        }
        printf("\n");
    }
}

void m_fifo_push(m_fifo_t *m)
{
    if (m_fifo_depth < 32) {
        m_fifo[m_fifo_depth++] = *m;
    } else {
        printf("m_fifo overflow\n");
    }
}

void m_fifo_reset(void)
{
    m_fifo_depth = 0;
}

void m_push_isn(int op, int d, int s1, int s2, int v)
{
    m_fifo_t mm;

    mm.op = op;
    mm.d = d;
    mm.s1 = s1;
    mm.s2 = s2;
    mm.v = v;
    mm.r_valid = 0;
    mm.r_valid2 = 0;
    mm.flush = 0;

    m_fifo_push(&mm);
}

/* ------------------------------------------------------------------ */

void m_nop(void)
{
    m_push_isn(M_NOP, 0, 0, 0, 0);
}

void m_load(int dr, int sr)
{
    m_push_isn(M_LOAD, dr, sr, 0, 0);
}

void m_loadb(int dr, int sr)
{
    m_push_isn(M_LOADB, dr, sr, 0, 0);
}

void m_loadind(int dr, int sr)
{
    m_push_isn(M_LOADIND, dr, sr, 0, 0);
}

void m_loadindpm(int dr, int sr)
{
    m_push_isn(M_LOADINDPM, dr, sr, 0, 0);
}

void m_loadindb(int dr, int sr)
{
    m_push_isn(M_LOADINDB, dr, sr, 0, 0);
}

void m_loadi(int dr, int val)
{
    m_push_isn(M_LOADI, dr, 0, 0, val);
}

void m_loadib(int dr, int val)
{
    m_push_isn(M_LOADIB, dr, 0, 0, val);
}

void m_loadpsw(int sr, int v)
{
    m_push_isn(M_LOADPSW, 0, sr, 0, v);
}

void m_storeb(int dr, int sr)
{
    /* sign extends */
    m_push_isn(M_STOREB, dr, sr, 0, 0);
}

void m_storeind(int dr, int sr)
{
    m_push_isn(M_STOREIND, dr, sr, 0, 0);
}

void m_storeindpm(int dr, int sr)
{
    m_push_isn(M_STOREINDPM, dr, sr, 0, 0);
}

void m_storeindb(int dr, int sr)
{
    m_push_isn(M_STOREINDB, dr, sr, 0, 0);
}

void m_storepsw(int dr)
{
    m_push_isn(M_STOREPSW, dr, 0, 0, 0);
}

void m_storesp(int v, int sr)
{
    m_push_isn(M_STORESP, 0, sr, 0, v);
}

void m_add1(int dr, int sr)
{
    m_push_isn(M_ADD1, dr, sr, 0, 0);
}

void m_add2(int dr, int s1, int s2)
{
    m_push_isn(M_ADD, dr, s1, s2, 0);
}

void m_addi(int dr, int val)
{
    m_push_isn(M_ADDI, dr, 0, 0, val);
}

void m_addib(int dr, int val)
{
    m_push_isn(M_ADDIB, dr, 0, 0, val);
}

void m_addc(int dr, int sr)
{
    m_push_isn(M_ADDC, dr, sr, 0, 0);
}

void m_addcb(int dr, int sr)
{
    m_push_isn(M_ADDCB, dr, sr, 0, 0);
}

void m_sub(int dr, int s1, int s2)
{
    m_push_isn(M_SUB, dr, s1, s2, 0);
}

void m_subb(int dr, int s1, int s2)
{
    m_push_isn(M_SUBB, dr, s1, s2, 0);
}

void m_subi(int dr, int val)
{
    m_push_isn(M_SUBI, dr, 0, 0, val);
}

void m_subib(int dr, int val)
{
    m_push_isn(M_SUBIB, dr, 0, 0, val);
}

void m_subc(int dr, int sr)
{
    m_push_isn(M_SUBC, dr, sr, 0, 0);
}

void m_flags(int dr, int sr, int v)
{
    m_push_isn(M_FLAGS, dr, sr, 0, v);
}

void m_flagmux(int dr, int sr, int v)
{
    m_push_isn(M_FLAGMUX, dr, sr, 0, v);
}

void m_checksp(int v)
{
    m_push_isn(M_CHECKSP, 0, 0, 0, v);
}

void m_inhibit(int v)
{
    m_push_isn(M_INHIBIT, 0, 0, 0, v);
}

void m_br(int code, int v)
{
    m_push_isn(M_BR, code, 0, 0, v);
}

void m_br_reg(int code, int reg, int v)
{
    m_push_isn(M_BR, code, reg, 0, v);
}

void m_and(int dr, int s1, int s2)
{
    m_push_isn(M_AND, dr, s1, s2, 0);
}

void m_andb(int dr, int s1, int s2)
{
    m_push_isn(M_ANDB, dr, s1, s2, 0);
}

void m_or(int dr, int s1, int s2)
{
    m_push_isn(M_OR, dr, s1, s2, 0);
}

void m_orb(int dr, int s1, int s2)
{
    m_push_isn(M_ORB, dr, s1, s2, 0);
}

void m_not(int dr, int sr)
{
    m_push_isn(M_NOT, dr, sr, 0, 0);
}

void m_notb(int dr, int sr)
{
    m_push_isn(M_NOTB, dr, sr, 0, 0);
}

void m_ror(int dr, int sr, int v)
{
    m_push_isn(M_ROTATE, dr, sr, 0, v);
}

void m_asr(int dr, int sr, int v)
{
    m_push_isn(M_ASR, dr, sr, 0, v);
}

void m_sxt(int dr, int sr)
{
    m_push_isn(M_SXT, dr, sr, 0, 0);
}


/* ------------------------------------------------------ */

/*
 * PDP-11 Addressing Modes:
 *
 * mode,symbol,ea1	ea2		ea3		data		side-eff
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
 */

void encode0(int op)
{
    m_push_isn(op, 0, 0, 0, 0);
}

void encode_rti(void)
{
    if (debug) {
        printf("encode_rti\n");
    }

    /* pop pc */
    m_loadind(7, 6);
    m_addi(6, 2);

    /* pop psw */
    m_loadind(R_S0, 6);
    m_addi(6, 2);
    m_loadpsw(R_S0, 0);
}

void encode_trace_inhibit(void)
{
    m_inhibit(1);
}

void encode_rtt(void)
{
    if (debug) {
        printf("encode_rtt\n");
    }

    encode_trace_inhibit();

    /* pop pc */
    m_loadind(7, 6);
    m_addi(6, 2);

    /* pop psw */
    m_loadind(R_S0, 6);
    m_addi(6, 2);
    m_loadpsw(R_S0, 0);
}

void encode_new_cc(int bits, int set)
{
    if (debug) {
        printf("encode_new_cc: bits %o set=%d\n", bits, set);
    }

    if (set) {
        m_flags(0, 0, 0100 | bits);
    } else {
        m_flags(0, 0, 0040 | bits);
    }
}

void encode_rts(int reg)
{
    if (debug) {
        printf("encode_rts: reg %o\n", reg);
    }

    m_push_isn(M_JMP, reg, 0, 0, 0);
    m_loadind(reg, 6);
    m_addi(6, 2);
}

void encode_spl(int spl)
{
}

void encode_mark(int nn)
{
    /*
     * sp <- pc + 2*nn
     * pc <- r5
     * r5 <- (sp)+
     */

    m_load(R_S0, 7);
    m_addi(R_S0, 2*nn);
    m_load(6, R_S0);

    m_load(7, 5);

    m_load(R_S0, 6);
    m_addi(6, 2);
    m_loadind(5, R_S0);
}

void __encode_ea_from_spec(int dest, int rmode, int reg, u16 arg,
                           int byte, int pm)
{
    /* special case r7 due to prefetch */
    if (reg == 7) {
        switch (rmode) {
        case 0:
            break;
        case 1:
            m_loadi(dest+1, reg);
            break;
        case 2:
            m_load(dest+1, reg);
            m_addi(reg, 2);
            break;
        case 3:
            m_loadi(dest+1, arg);
            m_addi(reg, 2);
            break;
        case 4:
            m_loadi(dest+1, arg);
            m_subi(reg, 2);
            break;
        case 5:
            m_loadi(dest+1, arg);
            m_subi(reg, 2);
            break;
        case 6:
            m_loadi(dest+1, arg);
            m_addi(reg, 2);
            m_add1(dest+1, reg);
            break;
        case 7:
            m_loadi(dest+2, arg);
            m_addi(reg, 2);
            m_add1(dest+2, reg);
            if (pm)
                m_loadindpm(dest+1, dest+2);
            else
                m_loadind(dest+1, dest+2);
            break;
        }
m_nop();
        return;
    }

    if (dest > 13) {
        printf("__encode_ea_from_spec() dest=%d; DEST TOO BIG!\n", dest);
        exit(1);
    }

    switch (rmode) {
    case 1:
        m_load(dest+1, reg);
        break;
    case 2:
        m_load(dest+1, reg);
        m_addi(reg, (byte && reg < 6) ? 1 : 2);
        break;
    case 3:
        m_loadind(dest+1, reg);
        m_addi(reg, 2);
        break;
    case 4:
        m_subi(reg, (byte && reg < 6) ? 1 : 2);
        m_load(dest+1, reg);
        break;
    case 5: 
        m_subi(reg, 2);
        m_loadind(dest+1, reg);
        break;
    case 6:
        m_loadi(dest+1, arg);
        m_addi(7, 2);
        m_add1(dest+1, reg);
        break;
    case 7:
        m_loadi(dest+2, arg);
        m_addi(7, 2);
        m_add1(dest+2, reg);
        if (pm)
            m_loadindpm(dest+1, dest+2);
        else
            m_loadind(dest+1, dest+2);
        break;
    }
m_nop();
}

void _encode_ea_from_spec(int dest, int mode, int reg, u16 arg)
{
    __encode_ea_from_spec(dest, mode, reg, arg, 0, 0);
}

void _encode_ea_from_spec_byte(int dest, int mode, int reg, u16 arg)
{
    __encode_ea_from_spec(dest, mode, reg, arg, 1, 0);
}


/* put ss_data/dd_data in R_S0/R_D0 */
void __encode_load_from_spec(int dest, int mode, int reg, u16 arg, int byte)
{
    /* special case r7, since we prefetch pc */
    if (reg == 7) {
        switch (mode) {
        case 0:
            m_load(dest, reg);
            break;
        case 1:
            m_loadi(dest, arg);
            break;
        case 2:
            m_loadi(dest, arg);
            m_addi(reg, 2);
            break;
        case 3:
            m_loadi(dest+1, arg);
            if (byte)
                m_loadindb(dest, dest+1);
            else
                m_loadind(dest, dest+1);
            m_addi(reg, 2);
            break;
        case 4:
            m_loadi(dest, arg);
            m_subi(reg, 2);
            break;
        case 5:
            m_loadi(dest+1, arg);
            if (byte)
                m_loadindb(dest, dest+1);
            else
                m_loadind(dest, dest+1);
            m_subi(reg, 2);
            break;
        case 6:
            m_loadi(dest+1, arg);
            m_addi(reg, 2);
            m_add1(dest+1, reg);
            if (byte)
                m_loadindb(dest, dest+1);
            else
                m_loadind(dest, dest+1);
            break;
        case 7:
            m_loadi(dest+2, arg);
            m_addi(reg, 2);
            m_add1(dest+2, reg);
            m_loadind(dest+1, dest+2);
            if (byte)
                m_loadindb(dest, dest+1);
            else
                m_loadind(dest, dest+1);
            break;
        }
m_nop();
        return;
    }

    switch (mode) {
    case 0:
        m_load(dest, reg);
        break;
    default:
        __encode_ea_from_spec(dest, mode, reg, arg, byte, 0);

        if (byte)
            m_loadindb(dest, dest+1);
        else
            m_loadind(dest, dest+1);
    }

    /* check stack pointer */
    if (reg == 6 && (mode == 4 || mode == 5))
        m_checksp(0);

m_nop();
}

void _encode_load_from_spec(int dest, int mode, int reg, u16 arg)
{
    __encode_load_from_spec(dest, mode, reg, arg, 0);
}

void _encode_load_from_spec_byte(int dest, int mode, int reg, u16 arg)
{
    __encode_load_from_spec(dest, mode, reg, arg, 1);
}


void __encode_store_result(int result_reg, int ea_reg, int dmode, int dreg, u16 dst, int byte)
{
    if (dreg == 7) {
        printf("** result_reg %d, ea_reg %d, dd %o%o, dst %o byte=%d\n",
               result_reg, ea_reg, dmode, dreg, dst, byte);
#if 1
        switch (dmode) {
        case 0:
            m_load(dreg, result_reg);
            break;
        case 1:
            if (byte)
                m_storeindb(dreg, result_reg);
            else
                m_storeind(dreg, result_reg);
            break;
        case 2:
        case 3:
            if (byte)
                m_storeindb(ea_reg, result_reg);
            else
                m_storeind(ea_reg, result_reg);
//            m_addi(dreg, 2);
            break;
        case 4:
            if (byte)
                m_storeindb(ea_reg, result_reg);
            else
                m_storeind(ea_reg, result_reg);
//            m_subi(dreg, 2);
            break;
        case 5: 
//            m_subi(dreg, 2);
        case 6:
        case 7:
            if (byte)
                m_storeindb(ea_reg, result_reg);
            else
                m_storeind(ea_reg, result_reg);
            break;
        }
        return;
#endif
    }

    switch (dmode) {
    case 0:
        if (byte)
            m_loadb(dreg, result_reg);
        else
            m_load(dreg, result_reg);
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5: 
//       if (dreg == 7) {
//            break;
//        }
        /* fall through */
    case 6:
    case 7:
        if (byte)
            m_storeindb(ea_reg, result_reg);
        else
            m_storeind(ea_reg, result_reg);
        break;
    }
}

void _encode_store_result(int result, int ea, int dmode, int dreg, u16 dst)
{
    __encode_store_result(result, ea, dmode, dreg, dst, 0);
}

void _encode_store_result_byte(int result, int ea, int dmode, int dreg, u16 dst)
{
    __encode_store_result(result, ea, dmode, dreg, dst, 1);
}

/* ------------------------------------------------------------------------ */


void encode_mov(int smode, int sreg, int dmode, int dreg, u16 src, u16 dst)
{
    if (debug) {
        printf("encode_mov: ss %o%o dd %o%o s=%o d=%o\n",
               smode, sreg, dmode, dreg, src, dst);
    }

// if (smode == 2 && sreg == 7 && dmode == 0) {
//    _encode_load_from_spec(dreg, smode, sreg, src);
//    return;
// }

// if (smode == 0 && (dmode <  2)) {
//    _encode_load_from_spec(dreg, smode, sreg, src);
//    return;
// }

    /* S0 <- ss */
    _encode_load_from_spec(R_S0, smode, sreg, src);

    /* S0 -> dd */
    _encode_ea_from_spec(R_D0, dmode, dreg, dst);

    /* set flags before store; store could be to 177776 (psw) */
    m_flagmux(R_S0, R_D0, FM_MOV);

    _encode_store_result(R_S0, R_D1, dmode, dreg, dst);
}

void encode_movb(int smode, int sreg, int dmode, int dreg, u16 src, u16 dst)
{
    if (debug) {
        printf("encode_mov: ss %o%o dd %o%o s=%o d=%o\n",
               smode, sreg, dmode, dreg, src, dst);
    }

    /* S0 <- ss */
    _encode_load_from_spec_byte(R_S0, smode, sreg, src);

    if (dmode == 0) {
        /* need to sign extend */
        m_storeb(dreg, R_S0);
    } else {
        /* S0 -> dd */
        _encode_ea_from_spec_byte(R_D0, dmode, dreg, dst);

        _encode_store_result_byte(R_S0, R_D1, dmode, dreg, dst);
    }

    m_flagmux(R_S0, R_D0, FM_MOVB);
}

void encode_cmp(int smode, int sreg, int dmode, int dreg, u16 src, u16 dst)
{
    if (debug) {
        printf("encode_cmp: ss %o%o dd %o%o s=%o d=%o\n",
               smode, sreg, dmode, dreg, src, dst);
    }

    /* S0 <- ss */
    _encode_load_from_spec(R_S0, smode, sreg, src);

    /* D0 <- dd */
    _encode_load_from_spec(R_D0, dmode, dreg, dst);

    /* src - dst */
    m_sub(R_R0, R_S0, R_D0);
    m_flagmux(R_R0, R_D0, FM_CMP);
}

void encode_cmpb(int smode, int sreg, int dmode, int dreg, u16 src, u16 dst)
{
    if (debug) {
        printf("encode_cmpb: ss %o%o dd %o%o s=%o d=%o\n",
               smode, sreg, dmode, dreg, src, dst);
    }

    /* S0 <- ss */
    _encode_load_from_spec_byte(R_S0, smode, sreg, src);

    /* D0 <- dd */
    _encode_load_from_spec_byte(R_D0, dmode, dreg, dst);

    /* src - dst */
    m_subb(R_R0, R_S0, R_D0);
    m_flagmux(R_R0, R_D0, FM_CMPB);
}


void encode_bit(int smode, int sreg, int dmode, int dreg, u16 src, u16 dst)
{
    if (debug) {
        printf("encode_bit: ss %o%o dd %o%o s=%o d=%o\n",
               smode, sreg, dmode, dreg, src, dst);
    }

    /* S0 <- ss */
    _encode_load_from_spec(R_S0, smode, sreg, src);

    /* D0 <- dd */
    _encode_load_from_spec(R_D0, dmode, dreg, dst);

    m_and(R_R0, R_S0, R_D0);
    m_flagmux(R_R0, R_D0, FM_BIT);
}

void encode_bitb(int smode, int sreg, int dmode, int dreg, u16 src, u16 dst)
{
    if (debug) {
        printf("encode_bit: ss %o%o dd %o%o s=%o d=%o\n",
               smode, sreg, dmode, dreg, src, dst);
    }

    /* S0 <- ss */
    _encode_load_from_spec_byte(R_S0, smode, sreg, src);

    /* D0 <- dd */
    _encode_load_from_spec_byte(R_D0, dmode, dreg, dst);

    m_andb(R_R0, R_S0, R_D0);
    m_flagmux(R_R0, R_D0, FM_BITB);
}

void encode_bic(int smode, int sreg, int dmode, int dreg, u16 src, u16 dst)
{
    if (debug) {
        printf("encode_bic: ss %o%o dd %o%o s=%o d=%o\n",
               smode, sreg, dmode, dreg, src, dst);
    }

    /* S0 <- ss */
    _encode_load_from_spec(R_S0, smode, sreg, src);

    /* D0 <- dd */
    _encode_load_from_spec(R_D0, dmode, dreg, dst);

    m_not(R_R0, R_S0);
    m_and(R_R0, R_D0, R_R0);
    m_flagmux(R_R0, R_D0, FM_BIC);

    _encode_store_result(R_R0, R_D1, dmode, dreg, dst);
}

void encode_bicb(int smode, int sreg, int dmode, int dreg, u16 src, u16 dst)
{
    if (debug) {
        printf("encode_bicb: ss %o%o dd %o%o s=%o d=%o\n",
               smode, sreg, dmode, dreg, src, dst);
    }

    /* S0 <- ss */
    _encode_load_from_spec_byte(R_S0, smode, sreg, src);

    /* D0 <- dd */
    _encode_load_from_spec_byte(R_D0, dmode, dreg, dst);

    m_notb(R_R0, R_S0);
    m_andb(R_R0, R_D0, R_R0);
    m_flagmux(R_R0, R_D0, FM_BICB);

    _encode_store_result_byte(R_R0, R_D1, dmode, dreg, dst);
}

void encode_bis(int smode, int sreg, int dmode, int dreg, u16 src, u16 dst)
{
    if (debug) {
        printf("encode_bis: ss %o%o dd %o%o s=%o d=%o\n",
               smode, sreg, dmode, dreg, src, dst);
    }

    /* S0 <- ss */
    _encode_load_from_spec(R_S0, smode, sreg, src);

    /* D0 <- dd */
    _encode_load_from_spec(R_D0, dmode, dreg, dst);

    m_or(R_R0, R_S0, R_D0);
    m_flagmux(R_R0, R_D0, FM_BIS);

    _encode_store_result(R_R0, R_D1, dmode, dreg, dst);
}

void encode_bisb(int smode, int sreg, int dmode, int dreg, u16 src, u16 dst)
{
    if (debug) {
        printf("encode_bisb: ss %o%o dd %o%o s=%o d=%o\n",
               smode, sreg, dmode, dreg, src, dst);
    }

    /* S0 <- ss */
    _encode_load_from_spec_byte(R_S0, smode, sreg, src);

    /* D0 <- dd */
    _encode_load_from_spec_byte(R_D0, dmode, dreg, dst);

    m_orb(R_R0, R_S0, R_D0);
    m_flagmux(R_R0, R_D0, FM_BISB);

    _encode_store_result_byte(R_R0, R_D1, dmode, dreg, dst);
}

void encode_add(int smode, int sreg, int dmode, int dreg, u16 src, u16 dst)
{
    if (debug) {
        printf("encode_add: ss %o%o dd %o%o s=%o d=%o\n",
               smode, sreg, dmode, dreg, src, dst);
    }

    /* S0 <- ss */
    _encode_load_from_spec(R_S0, smode, sreg, src);

    /* D0 <- dd */
    _encode_load_from_spec(R_D0, dmode, dreg, dst);

    m_add2(R_R0, R_S0, R_D0);
    m_flagmux(R_R0, 0, FM_ADD);

    /* R0 -> dd */
    _encode_store_result(R_R0, R_D1, dmode, dreg, dst);
}

void encode_sub(int smode, int sreg, int dmode, int dreg, u16 src, u16 dst)
{
    if (debug) {
        printf("encode_sub: ss %o%o dd %o%o s=%o d=%o\n",
               smode, sreg, dmode, dreg, src, dst);
    }

    /* S0 <- ss */
    _encode_load_from_spec(R_S0, smode, sreg, src);

    /* D0 <- dd */
    _encode_load_from_spec(R_D0, dmode, dreg, dst);

    m_sub(R_R0, R_D0, R_S0);
    m_flagmux(R_R0, 0, FM_SUB);

    /* R0 -> dd */
    _encode_store_result(R_R0, R_D1, dmode, dreg, dst);
}


void encode_clr(int dmode, int dreg, u16 dst)
{
    if (debug) {
        printf("encode_clr: dd %o%o d=%o\n",
               dmode, dreg, dst);
    }

    if (dmode >= 2) {
        _encode_ea_from_spec(R_S0, dmode, dreg, dst);
    }

    if (dmode != 0) {
        m_loadi(R_S0, 0);
    }

    if (dreg == 7) {
        switch (dmode) {
        case 0:
            m_flagmux(dreg, 0, FM_CLR);
            m_loadi(dreg, 0);
            break;
        case 1:
            m_flagmux(R_S0, 0, FM_CLR);
            m_storeind(dreg, R_S0);
            break;
        case 2:
        case 4:
            m_flagmux(R_S0, 0, FM_CLR);
            m_storeind(R_S1, R_S0);
            break;
        case 3:
        case 5:
            m_flagmux(R_S0, 0, FM_CLR);
            m_storeind(R_S1, R_S0);
            break;
        case 6:
            m_flagmux(R_S0, 0, FM_CLR);
            m_storeind(R_S1, R_S0);
            break;
        case 7:
            m_flagmux(R_S0, 0, FM_CLR);
            m_storeind(R_S1, R_S0);
            break;
        }
    } else {
        switch (dmode) {
        case 0:
            m_flagmux(dreg, 0, FM_CLR);
            m_loadi(dreg, 0);
            break;
        case 1:
            m_flagmux(R_S0, 0, FM_CLR);
            m_storeind(dreg, R_S0);
            break;
        case 2:
        case 3:
        case 4:
        case 5: 
        case 6:
        case 7:
            m_flagmux(R_S0, 0, FM_CLR);
            m_storeind(R_S1, R_S0);
            break;
        }
    }
}

void encode_clrb(int dmode, int dreg, u16 dst)
{
    if (debug) {
        printf("encode_clrb: dd %o%o d=%o\n",
               dmode, dreg, dst);
    }

    if (dmode >= 2) {
        _encode_ea_from_spec_byte(R_S0, dmode, dreg, dst);
    }

    if (dmode != 0) {
        m_loadi(R_S0, 0);
    }

    if (dreg == 7) {
        switch (dmode) {
        case 0:
            m_flagmux(dreg, 0, FM_CLRB);
            m_loadib(dreg, 0);
            break;
        case 1:
        case 2:
        case 4:
            m_flagmux(R_S0, 0, FM_CLRB);
            m_storeindb(dreg, R_S0);
        case 3:
        case 5:
            m_loadi(R_D1, dst);
            m_flagmux(R_S0, 0, FM_CLRB);
            m_storeindb(R_D1, R_S0);
            break;
        case 6:
            m_loadi(R_D1, dst);
            m_add1(R_D1, dreg);
            m_flagmux(R_S0, 0, FM_CLRB);
            m_storeindb(R_D1, R_S0);
            break;
        case 7:
            m_loadi(R_D2, dst);
            m_add1(R_D2, dreg);
            m_loadind(R_D1, R_D2);
            m_flagmux(R_S0, 0, FM_CLRB);
            m_storeindb(R_D1, R_S0);
            break;
        }
    } else {
        switch (dmode) {
        case 0:
            m_flagmux(dreg, 0, FM_CLRB);
            m_loadib(dreg, 0);
            break;
        case 1:
            m_flagmux(R_S0, R_S0, FM_CLRB);
            m_storeindb(dreg, R_S0);
            break;
        case 2:
        case 3:
        case 4:
        case 5: 
        case 6:
        case 7:
            m_flagmux(R_S0, R_S0, FM_CLRB);
            m_storeindb(R_S1, R_S0);
            break;
        }
    }

}

void encode_branch(int code, u8 offset8)
{
    int offset;

    if (debug) {
        printf("encode_branch: code %d offset=%o\n", code, offset8);
    }

    offset = (offset8 & 0200) ?
        offset8 | 0177400 :
        offset8;

    m_br(code, offset & 0xffff);
}

void encode_sob(int reg, u8 offset6)
{
    int offset;

    offset = -offset6;

    m_subi(reg, 1);
    m_br_reg(B_REGZERO, reg, offset & 0xffff);
}

void encode_mfpi(int smode, int sreg, u16 src)
{
    if (debug) printf("encode_mfpi %o%o %o\n", smode, sreg, src);
    if (smode == 0 && sreg == 6) {
        m_load(R_S0, R_SP(previous_mode));
    } else {
        __encode_ea_from_spec(R_S0, smode, sreg, src, 0, 1);
        m_loadindpm(R_S0, R_S1);
    }

    m_subi(6, 2);
    m_storeind(6, R_S0);
    m_flagmux(R_S0, R_S0, FM_MFPI);
}

void encode_mtpi(int dmode, int dreg, u16 dst)
{
    if (debug) printf("encode_mtpi %o%o %o\n", dmode, dreg, dst);

    /* pop */
    m_loadind(R_S0, 6);
    m_addi(6, 2);
    m_flagmux(R_S0, R_S0, FM_MTPI);

    if (dmode == 0 && dreg == 6) {
        m_storesp(previous_mode, R_S0);
    } else {
        __encode_ea_from_spec(R_D0, dmode, dreg, dst, 0, 1);
        m_storeindpm(R_D1, R_S0);
    }
}


void encode_mtps(int smode, int sreg, u16 src)
{
    if (debug) printf("encode_mtps: ss %o%o s=%o\n", smode, sreg, src);

    if (smode == 0) {
        m_loadpsw(sreg, 2);
    } else {
        _encode_load_from_spec_byte(R_S0, smode, sreg, src);
        m_loadpsw(R_S0, 2);
    }

    m_flagmux(0, 0, FM_MTPS);
}

void encode_mfpd(int smode, int sreg, u16 src)
{
}

void encode_swab(int dmode, int dreg, u16 dest)
{
    /* S0 <- ss */
    if (dmode == 0) {
        m_push_isn(M_SWAB, dreg, dreg, 0, 0);
        m_flagmux(dreg, 0, FM_SWAB);
    } else {
        _encode_load_from_spec(R_S0, dmode, dreg, dest);
        m_push_isn(M_SWAB, R_S0, R_S0, 0, 0);
        _encode_store_result(R_S0, R_S1, dmode, dreg, dest);
        m_flagmux(R_S0, 0, FM_SWAB);
    }
}

void encode_asl(int dmode, int dreg, u16 dest)
{
    /* S0 <- ss */
    if (dmode == 0) {
        m_push_isn(M_SHIFTI, dreg, dreg, 0, 1);
        m_flagmux(dreg, dreg, FM_ASL);
    } else {
        _encode_load_from_spec(R_S0, dmode, dreg, dest);
        m_push_isn(M_SHIFTI, R_R0, R_S0, 0, 1);
        m_flagmux(R_S0, R_S0, FM_ASL);
    }
}

void encode_asr(int dmode, int dreg, u16 dst)
{
    if (debug) {
        printf("encode_asr: dd %o%o d=%o\n",
               dmode, dreg, dst);
    }

    /* S0 <- ss */
    if (dmode == 0) {
        m_asr(R_R0, dreg, -1);
        m_flagmux(R_R0, dreg, FM_ASR);
        m_load(dreg, R_R0);
    } else {
        _encode_load_from_spec(R_S0, dmode, dreg, dst);
        m_asr(R_R0, R_S0, -1);
        _encode_store_result(R_R0, R_S1, dmode, dreg, dst);
        m_flagmux(R_R0, R_S0, FM_ASR);
    }

}

void encode_tstb(int dmode, int dreg, u16 dest)
{
    /* S0 <- ss */
    _encode_load_from_spec_byte(R_S0, dmode, dreg, dest);
    m_load(R_R0, R_S0);
    m_flagmux(R_R0, R_S0, FM_TSTB);
}

void encode_tst(int dmode, int dreg, u16 dest)
{
    /* S0 <- ss */
    _encode_load_from_spec(R_S0, dmode, dreg, dest);
    m_load(R_R0, R_S0);
    m_flagmux(R_R0, R_S0, FM_TST);
}

void encode_rol(int dmode, int dreg, u16 dst)
{
    /* S0 <- ss */
    _encode_load_from_spec(R_S0, dmode, dreg, dst);
    m_push_isn(M_ROTATE, R_R0, R_S0, 0, 1);
    _encode_store_result(R_R0, R_S1, dmode, dreg, dst);
    m_flagmux(R_R0, R_S0, FM_ROL);
}

void encode_rolb(int dmode, int dreg, u16 dst)
{
    /* S0 <- ss */
    _encode_load_from_spec_byte(R_S0, dmode, dreg, dst);
    m_push_isn(M_ROTATEB, R_R0, R_S0, 0, 1);
    _encode_store_result_byte(R_R0, R_S1, dmode, dreg, dst);
    m_flagmux(R_R0, R_S0, FM_ROLB);
}

void encode_neg(int dmode, int dreg, u16 dst)
{
    if (debug) {
        printf("encode_neg: dd %o%o d=%o\n",
               dmode, dreg, dst);
    }

    /* S0 <- ss */
    _encode_load_from_spec(R_S0, dmode, dreg, dst);
    m_not(R_R0, R_S0);
    m_addi(R_R0, 1);
    _encode_store_result(R_R0, R_S1, dmode, dreg, dst);
    m_flagmux(R_R0, 0, FM_NEG);
}

void encode_negb(int dmode, int dreg, u16 dst)
{
    if (debug) {
        printf("encode_negb: dd %o%o d=%o\n",
               dmode, dreg, dst);
    }

    /* S0 <- ss */
    _encode_load_from_spec_byte(R_S0, dmode, dreg, dst);
    m_notb(R_R0, R_S0);
    m_addib(R_R0, 1);
    _encode_store_result_byte(R_R0, R_S1, dmode, dreg, dst);
    m_flagmux(R_R0, 0, FM_NEGB);
}

void encode_jmp(int dmode, int dreg, u16 dst)
{
    if (debug) {
        printf("encode_jmp: dd %o%o d=%o\n",
               dmode, dreg, dst);
    }

    /* S0 <- dd */
    _encode_ea_from_spec(R_S0, dmode, dreg, dst);
    m_push_isn(M_JMP, R_S1, 0, 0, 0);
}

void encode_jsr(int reg, int dmode, int dreg, u16 dst)
{
    _encode_ea_from_spec(R_S0, dmode, dreg, dst);

    /* push */
    m_subi(6, 2);
    m_storeind(6, reg);

    /* reg <- return */
    if (reg != 7)
        m_load(reg, 7);

    m_push_isn(M_JMP, R_S1, 0, 0, 0);
}


void encode_div(int reg, int smode, int sreg, u16 src)
{
    _encode_load_from_spec(R_S0, smode, sreg, src);

    m_push_isn(M_DIV, R_R0, reg, R_S0, 0);
    m_load(reg, R_R0);
    m_load(reg+1, R_R1);
    m_flagmux(R_R0, 0, FM_DIV);
}

void encode_mul(int reg, int smode, int sreg, u16 src)
{
    _encode_load_from_spec(R_S0, smode, sreg, src);
    m_load(R_R0, reg);
    m_push_isn(M_MUL, R_R0, R_S0, 0, 0);
    m_load(reg, R_R1);
    if ((reg & 1) == 0)
        m_load(reg+1, R_R0);
    m_flagmux(R_R0, 0, FM_MUL);
}

void encode_adc(int dmode, int dreg, u16 dst)
{
    if (debug) {
        printf("encode_adc: dd %o%o d=%o\n",
               dmode, dreg, dst);
    }

    /* S0 <- ss */
    _encode_load_from_spec(R_S0, dmode, dreg, dst);
    m_addc(R_S0, R_S0);
    _encode_store_result(R_S0, R_S1, dmode, dreg, dst);
    m_flagmux(R_S0, 0, FM_ADC);
}

void encode_adcb(int dmode, int dreg, u16 dst)
{
    if (debug) {
        printf("encode_adcb: dd %o%o d=%o\n",
               dmode, dreg, dst);
    }

    /* S0 <- ss */
    _encode_load_from_spec(R_S0, dmode, dreg, dst);
    m_addcb(R_R0, R_S0);
    _encode_store_result(R_R0, R_S1, dmode, dreg, dst);
    m_flagmux(R_R0, 0, FM_ADCB);
}

void encode_inc(int dmode, int dreg, u16 dst)
{
    if (debug) {
        printf("encode_inc: dd %o%o d=%o\n",
               dmode, dreg, dst);
    }

    /* S0 <- ss */
    if (dmode == 0) {
        m_addi(dreg, 1);
        m_flagmux(dreg, 0, FM_INC);
    } else {
        _encode_load_from_spec(R_S0, dmode, dreg, dst);
        m_addi(R_S0, 1);
        _encode_store_result(R_S0, R_S1, dmode, dreg, dst);
        m_flagmux(R_S0, 0, FM_INC);
    }

}

void encode_incb(int dmode, int dreg, u16 dst)
{
    if (debug) {
        printf("encode_incb: dd %o%o d=%o\n",
               dmode, dreg, dst);
    }

    /* S0 <- ss */
    if (dmode == 0) {
        m_addib(dreg, 1);
        m_flagmux(dreg, 0, FM_INCB);
    } else {
        _encode_load_from_spec_byte(R_S0, dmode, dreg, dst);
        m_addib(R_S0, 1);
        _encode_store_result_byte(R_S0, R_S1, dmode, dreg, dst);
        m_flagmux(R_S0, 0, FM_INCB);
    }
}

void encode_dec(int dmode, int dreg, u16 dst)
{
    if (debug) {
        printf("encode_dec: dd %o%o d=%o\n",
               dmode, dreg, dst);
    }

    /* S0 <- ss */
    if (dmode == 0) {
        m_subi(dreg, 1);
        m_flagmux(dreg, 0, FM_DEC);
    } else {
        _encode_load_from_spec(R_S0, dmode, dreg, dst);
        m_subi(R_S0, 1);
        _encode_store_result(R_S0, R_S1, dmode, dreg, dst);
        m_flagmux(R_S0, 0, FM_DEC);
    }
}

void encode_decb(int dmode, int dreg, u16 dst)
{
    if (debug) {
        printf("encode_decb: dd %o%o d=%o\n",
               dmode, dreg, dst);
    }

    /* S0 <- ss */
    if (dmode == 0) {
        m_subib(dreg, 1);
        m_flagmux(dreg, 0, FM_DECB);
    } else {
        _encode_load_from_spec_byte(R_S0, dmode, dreg, dst);
        m_subib(R_S0, 1);
        _encode_store_result_byte(R_S0, R_S1, dmode, dreg, dst);
        m_flagmux(R_S0, 0, FM_DECB);
    }
}

void encode_ror(int dmode, int dreg, u16 dst)
{
    if (debug) {
        printf("encode_ror: dd %o%o d=%o\n",
               dmode, dreg, dst);
    }

    /* S0 <- ss */
    if (dmode == 0) {
        m_ror(R_R0, dreg, -1);
        m_flagmux(R_R0, dreg, FM_ROR);
        m_load(dreg, R_R0);
    } else {
        _encode_load_from_spec(R_S0, dmode, dreg, dst);
        m_ror(R_R0, R_S0, -1);
        _encode_store_result(R_R0, R_S1, dmode, dreg, dst);
        m_flagmux(R_R0, R_S0, FM_ROR);
    }

}

void encode_com(int dmode, int dreg, u16 dst)
{
    if (debug) {
        printf("encode_com: dd %o%o d=%o\n",
               dmode, dreg, dst);
    }

    /* S0 <- ss */
    _encode_load_from_spec(R_S0, dmode, dreg, dst);
    m_not(R_R0, R_S0);
    _encode_store_result(R_R0, R_S1, dmode, dreg, dst);
    m_flagmux(R_R0, 0, FM_COM);
}

void encode_comb(int dmode, int dreg, u16 dst)
{
    if (debug) {
        printf("encode_comb: dd %o%o d=%o\n",
               dmode, dreg, dst);
    }

    /* S0 <- ss */
    _encode_load_from_spec_byte(R_S0, dmode, dreg, dst);
    m_notb(R_S0, R_S0);
    _encode_store_result_byte(R_S0, R_S1, dmode, dreg, dst);
    m_flagmux(R_S0, 0, FM_COMB);
}

encode_rorb() {}

void encode_ash(int reg, int smode, int sreg, u16 src)
{
    if (smode == 0) {
        m_push_isn(M_SHIFT, reg, reg, sreg, 0);
        m_flagmux(reg, sreg, FM_ASH);
    } else {
        _encode_load_from_spec(R_S0, smode, sreg, src);
        m_push_isn(M_SHIFT, reg, reg, R_S0, 0);
        m_flagmux(reg, R_S0, FM_ASH);
    }
}

void encode_ashc(int reg, int smode, int sreg, u16 src)
{
    if (smode == 0) {
        m_push_isn(M_SHIFT32, reg, reg, sreg, 0);
        m_flagmux(reg, sreg, FM_ASHC);
    } else {
        _encode_load_from_spec(R_S0, smode, sreg, src);
        m_push_isn(M_SHIFT32, reg, reg, R_S0, 0);
        m_flagmux(reg, R_S0, FM_ASHC);
    }
}

void encode_sxt(int dmode, int dreg, u16 dst)
{
    if (debug) {
        printf("encode_sxt: dd %o%o d=%o\n",
               dmode, dreg, dst);
    }

    /* S0 <- ss */
    _encode_load_from_spec(R_S0, dmode, dreg, dst);
    m_sxt(R_R0, R_S0);
    _encode_store_result(R_R0, R_S1, dmode, dreg, dst);
    m_flagmux(R_R0, 0, FM_SXT);
}


void encode_sbc(int dmode, int dreg, u16 dst)
{
    if (debug) {
        printf("encode_sbc: dd %o%o d=%o\n",
               dmode, dreg, dst);
    }

    /* S0 <- ss */
    _encode_load_from_spec(R_S0, dmode, dreg, dst);
    m_subc(R_S0, R_S0);
    _encode_store_result(R_S0, R_S1, dmode, dreg, dst);
    m_flagmux(R_S0, 0, FM_SBC);
}

void encode_xor(int reg, int dmode, int dreg, u16 dst)
{
    _encode_load_from_spec(R_S0, dmode, dreg, dst);
    m_load(R_R0, reg);
    m_push_isn(M_XOR, R_R0, R_S0, 0, 0);
    _encode_store_result(R_R0, R_S0, dmode, dreg, dst);
    m_flagmux(R_R0, 0, FM_XOR);
}


void encode_mfps(int dmode, int dreg, u16 dst)
{
    m_storepsw(R_S0);
    _encode_ea_from_spec_byte(R_S0, dmode, dreg, dst);
    _encode_store_result_byte(R_S0, R_S1, dmode, dreg, dst);
    if (dmode == 0) {
        m_storeb(dreg, dreg);
    }
    m_flagmux(R_S0, 0, FM_MFPS);
}

encode_mtpd() {}
encode_csm() {}
encode_sbcb() {}
encode_arsb() {}
encode_aslb() {}


void encode_exception(u8 vector)
{
    /*
     * load new pc [kernel mode]
     * load new-psw [kernel mode]
     * psw <- new-psw
     * push psw [mode from new-psw]
     * push pc [mode from new-psw]
     */

    /* save old-psw */
    m_storepsw(R_S1);

    /* force psw to kernel mode */
    m_loadi(R_R1, 0340);
    m_loadpsw(R_R1, 1);

    /* load new-pc [kernel mode] */
    m_loadi(R_S0, vector);
    m_loadind(R_R0, R_S0);

    /* load new-psw [kernel mode] */
    m_addi(R_S0, 2);
    m_loadind(R_R1, R_S0);
    m_loadpsw(R_S1, 0);
    m_loadpsw(R_R1, 1);

    /* push old-psw [mode from new psw] */
    m_subi(6, 2);
    m_storeind(6, R_S1);

    /* push pc [mode from new psw] */
    m_subi(6, 2);
    m_storeind(6, 7);

if (vector != 4) m_checksp(0);

    /* jump to new-pc */
    m_push_isn(M_JMP, R_R0, 0, 0, 0);
}

void encode_trap(int vector, int op)
{
//    /* check stack before trap exception */
//    m_checksp(2); 

    encode_exception(vector);
}

void encode_mfpt(void)
{
    /* on 11/34, trap 10 */
    encode_exception(010);
}



/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
