// tt_regs.v
//
// simulated DL11 uart for pdp11
// copyright Brad Parker <brad@heeltoe.com> 2009

module tt_regs(clk, brgclk, reset, iopage_addr, data_in, data_out, decode,
	       iopage_rd, iopage_wr, iopage_byte_op,
	       interrupt, interrupt_ack, vector,
	       rs232_tx, rs232_rx);

   input clk;
   input brgclk;
   input reset;
   input [12:0] iopage_addr;
   input [15:0] data_in;
   input 	iopage_rd, iopage_wr, iopage_byte_op;
   input 	interrupt_ack;
   
   output [15:0] data_out;
   output 	 decode;

   output 	 interrupt;
   output [7:0]  vector;
   reg [7:0]  vector;

   output 	 rs232_tx;
   input 	 rs232_rx;
   
   reg [15:0] 	 tto_data;
   reg 		 tto_empty;
   wire 	 tto_data_wr;
   wire 	 tti_full;
   wire 	 tti_data_rd;

   reg [1:0] 	 interrupt_out;
 	 
   wire 	 asserting_tx_int;
   wire 	 asserting_rx_int;
   
   wire 	 assert_tx_int;
   wire 	 clear_tx_int;
   
   wire 	 assert_rx_int;
   wire 	 clear_rx_int;

   assign 	 decode = (iopage_addr == 13'o17560) |
			  (iopage_addr == 13'o17562) |
			  (iopage_addr == 13'o17564) |
			  (iopage_addr == 13'o17566);

   wire 	 uart_tx_clk;
   wire 	 uart_rx_clk;
   
   brg baud_rate_generator(.clk(brgclk), .reset(reset),
			   .tx_baud_clk(uart_tx_clk),
			   .rx_baud_clk(uart_rx_clk));

   wire 	 ld_tx_req, ld_tx_ack;
   wire		 uld_rx_req, uld_rx_ack;
   wire 	 tx_enable, tx_empty;
   wire 	 rx_enable, rx_empty;
   wire [7:0] 	 rx_data;

`ifdef fake_uart
   fake_uart
`else
   uart
`endif
     tt_uart(
	     .clk(clk),
	     .reset(reset),

	     .txclk(uart_tx_clk),
	     .ld_tx_req(ld_tx_req),
	     .ld_tx_ack(ld_tx_ack),
	     .tx_data(tto_data[7:0]), 
	     .tx_enable(tx_enable),
	     .tx_out(rs232_tx),
	     .tx_empty(tx_empty),
      
	     .rxclk(uart_rx_clk),
	     .uld_rx_req(uld_rx_req),
	     .uld_rx_ack(uld_rx_ack),
	     .rx_data(rx_data),
	     .rx_enable(rx_enable),
	     .rx_in(rs232_rx),
	     .rx_empty(rx_empty)
	     );

   reg 		 rx_int;
   reg 		 tx_int;

   reg 		 rx_int_enable;
   reg 		 tx_int_enable;

   wire [15:0] 	 reg_in;
   reg [15:0] 	 reg_out;
   
   // iopage reads
   always @(clk or decode or iopage_addr or iopage_rd or 
	    tti_full or tto_empty or rx_data or tto_data or 
            rx_int_enable or tx_int_enable)
     begin
	if (iopage_rd && decode)
	  case (iopage_addr)
	    13'o17560: reg_out = {8'b0, tti_full, rx_int_enable, 6'b0};
    	    13'o17562: reg_out = {8'b0, rx_data};
	    13'o17564: reg_out = {8'b0, tto_empty, tx_int_enable, 6'b0};
	    13'o17566: reg_out = tto_data;
	    default: reg_out = 16'b0;
	  endcase
	else
	  reg_out = 16'b0;
     end

   // handle byte accesses on read
   assign data_out = iopage_byte_op ?
		     {8'b0, iopage_addr[0] ? reg_out[15:8] : reg_out[7:0]} :
		     reg_out;

   // iopage writes
   assign reg_in = (iopage_byte_op & iopage_addr[0]) ? {8'b0, data_in[15:8]} :
		   data_in;
   
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
	   13'o17560:
	     begin
`ifdef debug_tt_int
		$display("tt: XXX rx_int_enable %b", reg_in[6]);
`endif
		rx_int_enable <= reg_in[6];	// tti csr
	     end

   	   //13'o17562: tti_data <= reg_in;

	   13'o17564:
	     begin
`ifdef debug_tt_int
		$display("tt: XXX tx_int_enable %b", reg_in[6]);
`endif
		tx_int_enable <= reg_in[6];	// tto csr
	     end
	   13'o17566:
	     begin
		tto_data <= reg_in;		// tto data
`ifdef debug_tt_out
		if (reg_in < 16'o40)
		  $display("tto_data %o", reg_in);
		else
		  $display("tto_data %o %c", reg_in, reg_in[7:0] & 8'h7f);
`endif
	     end
	 endcase

   assign tx_enable = 1'b1;
   assign rx_enable = 1'b1;

   // tto state machine
   // assert ld_tx_req until uart catches up
   // hold off cpu until tx_empty does full transition
   // state 0 - idle; wait for iopage write to data
   // state 1 - wait for ld_tx_ack to assert
   // state 2 - wait for ld_tx_ack to deassert
   // state 3 - wait for tx_empty to assert
   // state 4 - 
   reg [2:0] tto_state;
   wire [2:0] tto_state_next;
   
   assign tto_data_wr = iopage_wr && (iopage_addr == 13'o17566);
   
   always @(posedge clk)
     if (reset)
       tto_state <= 0;
     else
       tto_state <= tto_state_next;

   assign tto_state_next = (tto_state == 0 && tto_data_wr) ? 1 :
			   (tto_state == 1 && ld_tx_ack) ? 2 :
			   (tto_state == 2 && ~ld_tx_ack) ? 3 :
			   (tto_state == 3) ? 0 :
			   tto_state;

   assign ld_tx_req = tto_state == 1;

   reg [3:0] tx_empty_sample;
   always @(posedge clk)
     if (reset)
       tx_empty_sample <= 4'b0;
     else
       tx_empty_sample <= { tx_empty_sample[2:0], tx_empty };
   
   assign  assert_tx_int =
			  (tx_empty_sample[3:2] == 2'b01) ||
			  /* below is a hack which I don't think I need anymore */
			  (tto_state == 0 && tx_empty_sample[3:2] == 2'b11 &&
			   iopage_wr && (iopage_addr == 13'o17564) && reg_in[6]);

   assign  clear_tx_int = tto_data_wr/*;*/
			  || (interrupt_ack && !asserting_rx_int && asserting_tx_int);
  
   always @(posedge clk)
     if (reset)
       tto_empty <= 0;
     else
       tto_empty <= tx_empty;
   
 `ifdef debug
   always @(posedge clk)
     if (tto_state != 0)
	   $display("uart: tto_state %d ld_tx_req %b ld_tx_ack %b tx_empty %b",
		    tto_state, ld_tx_req, ld_tx_ack, tx_empty);
 `endif
   
   
   // tti state machine
   // don't become ready until we've clock data out of uart holding reg
   // state 0 - idle; wait for rx_empty to deassert
   // state 1 - wait for uld_rx_ack to assert
   // state 2 - wait for uld_rx_ack to deassert
   // state 3 - wait for iopage read of uart (tti); assert interrupt
   // state 4 - wait for rx_empty to assert
   reg [2:0] tti_state;
   wire [2:0] tti_state_next;

   assign tti_data_rd = iopage_rd && (iopage_addr == 13'o17562);
   
   always @(posedge clk)
     if (reset)
       tti_state <= 0;
     else
       tti_state <= tti_state_next;

   assign tti_state_next = (tti_state == 0 && ~rx_empty) ? 1 :
			   (tti_state == 1 && uld_rx_ack) ? 2 :
			   (tti_state == 2 && ~uld_rx_ack) ? 3 :
			   (tti_state == 3 && tti_data_rd) ? 4 :
			   (tti_state == 4) ? 0 :
			   tti_state;
   
   assign uld_rx_req = tti_state == 1;
   assign tti_full = tti_state == 3;
   
   assign assert_rx_int = tti_state == 2 && ~uld_rx_ack;
   assign clear_rx_int = tti_data_rd;

 `ifdef debug
   always @(posedge clk)
     if (tti_state != tti_state_next)
	   $display("uart: tti_state %d uld_rx_req %b uld_rx_ack %b rx_empty %b",
		    tti_state, uld_rx_req, uld_rx_ack, rx_empty);
 `endif
   
   // interrupts
   assign interrupt = |interrupt_out;
   
   assign asserting_rx_int = rx_int_enable && rx_int;
   assign asserting_tx_int = tx_int_enable && tx_int;

   always @(posedge clk)
     if (reset)
       interrupt_out <= 2'b00;
     else
       // since we have 2 sources, ack must be higher priority
       if (interrupt_ack)
	 begin
	    if (asserting_rx_int)
	      interrupt_out[0] <= 0;
	    else
	      if (asserting_tx_int)
		interrupt_out[1] <= 0;
	 end
       else
	 begin
	    interrupt_out[0] <= asserting_rx_int;
	    interrupt_out[1] <= asserting_tx_int;
	    // assert vector in int priority order
	    vector <= asserting_rx_int ? 8'o60 : 8'o64;
	 end
   
   always @(posedge clk)
     if (reset)
       begin
	  tx_int <= 1;
	  rx_int <= 0;
       end
     else
       begin
	  if (assert_tx_int)
	    tx_int <= 1;
	  else
	    if (clear_tx_int)
	      tx_int <= 0;

	  if (assert_rx_int)
	    rx_int <= 1;
	  else
	    if (clear_rx_int)
	      rx_int <= 0;
       end
   
`ifdef debug/*_tt_int*/
   always @(posedge clk)
     begin
	if (tto_state == 1)
	  $display("tt: XXX tto_state 1");
	if (tto_data_wr)
	  $display("tt: XXX tto_data_wr");
	if (tx_int == 0 && assert_tx_int)
	  $display("tt: XXX tx_int+ %b; %t", tx_int, $time);
	if (tx_int == 1 && clear_tx_int)
	  $display("tt: XXX tx_int- %b; %t", tx_int, $time);
	if (rx_int == 0 && assert_rx_int)
	  $display("tt: XXX rx_int+ %b; %t", rx_int, $time);
	if (rx_int == 1 && clear_rx_int)
	  $display("tt: XXX rx_int- %b; %t", rx_int, $time);
	if (interrupt_ack)
	  $display("tt: interrupt_ack; %t", $time);
     end
`endif
   
endmodule


