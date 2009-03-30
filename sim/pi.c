/*
 * pi.c
 */

#include <stdio.h>
#include <stdlib.h>

typedef unsigned char u_char;
typedef unsigned short u_short;

#include "isn.h"

extern u_short isn_dispatch[0x10000];
extern raw_isn_t *isn_decode[0x10000];
extern raw_isn_t raw_isns[];

int fetch;
int halted;

u_char cc_c, cc_n, cc_z, cc_v;

u_short regs[8];
#define pc (regs[7])

u_short memory[4 * 1024 * 1024];

void
execute(u_short op)
{
    int code, dispatch, rt;
    int smode, dmode;
    int n, reg, sreg, dreg;
    int regv, dregv;
    u_short *src, *dest;
    int regdelta, off, d;
    int set_cc;

    code = isn_dispatch[op];

    if (!(code & 0x8000)) {
        printf("illegal opcode; pc %o, opcode %o\n", pc-2, op);
        halted = 1;
        return;
    }

    regdelta = 2;
    if (code & 0x4000)
        regdelta = 1;

    dispatch = code & 0xff;
    rt = (code >> 8) & 0xf;
    
    n = -1;
    reg = -1;
    smode = -1;
    dmode = -1;
    sreg = -1;
    sreg = -1;

    regv = -1;
    dregv = -1;

    src = (void *)0;
    dest = (void *)0;
    set_cc = 0;

    {
        int j;

        for (j = 0; raw_isns[j].isn_num >= 0; j++)
            if (raw_isns[j].isn_num == dispatch) {
                printf("pc %o %s\n", pc-2, raw_isns[j].isn_name);
                break;
            }
    }

    switch (rt) {
        case R_NONE:
            break;
        case R_8OFF:
            off = (char)(op & 0xff)*2;
            break;
        case R_R:
            reg = op & 7;
            regv = regs[reg];
            break;
        case R_N:
            n = op & 7;
            break;
        case R_R8OFF:
            reg = op & 7;
            regv = regs[reg];
            off = memory[ pc / 2 ] & 077;
            pc += 2;

            if (off & 0x20)
                off |= 0xc0;

            regv += (int)(char)off;
            break;
        case R_DD:
            dmode = (op >> 3) & 7;
            dreg = (op >> 0) & 7;
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
            break;
        case R_NN:
            n = op & 077;
            break;
        case R_SS:
            smode = (op >> 3) & 7;
            sreg = (op >> 0) & 7;
            break;
        case R_RSS:
            reg = (op >> 6) & 7;
            smode = (op >> 3) & 7;
            sreg = (op >> 0) & 7;
            break;
        case R_RDD:
            reg = (op >> 6) & 7;
            dmode = (op >> 3) & 7;
            dreg = (op >> 0) & 7;
            break;
        case R_SSDD:
            smode = (op >> 9) & 7;
            sreg = (op >> 6) & 7;
            dmode = (op >> 3) & 7;
            dreg = (op >> 0) & 7;

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

            break;
    }

    {
        if (smode != -1) printf("smode %o, sreg %o\n", smode, sreg);
        if (dmode != -1) printf("dmode %o, dreg %o\n", dmode, dreg);
    }

    switch (dispatch) {
    case ISN_HALT:
        halted = 1;
        break;
    case ISN_WAIT:
        break;
    case ISN_RTI:
        break;
    case ISN_BPT:
        break;
    case ISN_IOT:
        break;
    case ISN_RESET:
        break;
    case ISN_RTT:
        break;
    case ISN_MFPT:
        break;
    case ISN_JMP:
        break;
    case ISN_RTS:
        break;
    case ISN_SPL:
        break;
    case ISN_C:
        break;
    case ISN_CLC:
        break;
    case ISN_CLV:
        break;
    case ISN_CLZ:
        break;
    case ISN_CLN:
        break;
    case ISN_CCC:
        break;
    case ISN_S:
        break;
    case ISN_SEC:
        break;
    case ISN_SEV:
        break;
    case ISN_SEZ:
        break;
    case ISN_SEN:
        break;
    case ISN_SCC:
        break;
    case ISN_SWAB:
        break;
    case ISN_BR:
        break;
    case ISN_BNE:
        if (cc_z == 0) {
            printf("take branch offset %d(10)\n", off);
            pc += off;
        }
        break;
    case ISN_BEQ:
        break;
    case ISN_BGE:
        break;
    case ISN_BLT:
        break;
    case ISN_BGT:
        break;
    case ISN_BLE:
        break;
    case ISN_JSR:
        break;
    case ISN_CLR:
        *dest = 0;
        break;
    case ISN_COM:
        break;
    case ISN_INC:
        break;
    case ISN_DEC:
        break;
    case ISN_NEG:
        break;
    case ISN_ADC:
        *dest = *dest + cc_c;
        break;
    case ISN_SBC:
        break;
    case ISN_TST:
        break;
    case ISN_ROR:
        break;
    case ISN_ROL:
        break;
    case ISN_ASR:
        break;
    case ISN_ASL:
        break;
    case ISN_MARK:
        break;
    case ISN_MFPI:
        break;
    case ISN_MTPI:
        break;
    case ISN_SXT:
        break;
    case ISN_CSM:
        break;
    case ISN_MOV:
        *dest = *src;
        break;
    case ISN_CMP:
        d = *dest + ~*src + 1;
        printf("src %o, dest %o, d 0x%x\n", *src, *dest, d);

        if ( ((*src >> 15) & 1) == ((*dest >> 15) & 1) &&
             ((*src >> 15) & 1) != (d >> 15) & 1) cc_v = 1;
        else cc_v = 0;

        if (d >> 16) cc_c = 1; else cc_c = 0;
        if ((short)d < 0) cc_n = 1; else cc_n = 0;
        if ((short)d == 0) cc_z = 1; else cc_z = 0;
        break;
    case ISN_BIT:
        break;
    case ISN_BIC:
        break;
    case ISN_BIS:
        break;
    case ISN_ADD:
        printf("add\n");
        d = *dest + *src;

        if ( ((*src >> 15) & 1) == ((*dest >> 15) & 1) &&
             ((*src >> 15) & 1) != (d >> 15) & 1) cc_v = 1;
        else cc_v = 0;
        if (d >> 16) cc_c = 1; else cc_c = 0;
        if ((short)d < 0) cc_n = 1; else cc_n = 0;
        if ((short)d == 0) cc_z = 1; else cc_z = 0;

        *dest += *src;
        break;
    case ISN_MUL:
        break;
    case ISN_DIV:
        break;
    case ISN_ASH:
        break;
    case ISN_ASHC:
        break;
    case ISN_XOR:
        break;
    case ISN_SOB:
        break;
    case ISN_BPL:
        break;
    case ISN_BMI:
        break;
    case ISN_BHI:
        break;
    case ISN_BLOS:
        break;
    case ISN_BVC:
        break;
    case ISN_BVS:
        break;
    case ISN_BCC:
        break;
    case ISN_BHIS:
        break;
    case ISN_BCS:
        break;
    case ISN_BLO:
        break;
    case ISN_EMT:
        break;
    case ISN_TRAP:
        break;
    case ISN_CLRB:
        break;
    case ISN_COMB:
        break;
    case ISN_INCB:
        break;
    case ISN_DECB:
        break;
    case ISN_NEGB:
        break;
    case ISN_ADCB:
        break;
    case ISN_SBCB:
        break;
    case ISN_TSTB:
        break;
    case ISN_RORB:
        break;
    case ISN_ROLB:
        break;
    case ISN_ASRB:
        break;
    case ISN_ASLB:
        break;
    case ISN_MTPS:
        break;
    case ISN_MFPD:
        break;
    case ISN_MTPD:
        break;
    case ISN_MFPS:
        break;
    case ISN_MOVB:
        break;
    case ISN_CPMB:
        break;
    case ISN_BITB:
        break;
    case ISN_BICB:
        break;
    case ISN_BISB:
        break;
    case ISN_SUB:
        printf("sub\n");
        d = *dest - *src;

        if ( ((*src >> 15) & 1) == ((*dest >> 15) & 1) &&
             ((*src >> 15) & 1) != (d >> 15) & 1) cc_v = 1;
        else cc_v = 0;
        if (d >> 16) cc_c = 1; else cc_c = 0;
        if ((short)d < 0) cc_n = 1; else cc_n = 0;
        if ((short)d == 0) cc_z = 1; else cc_z = 0;

        *dest -= *src;
        break;
    }

    printf("R0 %06o R1 %06o R2 %06o R3 %06o\n",
           regs[0], regs[1], regs[2], regs[3]);
    printf("R4 %06o R5 %06o R6 %06o R7 %06o\n",
           regs[4], regs[5], regs[6], regs[7]);
    printf("C%d N%d Z%d V%d\n",
           cc_c, cc_n, cc_z, cc_v);

}

void
run(void)
{
    while (!halted) {
        fetch = memory[pc/2];
        if (0) printf("fetch %o %o\n", pc, fetch);
        pc += 2;
        execute(fetch);
    }

    if (halted) {
        printf("halted, pc %o\n", pc);
    }
}

u_short test_code[] = {
    000500, 0012706,
    000502, 0000500,
    000504, 0012701,
    000506, 0000700,
    000510, 0012702,
    000512, 0000712,
    000514, 0012703,
    000516, 0001000,
    000520, 0012704,
    000522, 0001012,
    000524, 0005000,
    000526, 0005005,
    000530, 0062105,
    000532, 0020102,
    000534, 0001375,
    000536, 0062300,
    000540, 0020304,
    000542, 0001375,
    000544, 0160500,
    000546, 0000000,

    000700, 0000001,
    000702, 0000002,
    000704, 0000003,
    000706, 0000004,
    000710, 0000005,

    001000, 0000004,
    001002, 0000005,
    001004, 0000006,
    001006, 0000007,
    001010, 0000010,
    0, 0
};

void
fill_test_code(void)
{
    int i;
    for (i = 0; test_code[i]; i += 2) {
        memory[ test_code[i]/2 ] = test_code[i+1];
    }
    pc = 0500;

    for (i = 0; i < 7; i++)
        regs[i] = -1;
}

extern int optind;
extern char *optarg;

main(int argc, char *argv[])
{
    int c;

    while ((c = getopt(argc, argv, "d")) != -1) {
        switch (c) {
        case 'd':
		break;
	}
    }

    make_isn_table();
    fill_test_code();
    run();
    exit(0);
}


/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
