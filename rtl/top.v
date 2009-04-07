
module top;

   wire clk, reset_n;
   wire [15:0] switches;

   wire [15:0] ide_data_bus;
   wire        ide_dior, ide_diow;
   wire [1:0]  ide_cs;
   wire [2:0]  ide_da;

   assign      switches = 16'b0;
   

   pdp11 cpu(.clk(clk), .reset_n(reset_n),
	     .switches(switches),
	     .ide_data_bus(ide_data_bus),
	     .ide_dior(ide_dior), .ide_diow(ide_diow),
	     .ide_cs(ide_cs), .ide_da(ide_da));

endmodule
