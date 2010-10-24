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

int main(int argc, char** argv)
{
    VerilatedVcdC* tfp = NULL;
    Verilated::commandArgs(argc, argv);   // Remember args

    top = new Vtest_top;             // Create instance

#ifdef TRACE
    if (0) {
	    Verilated::traceEverOn(true);
	    VL_PRINTF("Enabling waves...\n");
	    tfp = new VerilatedVcdC;
	    top->trace(tfp, 99);	// Trace 99 levels of hierarchy
	    tfp->open("test.vcd");	// Open the dump file
    }
#endif

    while (!Verilated::gotFinish()) {

	// Resets
	if (main_time < 500) {
	    if (main_time == 20) {
		    VL_PRINTF("reset on\n");
		    top->v__DOT__top__DOT__reset = 1;
//		    top->v__DOT__initial_pc = 07400;
	    }
	    if (main_time == 50) {
		    VL_PRINTF("reset off\n");
		    top->v__DOT__top__DOT__reset = 0;
	    }
	}

	// Toggle clock
	top->v__DOT__sysclk = ~top->v__DOT__sysclk;

	// Evaluate model
        top->eval();

#ifdef TRACE
	if (tfp)
		tfp->dump(main_time);
#endif

        main_time += 10;
    }

    top->final();

    if (tfp)
	    tfp->close();
}
