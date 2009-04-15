`include "pdp11.v"

`include "debounce.v"

module top(rs232_txd, rs232_rxd,
	   button, led, sysclk,
	   ps2_clk, ps2_data, sevenseg, sevenseg_an,
	   slideswitch,
	   ram_a, ram_oe_n, ram_we_n,
	   ram1_io, ram1_ce_n, ram1_ub_n, ram1_lb_n,
	   ram2_io, ram2_ce_n, ram2_ub_n, ram2_lb_n,
	   ide_data_bus, ide_dior, ide_diow, ide_cs, ide_da);

   output	rs232_txd;
   input	rs232_rxd;

   input [3:0] 	button;

   output [7:0] led;
   input 	sysclk;

   output 	ps2_clk;
   inout 	ps2_data;

   output [7:0] sevenseg;
   output [3:0] sevenseg_an;

   input [7:0] 	slideswitch;

   output [17:0] ram_a;
   output 	 ram_oe_n;
   output 	 ram_we_n;

   inout [15:0]	 ram1_io;
   output 	 ram1_ce_n;
   output 	 ram1_ub_n;
   output 	 ram1_lb_n;

   inout [15:0]	 ram2_io;
   output 	 ram2_ce_n;
   output 	 ram2_ub_n;
   output 	 ram2_lb_n;
   
   inout [15:0] ide_data_bus;
   output       ide_dior, ide_diow;
   output [1:0] ide_cs;
   output [2:0] ide_da;

   //
   wire 	clk;
   wire [15:0] initial_pc;

   assign clk = sysclk;
   assign initial_pc = 16'o173000;

   debounce reset_sw(.clk(clk), .in(button[3]), .out(reset));
	       
   pdp11 cpu(.clk(clk), .reset_n(~reset), .switches({8'b0, slideswitch}),
	     .initial_pc(initial_pc),
	     .rs232_tx(rs232_txd), .rs232_rx(rs232_rxd),
	     .ide_data_bus(ide_data_bus),
	     .ide_dior(ide_dior), .ide_diow(ide_diow),
	     .ide_cs(ide_cs), .ide_da(ide_da));

endmodule
