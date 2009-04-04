// tt_regs.v

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
   
   reg [15:0] 	 tti_csr, tto_csr, tti_data, tto_data;
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


   reg 		 rx_int, rx_int_enable;
   reg 		 tx_int, tx_int_enable;

   always @(clk or iopage_addr or iopage_rd or iopage_byte_op)
     begin
	if (decode)
	  case (iopage_addr)
	    13'o17560: data_out = {8'b0, tti_empty, rx_int_enable, 6'b0};
	    13'o17562: data_out = tti_data;
	    13'o17564: data_out = {8'b0, tto_empty, tx_int_enable, 6'b0};
	    13'o17566: data_out = tto_data;
	  endcase
     end
   
   always @(posedge clk)
     if (reset)
       begin
	  tti_csr <= 0;
	  tto_csr <= 0;
	  tto_data <= 0;

	  rx_int_enable <= 0;
	  tx_int_enable <= 0;
       end
     else
       if (iopage_wr)
	 case (iopage_addr)
	   13'o17560: rx_int_enable <= data_in[6];
	   	   //13'o17562: tti_data <= data_in;
	   13'o17564: tx_int_enable <= data_in[6];
	   13'o17566: tto_data <= data_in; // uart out
	 endcase

   assign tx_enable = 1'b1;
   assign rx_enable = 1'b1;

   // tto state machine
   // assert ld_tx_data until uart catches up
   // hold off cpu until tx_empty does full transition
   reg [1:0] tto_state;
   wire [1:0] tto_state_next;
   
   assign tto_data_wr = iopage_wr && (iopage_addr == 13'o17566);
   
   always @(posedge clk or reset)
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
   reg [1:0] tti_state;
   wire [1:0] tti_state_next;

   assign tti_data_rd = iopage_rd && (iopage_addr == 13'o17562);
   
   always @(posedge clk or reset)
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
   
   always @(posedge clk or reset)
     if (reset)
       tti_data <= 0;
     else
       if (~rx_empty)
	 tti_data <= rx_data;

   // interrupts
   always @(posedge clk or reset)
     if (reset)
       begin
	  rx_int <= 0;
	  tx_int <= 0;
       end
     else
       begin
	  if (rx_int_enable && ~tti_empty)
	    rx_int <= 1;

	  if (tx_int_enable && tto_empty)
	    tx_int <= 1;
	  else
	    tx_int <= 0;
       end
   
endmodule


