/*
 * pdp11.c
 * a very simple pdp-11 disassembler
 * 
 * brad@heeltoe.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

typedef unsigned short u16;
typedef unsigned char u8;

#include "pdp11_isn.h"

static inited = 0;

static raw_isn_t *isn_decode[0x10000];

static raw_isn_t raw_isns[] = {
    { ISN_HALT, "HALT", I_MS, R_NONE, 0000000, 0 },
    { ISN_WAIT, "WAIT", I_MS, R_NONE, 0000001, 0 },
    { ISN_RTI, "RTI", I_MS, R_NONE, 0000002, 0 },
    { ISN_BPT, "BPT", I_PC, R_NONE, 0000003, 0 },
    { ISN_IOT, "IOT", I_PC, R_NONE, 0000004, 0 },
    { ISN_RESET, "RESET", I_MS, R_NONE, 0000005, 0 },
    { ISN_RTT, "RTT", I_MS, R_NONE, 0000006, 0 },
    { ISN_MFPT, "MFPT", I_MS, R_NONE, 0000007, 0 },
    { ISN_JMP, "JMP", I_PC, R_DD, 0000100, 0 },
    { ISN_RTS, "RTS", I_PC, R_R, 0000200, 0 },
    { ISN_SPL, "SPL", I_PC, R_N, 0000230, 0 },
    { ISN_C, "C", I_CC, R_NONE, 0000240, 0 },
    { ISN_CLC, "CLC", I_CC, R_NONE, 0000241, 0 },
    { ISN_CLV, "CLV", I_CC, R_NONE, 0000242, 0 },
    { ISN_CLZ, "CLZ", I_CC, R_NONE, 0000244, 0 },
    { ISN_CLN, "CLN", I_CC, R_NONE, 0000250, 0 },
    { ISN_CCC, "CCC", I_CC, R_NONE, 0000257, 0 },
    { ISN_S, "S", I_CC, R_NONE, 0000260, 0 },
    { ISN_SEC, "SEC", I_CC, R_NONE, 0000261, 0 },
    { ISN_SEV, "SEV", I_CC, R_NONE, 0000262, 0 },
    { ISN_SEZ, "SEZ", I_CC, R_NONE, 0000264, 0 },
    { ISN_SEN, "SEN", I_CC, R_NONE, 0000270, 0 },
    { ISN_SCC, "SCC", I_CC, R_NONE, 0000277, 0 },
    { ISN_SWAB, "SWAB", I_SO, R_DD, 0000300, 0 },
    { ISN_BR, "BR", I_PC, R_8OFF, 0000400, 0 },
    { ISN_BNE, "BNE", I_PC, R_8OFF, 0001000, 0 },
    { ISN_BEQ, "BEQ", I_PC, R_8OFF, 0001400, 0 },
    { ISN_BGE, "BGE", I_PC, R_8OFF, 0002000, 0 },
    { ISN_BLT, "BLT", I_PC, R_8OFF, 0002400, 0 },
    { ISN_BGT, "BGT", I_PC, R_8OFF, 0003000, 0 },
    { ISN_BLE, "BLE", I_PC, R_8OFF, 0003400, 0 },
    { ISN_JSR, "JSR", I_PC, R_RDD, 0004000, 0 },
    { ISN_CLR, "CLR", I_SO, R_DD, 0005000, 0 },
    { ISN_COM, "COM", I_SO, R_DD, 0005100, 0 },
    { ISN_INC, "INC", I_SO, R_DD, 0005200, 0 },
    { ISN_DEC, "DEC", I_SO, R_DD, 0005300, 0 },
    { ISN_NEG, "NEG", I_SO, R_DD, 0005400, 0 },
    { ISN_ADC, "ADC", I_SO, R_DD, 0005500, 0 },
    { ISN_SBC, "SBC", I_SO, R_DD, 0005600, 0 },
    { ISN_TST, "TST", I_SO, R_DD, 0005700, 0 },
    { ISN_ROR, "ROR", I_DO, R_DD, 0006000, 0 },
    { ISN_ROL, "ROL", I_DO, R_DD, 0006100, 0 },
    { ISN_ASR, "ASR", I_SO, R_DD, 0006200, 0 },
    { ISN_ASL, "ASL", I_SO, R_DD, 0006300, 0 },
    { ISN_MARK, "MARK", I_PC, R_NN, 0006400, 0 },
    { ISN_MFPI, "MFPI", I_MS, R_SS, 0006500, 0 },
    { ISN_MTPI, "MTPI", I_MS, R_DD, 0006600, 0 },
    { ISN_SXT, "SXT", I_SO, R_DD, 0006700, 0 },
    { ISN_CSM, "CSM", I_PC, R_DD, 0007000, 0 },
    { ISN_MOV, "MOV", I_DO, R_SSDD, 0010000, 0 },
    { ISN_CMP, "CMP", I_DO, R_SSDD, 0020000, 0 },
    { ISN_BIT, "BIT", I_DO, R_SSDD, 0030000, 0 },
    { ISN_BIC, "BIC", I_DO, R_SSDD, 0040000, 0 },
    { ISN_BIS, "BIS", I_DO, R_SSDD, 0050000, 0 },
    { ISN_ADD, "ADD", I_DO, R_SSDD, 0060000, 0 },
    { ISN_MUL, "MUL", I_DO, R_RSS, 0070000, 0 },
    { ISN_DIV, "DIV", I_DO, R_RSS, 0071000, 0 },
    { ISN_ASH, "ASH", I_DO, R_RSS, 0072000, 0 },
    { ISN_ASHC, "ASHC", I_DO, R_RSS, 0073000, 0 },
    { ISN_XOR, "XOR", I_DO, R_RDD, 0074000, 0 },
    { ISN_SOB, "SOB", I_PC, R_R6OFF, 0077000, 0 },
    { ISN_BPL, "BPL", I_PC, R_8OFF, 0100000, 0 },
    { ISN_BMI, "BMI", I_PC, R_8OFF, 0100400, 0 },
    { ISN_BHI, "BHI", I_PC, R_8OFF, 0101000, 0 },
    { ISN_BLOS, "BLOS", I_PC, R_8OFF, 0101400, 0 },
    { ISN_BVC, "BVC", I_PC, R_8OFF, 0102000, 0 },
    { ISN_BVS, "BVS", I_PC, R_8OFF, 0102400, 0 },
    { ISN_BCC, "BCC", I_PC, R_8OFF, 0103000, 0 },
    { ISN_BHIS, "BHIS", I_PC, R_8OFF, 0103000, 0 },
    { ISN_BCS, "BCS", I_PC, R_8OFF, 0103400, 0 },
    { ISN_BLO, "BLO", I_PC, R_8OFF, 0103400, 0 },
    { ISN_EMT, "EMT", I_PC, R_NONE, 0104000, 0104377 },
    { ISN_TRAP, "TRAP", I_PC, R_NONE, 0104400, 0104777 },
    { ISN_CLRB, "CLRB", I_SOB, R_DD, 0105000, 0 },
    { ISN_COMB, "COMB", I_SOB, R_DD, 0105100, 0 },
    { ISN_INCB, "INCB", I_SOB, R_DD, 0105200, 0 },
    { ISN_DECB, "DECB", I_SOB, R_DD, 0105300, 0 },
    { ISN_NEGB, "NEGB", I_SOB, R_DD, 0105400, 0 },
    { ISN_ADCB, "ADCB", I_SOB, R_DD, 0105500, 0 },
    { ISN_SBCB, "SBCB", I_SOB, R_DD, 0105600, 0 },
    { ISN_TSTB, "TSTB", I_SOB, R_DD, 0105700, 0 },
    { ISN_RORB, "RORB", I_DOB, R_DD, 0106000, 0 },
    { ISN_ROLB, "ROLB", I_DOB, R_DD, 0106100, 0 },
    { ISN_ASRB, "ASRB", I_SOB, R_DD, 0106200, 0 },
    { ISN_ASLB, "ASLB", I_SOB, R_DD, 0106300, 0 },
    { ISN_MTPS, "MTPS", I_MS, R_SS, 0106400, 0 },
    { ISN_MFPD, "MFPD", I_MS, R_SS, 0106500, 0 },
    { ISN_MTPD, "MTPD", I_MS, R_DD, 0106600, 0 },
    { ISN_MFPS, "MFPS", I_MS, R_DD, 0106700, 0 },
    { ISN_MOVB, "MOVB", I_DOB, R_SSDD, 0110000, 0 },
    { ISN_CPMB, "CPMB", I_DOB, R_SSDD, 0120000, 0 },
    { ISN_BITB, "BITB", I_DOB, R_SSDD, 0130000, 0 },
    { ISN_BICB, "BICB", I_DOB, R_SSDD, 0140000, 0 },
    { ISN_BISB, "BISB", I_DOB, R_SSDD, 0150000, 0 },
    { ISN_SUB, "SUB", I_DO, R_SSDD, 0160000, 0 },
    { -1, (char *)0, 0, 0, 0, 0 }
};

void
fmt_reg_mode(char *buf, int m, int r, int arg)
{
    char rn[10];

    if (r < 6)
        sprintf(rn, "r%d", r);
    else
        if (r == 6)
            strcpy(rn, "sp");
        else
            strcpy(rn, "pc");

    switch (m & 7) {
    case M_REG:
        strcpy(buf, rn);
        break;
    case M_REG_DEF:
        sprintf(buf, "@%s", rn);
        break;
    case M_AUTOINC:
//        if (r < 7)
            sprintf(buf, "(%s)+", rn);
//        else
//            sprintf(buf, "0%o", arg);
        break;
    case M_AUTOINC_DEF:
        sprintf(buf, "@(%s)+", rn);
        break;
    case M_AUTODEC:
        sprintf(buf, "-(%s)", rn);
        break;
    case M_AUTODEC_DEF:
        sprintf(buf, "@-(%s)", rn);
        break;
    case M_INDEXED:
//        if (r < 7)
            sprintf(buf, "%d.(%s)", arg, rn);
//        else
//            sprintf(buf, "0%o", arg);
        break;
    case M_INDEX_DEF:
        sprintf(buf, "@%d.(%s)", arg, rn);
        break;
    }
}

static void
make_isn_table(void)
{
    int i, num, o, op, op2, code;

    for (i = 0; raw_isns[i].isn_num >= 0; i++) {
        num = raw_isns[i].isn_num;
        op = raw_isns[i].isn_opcode;
        op2 = raw_isns[i].isn_opcode2;

        code = 0x8000 | (raw_isns[i].isn_regs << 8) | num;

        if (raw_isns[i].isn_type == I_SOB ||
            raw_isns[i].isn_type == I_DOB)
            code |= 0x4000;

        switch (raw_isns[i].isn_regs) {
        case R_NONE:
            isn_decode[op] = &raw_isns[i];

            if (op2) {
                for (o = op; o <= op2; o++) {
                    isn_decode[o] = &raw_isns[i];
                }
            }
            break;
        case R_R:
        case R_N:
        case R_R6OFF:
            for (o = op; o < op + 01000; o++) {
                isn_decode[o] = &raw_isns[i];
            }
            break;
        case R_DD:
        case R_NN:
        case R_SS:
            for (o = op; o < op + 0100; o++) {
                isn_decode[o] = &raw_isns[i];
            }
            break;
        case R_8OFF:
            for (o = op; o < op + 0400; o++) {
                isn_decode[o] = &raw_isns[i];
            }
            break;
        case R_RSS:
        case R_RDD:
            for (o = op; o < op + 01000; o++) {
                isn_decode[o] = &raw_isns[i];
            }
            break;
        case R_SSDD:
            for (o = op; o < op + 010000; o++) {
                isn_decode[o] = &raw_isns[i];
            }
            break;
        }
    }
}

void pdp11_dis(u16 inst, u16 arg1, u16 arg2, char *str)
{
    raw_isn_t *ri;
    char sm, sr, dm, dr, rr;
    int off, ref, refset;

    if (!inited) {
        make_isn_table();
        inited++;
    }

    str[0] = 0;
    ri = isn_decode[inst];

    if (ri) {

        sm = -1;
        dm = -1;
        sr = -1;
        dr = -1;

        refset = 0;
        ref = 0;

        switch (ri->isn_regs) {
        case R_NONE:
            break;
        case R_DD:
            dm = (inst >> 3) & 07; /* 00070 */
            dr = (inst >> 0) & 07; /* 00007 */
            arg2 = arg1;
            break;
        case R_SS:
            sm = (inst >> 3) & 07; /* 00070 */
            sr = (inst >> 0) & 07; /* 00007 */
            break;
        case R_SSDD:
            sm = (inst >> 9) & 07; /* 07000 */
            sr = (inst >> 6) & 07; /* 00700 */
            dm = (inst >> 3) & 07; /* 00070 */
            dr = (inst >> 0) & 07; /* 00007 */
            break;
        case R_R:
            break;
        case R_RSS:
            rr = (inst >> 6) & 07; /* 00700 */
            sm = (inst >> 3) & 07; /* 00070 */
            sr = (inst >> 0) & 07; /* 00007 */
            break;
        case R_RDD:
            rr = (inst >> 6) & 07; /* 00700 */
            dm = (inst >> 3) & 07; /* 00070 */
            dr = (inst >> 0) & 07; /* 00007 */
            arg2 = arg1;
            break;
        case R_N:
        case R_NN:
            break;
        case R_8OFF:
            off = (signed char)(inst & 0377) * 2;
            ref = off + 2;
            refset++;
            break;
        case R_R6OFF:
            rr = (inst >> 6) & 07; /* 00700 */
            off = (signed char)(inst & 077) * 2;
            ref = -(off + 2);
            refset++;
            break;
        }

        sprintf(str, "%06o %s", inst, ri->isn_name);

        if (ri->isn_regs == R_RSS ||
            ri->isn_regs == R_RDD ||
            ri->isn_regs == R_R6OFF)
        {
            char b[64];
            sprintf(b, " r%o,", rr);
            strcat(str, b);
        }

        if (refset) {
            char b[64];
            sprintf(b, "%c0%o", ref >= 0 ? '+' : '-', ref < 0 ? -ref : ref);
            strcat(str, " ");
            strcat(str, b);
        }

        if (sr >= 0) {
            char b[64];
            fmt_reg_mode(b, sm, sr, arg1);
            strcat(str, " ");
            strcat(str, b);
        }

        if (dr >= 0) {
            char b[64];
            fmt_reg_mode(b, dm, dr, arg2);
            if (sr >= 0)
                strcat(str, ",");

            strcat(str, " ");
            strcat(str, b);
        }
    }
    else
        sprintf(str, "%06o ?", inst);
}



/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/

