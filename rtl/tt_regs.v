// tt_regs.v
//
// simulated DL11 uart for pdp11
// copyright Brad Parker <brad@heeltoe.com> 2009

`include "brg.v"
`include "uart.v"

module tt_regs(clk, reset, iopage_addr, data_in, data_out, decode,
	       iopage_rd, iopage_wr, iopage_byte_op,
	       interrupt, vector,
	       rs232_tx, rs232_rx);

   input clk;
   input reset;
   input [12:0] iopage_addr;
   input [15:0] data_in;
   input 	iopage_rd, iopage_wr, iopage_byte_op;
   output [15:0] data_out;
   reg [15:0] 	 data_out;
   output 	 decode;

   output 	 interrupt;
   output [7:0]  vector;

   output 	 rs232_tx;
   input 	 rs232_rx;
   
   reg [15:0] 	 tti_data, tto_data;
   wire 	 tto_empty;
   wire 	 tto_data_wr;
   wire 	 tti_empty;
   wire 	 tti_data_rd;
 	 
   assign 	 decode = (iopage_addr == 13'o17560) |
			  (iopage_addr == 13'o17562) |
			  (iopage_addr == 13'o17564) |
			  (iopage_addr == 13'o17566);

   wire 	 uart_tx_clk;
   wire 	 uart_rx_clk;
   
   brg baud_rate_generator(.clk(clk), .reset(reset),
			   .tx_baud_clk(uart_tx_clk),
			   .rx_baud_clk(uart_rx_clk));

   wire 	 ld_tx_data;
   wire		 uld_rx_data;
   wire 	 tx_enable, tx_empty;
   wire 	 rx_enable, rx_empty;
   wire [7:0] 	 rx_data;

   uart tt_uart(.clk(clk), .reset(reset),

		.txclk(uart_tx_clk),
		.ld_tx_data(ld_tx_data),
		.tx_data(tto_data[7:0]), 
		.tx_enable(tx_enable),
		.tx_out(rs232_tx),
		.tx_empty(tx_empty),

		.rxclk(uart_rx_clk),
		.uld_rx_data(uld_rx_data),
		.rx_data(rx_data),
		.rx_enable(rx_enable),
		.rx_in(rs232_rx),
		.rx_empty(rx_empty));

   wire 	 rx_int, tx_int;
   reg 		 rx_int_enable, tx_int_enable;


   // iopage reads
   always @(clk or decode or iopage_addr or tti_empty or tto_empty or
	    tti_data or tto_data or rx_int_enable or tx_int_enable)
     begin
	if (iopage_rd && decode)
	  case (iopage_addr)
	    13'o17560: data_out = {8'b0, tti_empty, rx_int_enable, 6'b0};
	    13'o17562: data_out = tti_data;
	    13'o17564: data_out = {8'b0, tto_empty, tx_int_enable, 6'b0};
	    13'o17566: data_out = tto_data;
	    default: data_out = 16'b0;
	  endcase
	else
	  data_out = 16'b0;
     end

   // iopage writes   
   always @(posedge clk)
     if (reset)
       begin
	  tto_data <= 0;

	  rx_int_enable <= 0;
	  tx_int_enable <= 0;
       end
     else
       if (iopage_wr)
	 case (iopage_addr)
	   13'o17560: rx_int_enable <= data_in[6];	// tti csr
   	   //13'o17562: tti_data <= data_in;
	   13'o17564:
	     begin
`ifdef debug
		$display("tt: XXX tx_int_enable %b", data_in[6]);
`endif
		tx_int_enable <= data_in[6];		// tto csr
	     end
	   13'o17566:
	     begin
		tto_data <= data_in;		// tto data
`ifdef debug
		if (tto_data < 16'o40)
		  $display("tto_data %o", tto_data);
		else
		  $display("tto_data %o %c", tto_data, tto_data);
`endif
	     end
	 endcase

   assign tx_enable = 1'b1;
   assign rx_enable = 1'b1;

   // tto state machine
   // assert ld_tx_data until uart catches up
   // hold off cpu until tx_empty does full transition
   // state 0 - idle; wait for iopage write to data
   // state 1 - wait for tx_empty to assert
   // state 2 - wait for tx_empty to deassert
   reg [1:0] tto_state;
   wire [1:0] tto_state_next;
   
   assign tto_data_wr = iopage_wr && (iopage_addr == 13'o17566);
   
   always @(posedge clk)
     if (reset)
       tto_state <= 0;
     else
       tto_state <= tto_state_next;

   assign tto_state_next = (tto_state == 0 && tto_data_wr) ? 1 :
			   (tto_state == 1 && ~tx_empty) ? 2 :
			   (tto_state == 2 && tx_empty) ? 0 :
			   tto_state;

   assign ld_tx_data = tto_state == 1;
   assign tto_empty = tto_state == 0;
   
   // tti state machine
   // don't become ready until we've clock data out of uart holding reg
   // state 0 - idle; wait for rx_empty to deassert
   // state 1 - wait for rx_empty to assert
   // state 2 - wait for iopage read of uart (tti)
   reg [1:0] tti_state;
   wire [1:0] tti_state_next;

   assign tti_data_rd = iopage_rd && (iopage_addr == 13'o17562);
   
   always @(posedge clk)
     if (reset)
       tti_state <= 0;
     else
       tti_state <= tti_state_next;

   assign tti_state_next = (tti_state == 0 && ~rx_empty) ? 1 :
			   (tti_state == 1 && rx_empty) ? 2 :
			   (tti_state == 2 && tti_data_rd) ? 0 :
			   tti_state;

   assign uld_rx_data = tti_state == 1;
   assign tti_empty = tti_state == 0;
   
   always @(posedge clk)
     if (reset)
       tti_data <= 0;
     else
       if (~rx_empty)
	 tti_data <= rx_data;

   // interrupts
   assign rx_int = rx_int_enable && ~tti_empty;
   assign tx_int = tx_int_enable && tto_empty;

   assign interrupt = rx_int || tx_int;
   assign vector = rx_int ? 8'o60 :
		   tx_int ? 8'o64 :
		   8'b0;

`ifdef debug_tt_int
   always @(posedge clk)
     begin
	if (tx_int)
	  $display("tt: XXX tx_int %b", tx_int);
     end
`endif
   
endmodule


