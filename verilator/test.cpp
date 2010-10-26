// test.cpp
//
//

#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Vtest_top.h"

#include <iostream>

Vtest_top *top;                 // Instantiation of module

unsigned int main_time = 0;     // Current simulation time

double sc_time_stamp () {       // Called by $time in Verilog
    return main_time;
}

int getnum(const char *p)
{
    int num;
    if (p[0] == '0') {
	if (p[1] == 'x')
	    sscanf(p+2, "%x", &num);
	else
	    sscanf(p, "%o", &num);
    } else
	sscanf(p, "%d", &num);

    return num;
}

//#define LOAD_MEMORIES

int init_memory(const char *filename, int show)
{
#ifdef LOAD_MEMORIES
    FILE *in;
    in = fopen(filename, "r");
    if (in == NULL)
	    return -1;
    if (in) {
	    fclose(in);
    }
#endif

    return 0;
}

int main(int argc, char** argv)
{
    VerilatedVcdC* tfp = NULL;
    Verilated::commandArgs(argc, argv);   // Remember args

    int initial_pc = 0;
    int load_memory = 0;
    int show_memory = 0;
    int show_waves = 0;
    int inital_pc = 0;
    int force_debug = 0;
    char *mem_filename;

    top = new Vtest_top;             // Create instance

    printf("built on: %s %s\n", __DATE__, __TIME__);

    mem_filename = (char *)"0";

    // process local args
    for (int i = 0; i < argc; i++) {
	    if (argv[i][0] == '+') {
		    switch (argv[i][1]) {
		    case 'l':
			    load_memory++;
			    mem_filename = strdup(argv[++i]);
			    break;
		    case 's': show_memory++; break;
		    case 'w': show_waves++; break;
		    case 'p': initial_pc = getnum(&argv[i][4]); break;
		    case 'd': force_debug++; break;
		    default:
			    fprintf(stderr, "bad arg? %s\n", argv[i]);
			    exit(1);
		    }
	    }
    }

#ifdef VM_TRACE_XX
    if (show_waves) {
	    Verilated::traceEverOn(true);
	    VL_PRINTF("Enabling waves...\n");
	    tfp = new VerilatedVcdC;
	    top->trace(tfp, 99);	// Trace 99 levels of hierarchy
	    tfp->open("test.vcd");	// Open the dump file
    }
#endif

    while (!Verilated::gotFinish()) {

	// load memory
        if (load_memory && main_time == 1) {
	    if (init_memory(mem_filename, show_memory)) {
		perror(mem_filename);
		fprintf(stderr, "memory initialization failed\n");
		exit(1);
	    }
	}

	// resets
	if (main_time < 500) {
	    if (main_time == 20) {
		    VL_PRINTF("reset on\n");
		    top->v__DOT__top__DOT__reset = 1;
	    }
	    if (main_time == 50) {
		    VL_PRINTF("reset off\n");
		    if (initial_pc) {
			    top->v__DOT__top__DOT__initial_pc = initial_pc;
			    VL_PRINTF("initial pc 0%o\n", initial_pc);
		    }
		    top->v__DOT__top__DOT__reset = 0;
	    }
	}

	// Toggle clock
	top->v__DOT__sysclk = ~top->v__DOT__sysclk;

	// Evaluate model
        top->eval();

#ifdef VM_TRACE_XX
	if (tfp)
		tfp->dump(main_time);
#endif

        main_time += 10;
    }

    top->final();

    if (tfp)
	    tfp->close();
}
