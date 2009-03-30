/* run.c */

#include <stdio.h>
#include "cpu.h"
#include "debug.h"

int verbose;
int boot;

#define SIMH_COSIM

u16 test1[] = {
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

u16 test2[] = {
    0012706,
    0000500,
    0012701,
    0000700,
    0012702,
    0000712,
    0005201,
    0005201,
    0005211,
    0005221,
    0005231,
    0005241,
    0005251,
    0005261,
    0000002,
    0005271,
    0000004,
    0000000,
};

u16 test3[] = {
    0012706,
    0000500,
    0012701,
    0000200,
    0012702,
    0000300,
    0011102,
    0011112,
    0011122,
    0011132,
    0011142,
    0011152,
    0011162,
    0000002,
    0011172,
    0000004,
    0000000,
};

u16 test4[] = {
    0012706,
    0000500,
    0012701,
    0000200,
    0012702,
    0000300,
    0016102,
    0000002,
    0016112,
    0000002,
    0016122,
    0000002,
    0016132,
    0000002,
    0016142,
    0000002,
    0016152,
    0000002,
    0016162,
    0000002,
    0000004,
    0016172,
    0000004,
    0000000,
};

u16 test5[] = {
    0012706,
    0000500,
    0012701,
    0000200,
    0012702,
    0000300,
    0017102,
    0000002,
    0017112,
    0000002,
    0017122,
    0000002,
    0017132,
    0000002,
    0017142,
    0000002,
    0017152,
    0000002,
    0017162,
    0000002,
    0000004,
    0017172,
    0000006,
    0000000,
};

u16 test6[] = {
    0012706,
    0000500,
    0012701,
    0000200,
    0012702,
    0000300,
    0017102,
    0000002,
    0017112,
    0000002,
    0017122,
    0000002,
    0017132,
    0000002,
    0017142,
    0000002,
    0017152,
    0000002,
    0017162,
    0000002,
    0000004,
    0017172,
    0000004,
    0000000,
};

u16 test7[] = {
    0012706,
    0000500,
    0012701,
    0000200,
    0012702,
    0000300,
    0016702,
    0000002,
    0016712,
    0000002,
    0016722,
    0000002,
    0016732,
    0000002,
    0016742,
    0000002,
    0016752,
    0000002,
    0016762,
    0000002,
    0000004,
    0016772,
    0000004,
    0000000,
};

u16 test8[] = {
    0012706,
    0000500,
    0012701,
    0000200,
    0012702,
    0000300,
//#if 0
    0016707,
    0000036,
    0016717,
    0000034,
//#endif
    0016727,
    0000002,
    0000004,
//#if 0
    0016737,
    0000002,
    0016747,
    0000002,
    0016757,
    0000002,
    0016767,
    0000002,
    0016777,
    0000004,
    0000000,
    0000520,
    0016727,
//#endif
    0000000,
    0000000,
};

u16 test9[] = {
    0012706,
    0000500,
    0012701,
    0000200,
    0012702,
    0000300,
    0020102,
    0021111,
    0022121,
    0023131,
    0005001,
    0005011,
    0005021,
    0005031,
    0005041,
    0005051,
};

u16 test10[] = {
    000500, 0012706,
    000502, 0000500,
    000504, 0012701,
    000506, 0000700,
//    000510, 0012702,
//    000512, 0000712,
  000510, 0005061,
  000512, 0000100,
    000514, 0004067,
    000516, 0000002,
    000520, 0000000,

    000522, 0010146,
    000524, 0010246,
    000526, 0012602,
    000530, 0012601,
    000532, 0000200,
    000534, 0000000,
};

u16 test11[] = {
    000034, 0000200,
    000036, 0000007,

    000200, 0012706,
    000202, 0000200,
    000204, 0000000,

    000500, 0012706,
    000502, 0000500,
    000504, 0104777,
    000506, 0000000,
};

u16 boot_code[] = {
    0042113,                        /* "KD" */
    0012706, 0002000,               /* MOV #boot_start, SP */
    0012700, 0000000,               /* MOV #unit, R0        ; unit number */
    0010003,                        /* MOV R0, R3 */
    0000303,                        /* SWAB R3 */
    0006303,                        /* ASL R3 */
    0006303,                        /* ASL R3 */
    0006303,                        /* ASL R3 */
    0006303,                        /* ASL R3 */
    0006303,                        /* ASL R3 */
    0012701, 0177412,               /* MOV #RKDA, R1        ; csr */
    0010311,                        /* MOV R3, (R1)         ; load da */
    0005041,                        /* CLR -(R1)            ; clear ba */
    0012741, 0177000,               /* MOV #-256.*2, -(R1)  ; load wc */
    0012741, 0000005,               /* MOV #READ+GO, -(R1)  ; read & go */
    0005002,                        /* CLR R2 */
    0005003,                        /* CLR R3 */
    0012704, 0002000+020,           /* MOV #START+20, R4 */
    0005005,                        /* CLR R5 */
    0105711,                        /* TSTB (R1) */
    0100376,                        /* BPL .-2 */
    0105011,                        /* CLRB (R1) */
    0005007,                        /* CLR PC */
    0xffff
};

struct {
    int num;
    int ttype;
    u16 *code;
    int size;
    unsigned int start;
} tests[] = {
    { 1, 1, test1, sizeof(test1), 0500 },
    { 2, 2, test2, sizeof(test2), 0500 },
    { 3, 2, test3, sizeof(test3), 0500 },
    { 4, 2, test4, sizeof(test4), 0500 },
    { 5, 2, test5, sizeof(test5), 0500 },
    { 6, 2, test6, sizeof(test6), 0500 },
    { 7, 2, test7, sizeof(test7), 0500 },
    { 8, 2, test8, sizeof(test8), 0500 },
    { 9, 2, test9, sizeof(test9), 0500 },
    { 10, 1, test10, sizeof(test10), 0500 },
    { 11, 1, test11, sizeof(test11), 0500 },
    { 0, 0, NULL }
};

void
write_test_code(int addr, unsigned code)
{
    if (0) printf("%o @ %o\n", code, addr);

    raw_write_memory(addr, code);
#ifdef SIMH_COSIM
    simh_write_mem(addr, code);
#endif
}

void
fill_test_code(int testnum)
{
    int i, j;
    unsigned short pc;
    unsigned int addr;

    if (!boot) {
        for (i = 0; tests[i].ttype; i++) {
            if (tests[i].num == testnum) {
                u16 *cp = tests[i].code;

                if (0) printf("index %d, type %d\n", i, tests[i].ttype);
                switch (tests[i].ttype) {
                case 1:
                    for (j = 0; cp[j]; j += 2) {
                        write_test_code(cp[j], cp[j+1]);
                    }
                    break;
                case 2:
                    addr = 0500;
                    for (j = 0; j < tests[i].size; j++) {
                        write_test_code(addr, cp[j]);
                        addr += 2;
                    }
                    break;
                }

                pc = tests[i].start;
                break;
            }
        }

        printf("test %d, start %o\n", tests[i].num, pc);
    }

    if (boot) {
        addr = 02000;
        for (i = 0; boot_code[i] != 0xffff; i++) {
            raw_write_memory(addr, boot_code[i]);
#ifdef SIMH_COSIM
            simh_write_mem(addr, boot_code[i]);
#endif
            addr += 2;
        }

        pc = 02002;
    }

    debug_reset();
    debug_set_pc(pc);

#ifdef SIMH_COSIM
    simh_set_pc(pc);

    for (i = 0; i < 8; i++) {
        extern u16 regs[8];
        simh_write_reg(i, regs[i]);
    }
#endif
}

#ifdef SIMH_COSIM
void
cosim_check()
{
    extern u16 regs[8];
    u16 simh_regs[8];
    int i;
    unsigned short spsw;
    extern u16 psw;

    V_SIMH printf("simh step\n");

    simh_step();

    simh_read_psw(&spsw);

//#define PSW_CHECK_MASK 0xf
#define PSW_CHECK_MASK 0xffff

    if ((psw & PSW_CHECK_MASK) != (spsw & PSW_CHECK_MASK)) {
        printf("psw disagrees: rtl %o, simh %o\n", psw, spsw);
    }

    for (i = 0; i < 8; i++) {
        simh_read_reg(i, &simh_regs[i]);
        if (regs[i] != simh_regs[i])
            printf("R%d disagree; rtl %06o simh %06o\n",
                   i, regs[i], simh_regs[i]);
    }

}

void cosim_trap(void)
{
    int i;
    unsigned short pc;
    extern u16 regs[8];
    int steps = 0;

    simh_read_reg(7, &pc);
    V_SIMH printf("simh step; rtl @ %o, simh @ %o\n", regs[7], pc);

    for (i = 0; i < 10; i++) {
        steps++;
        simh_step();
        simh_read_reg(7, &pc);
        if (pc == regs[7]) {
            V_SIMH printf("simh: traps sync after %d steps\n", steps);
            break;
        }
    }

    if (i == 10) {
        printf("simh: trap out of sync\n");
    }
}

void
cosim_setup(void)
{
	simh_init();
//	simh_command("set cpu 11/44");
	simh_command("set cpu 11/34");
	simh_command("set cpu 256k");
        simh_command("att rk0 rk.dsk");
}
#else
void cosim_setup(void) {}
void cosim_check(void) {}
void cosim_trap(void) {}
#endif


extern char *optarg;

main(int argc, char *argv[])
{
    int c;
    int test_no;

    test_no = 1;
    verbose = 0xffff;

    while ((c = getopt(argc, argv, "bv:t:")) != -1) {
        switch (c) {
        case 'b':
            boot = 1;
            break;
        case 't':
            test_no = atoi(optarg);
            break;
        case 'v':
            verbose = atoi(optarg);
            break;
        }
    }

    make_isn_table();
    mem_init();
    cosim_setup();
    fill_test_code(test_no);
    reset_support();
    run();

    printf("done!\n");
}



/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
