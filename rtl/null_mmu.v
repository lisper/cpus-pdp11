// null_mmu.v

module null_mmu(clk, reset, soft_reset,
		cpu_va, cpu_cm, cpu_rd, cpu_wr, cpu_i_access, cpu_d_access,
		cpu_trap, cpu_pa, fetch_va, trap_odd, 
		signal_abort, signal_trap,
		pxr_wr, pxr_rd, pxr_be, pxr_addr, pxr_data_in, pxr_data_out);

   input clk;
   input reset;
   input soft_reset;
   
   input [15:0] cpu_va;
   input [1:0] 	cpu_cm;
   input 	cpu_rd;
   input 	cpu_wr;
   input 	cpu_i_access;
   input 	cpu_d_access;
   input 	cpu_trap;

   input 	fetch_va;
   input 	trap_odd;
   output 	signal_abort;
   output 	signal_trap;
 	
   input 	pxr_wr;
   input 	pxr_rd;
   input [1:0] 	pxr_be;
   input [7:0] 	pxr_addr;
   input [15:0] pxr_data_in;

   output [21:0] cpu_pa;
   output [15:0] pxr_data_out;

   wire 	 va_is_iopage;
   
   assign signal_abort = 1'b0;
   assign signal_trap = 1'b0;
   assign pxr_data_out = 16'b0;

   assign va_is_iopage = cpu_va[15:13] == 3'b111;	/* 8k io page */

   assign cpu_pa = va_is_iopage ? {6'o77, cpu_va} : {6'b0, cpu_va};

endmodule // mmu
