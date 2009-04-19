
`include "debounce.v"
//`include "sevenseg.v"
`include "sevensegdecode.v"
`include "display_hex.v"

`include "brg.v"
`include "uart.v"

module top(rs232_txd, rs232_rxd,
	   button, led, sysclk,
	   sevenseg, sevenseg_an,
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
   
output/*   inout*/ [15:0] ide_data_bus;
   output       ide_dior, ide_diow;
   output [1:0] ide_cs;
   output [2:0] ide_da;

   //
//   wire 	clk;
   wire         reset;
   wire [15:0] initial_pc;
   wire [15:0] pc;

//   assign clk = sysclk;
   assign initial_pc = 16'o173000;

   wire [7:0]  rx_data;

   //-----------
   reg clk;
   reg [/*15*//*24*/24:0] clkdiv;

   always @(posedge sysclk)
     if (reset)
       begin
         clkdiv <= 0;
         clk <= 1;
       end
     else
       begin
         clkdiv <= clkdiv + 1'b1;
         if (clkdiv == 0)
            clk <= ~clk;
       end
   //-----------

   debounce reset_sw(.clk(sysclk), .in(button[3]), .out(reset));

wire [15:0] ide_data_out;

   display_hex show_data(.clk(sysclk), .reset(reset), .hex(ide_data_bus),
			 .sevenseg(sevenseg), .sevenseg_an(sevenseg_an),
			 .dots(ide_data_bus[3:0]));

//   assign led = {ide_cs, ide_diow, rk_state};
//   assign led = button[2] ? ide_data_bus[15:8] : ide_data_bus[7:0];
   assign led = rx_data;

//assign ide_data_bus = ide_data_out;
assign ide_data_bus = 16'h1234;

   wire [21:0] ram_addr;
   wire [15:0] ram_data_in, ram_data_out;
   wire        ram_rd, ram_wr, ram_byte_op;

   wire        ram_oe_n, ram_we_n, ram_ce_n;
   wire [15:0] ram1_io, ram2_io;
   wire        ram_ub, ram_lb;

   wire [15:0] switches;
   assign switches = {8'b0, slideswitch};

assign ram_a = 18'b0;
assign ram_oe_n = 1'b1;
assign ram_we_n = 1'b1;

assign ram1_io = 16'b0;
assign ram1_ce_n = 1'b1;
assign ram1_ub_n = 1'b1;
assign ram1_lb_n = 1'b1;

assign ram2_io = 16'b0;
assign ram2_ce_n = 1'b1;
assign ram2_ub_n = 1'b1;
assign ram2_lb_n = 1'b1;

assign ide_cs = 0;
assign ide_da = 0;
assign ide_dior = 1'b1;
assign ide_diow = 1'b1;

   // ----------------------------------------------
   wire 	 uart_tx_clk;
   wire 	 uart_rx_clk;
   
   brg baud_rate_generator(.clk(sysclk), .reset(reset),
			   .tx_baud_clk(uart_tx_clk),
			   .rx_baud_clk(uart_rx_clk));

   reg 		 ld_tx_data;
   reg		 uld_rx_data;
   wire 	 tx_enable, tx_empty;
   wire 	 rx_enable, rx_empty;

   reg [15:0] 	 tti_data, tto_data;

   assign tx_enable = 1'b1;
   assign rx_enable = 1'b1;

   wire 	 txd;
   assign rs232_txd = /*~*/txd;
   
   uart tt_uart(.clk(clk), .reset(reset),

		.txclk(uart_tx_clk),
		.ld_tx_data(ld_tx_data),
		.tx_data(tto_data[7:0]), 
		.tx_enable(tx_enable),
		.tx_out(txd),
		.tx_empty(tx_empty),

		.rxclk(uart_rx_clk),
		.uld_rx_data(uld_rx_data),
		.rx_data(rx_data),
		.rx_enable(rx_enable),
		.rx_in(rs232_rxd),
		.rx_empty(rx_empty));

   always @(posedge clk)
     begin
	if (tx_empty)
	  begin
	     ld_tx_data <= 1;
	     if (tto_data < 8'h20 || tto_data > 8'h7f)
	       tto_data <= 8'h20;
	     else
	       tto_data <= tto_data + 1;
	  end
	else
	  ld_tx_data <= 0;
     end
   
   always @(posedge clk)
     begin
	if (~tx_empty)
	  uld_rx_data <= 1;
	else
	  uld_rx_data <= 0;
     end
   
   // ----------------------------------------------
   
endmodule
