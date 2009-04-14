
`timescale 1ns / 1ns

`include "pdp11.v"

module test;

   reg clk, reset_n;
   reg [15:0] switches;

   wire       rs232_tx;
   reg 	      rs232_rx;

   wire [15:0] ide_data_bus;
   wire        ide_dior, ide_diow;
   wire [1:0]  ide_cs;
   wire [2:0]  ide_da;

   reg [15:0]  starting_pc;

   pdp11 cpu(.clk(clk), .reset_n(reset_n), .switches(switches),
	     .initial_pc(starting_pc),
	     .rs232_tx(rs232_tx), .rs232_rx(rs232_rx),
	     .ide_data_bus(ide_data_bus),
	     .ide_dior(ide_dior), .ide_diow(ide_diow),
	     .ide_cs(ide_cs), .ide_da(ide_da));

   reg [1023:0] arg;
   integer 	n;

   initial
     begin
	$timeformat(-9, 0, "ns", 7);

	starting_pc = 16'o173000;
//	starting_pc = 16'o0200;
//	starting_pc = 16'o0500;
	
 	n = $scan$plusargs("pc=", arg);
	if (n > 0)
	  begin
//	     $sformat(arg, "%o", starting_pc);
//	     $sformat(starting_pc, "%o", arg);
//	     $sscanf(arg, "%o", starting_pc);
	     $display("arg %s pc %o", arg, starting_pc);
	  end
	
`ifdef debug_log
`else
	$nolog;
`endif
	
`ifdef debug_vcd
	$dumpfile("pdp11.vcd");
	$dumpvars(0, test.cpu);
`endif
     end

   initial
     begin
	clk = 0;
	reset_n = 1;
	switches = 0;
	rs232_rx = 0;
	
	#1 begin
           reset_n = 0;
	end

	#40 begin
           reset_n = 1;
	end
	
//       #5000000 $finish;
     end

   always @(posedge clk)
     begin
	$pli_ide(ide_data_bus, ide_dior, ide_diow, ide_cs, ide_da);
     end

   always
     begin
	#10 clk = 0;
	#10 clk = 1;
     end

   //----
   integer cycle;

   initial
     cycle = 0;

   always @(posedge cpu.clk)
     begin
	cycle = cycle + 1;
	#1 begin
	   if (cpu.istate == 1)
	     $display("------------------------------");
	   $display("cycle %d, pc %o, psw %o, istate %d",
		    cycle, cpu.pc, cpu.psw, cpu.istate);
	end

	if (cpu.istate == 0)
	  $finish;
     end

endmodule

