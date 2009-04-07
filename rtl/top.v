`include "pdp11.v"

module top(clk, reset_n, switches, 
	   ide_data_bus, ide_dior, ide_diow, ide_cs, ide_da);

   input clk, reset_n;
   input [15:0] switches;

   inout [15:0] ide_data_bus;
   output        ide_dior, ide_diow;
   output [1:0]  ide_cs;
   output [2:0]  ide_da;

   pdp11 cpu(.clk(clk), .reset_n(reset_n), .switches(switches),
	     .ide_data_bus(ide_data_bus),
	     .ide_dior(ide_dior), .ide_diow(ide_diow),
	     .ide_cs(ide_cs), .ide_da(ide_da));

endmodule
