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

   output 	 rs232_tx;
   input 	 rs232_rx;
   
   reg [15:0] 	 tto_data;
   wire 	 tto_empty;
   wire 	 tto_data_wr;
   wire 	 tti_full;
   wire 	 tti_data_rd;

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
   integer 	 fake_count;
   reg [7:0] 	 fake_rx_data;
`endif

   uart tt_uart(.clk(clk), .reset(reset),

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
		.rx_empty(rx_empty));

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
`ifdef fake_uart
    	    13'o17562: reg_out = {8'b0, fake_rx_data};
`else
    	    13'o17562: reg_out = {8'b0, rx_data};
`endif
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
   // state 1 - wait for tx_ack to assert
   // state 2 - wait for tx_ack to deassert
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

`ifdef fake_uart
   // quicker version for sim; don't wait for uart
   assign tto_state_next = (tto_state == 0 && tto_data_wr) ? 1 :
			   (tto_state == 1) ? 2 :
			   (tto_state == 2) ? 3 :
			   (tto_state == 3 && _tx_delay == 0) ? 4 :
			   (tto_state == 4) ? 0 :
			   tto_state;

   integer _tx_delay;

   initial
     _tx_delay = 0;
   
   always @(posedge clk)
     if (tto_data_wr)
         _tx_delay = 100;
     else
       if (_tx_delay > 0)
	 _tx_delay = _tx_delay - 1;

`else
   assign tto_state_next = (tto_state == 0 && tto_data_wr) ? 1 :
			   (tto_state == 1 && ld_tx_ack) ? 2 :
			   (tto_state == 2 && ~ld_tx_ack) ? 3 :
			   (tto_state == 3 && tx_empty) ? 4 :
			   (tto_state == 4) ? 0 :
			   tto_state;

 `ifdef debug
   always @(posedge clk)
     if (tto_state != 0)
	   $display("tto_state %d ld_tx_ack %b tx_empty %b",
		    tto_state, ld_tx_ack, tx_empty);
 `endif
   
`endif

   assign  assert_tx_int = (tto_state == 3) ||
		   (tto_state == 0 && 
		    iopage_wr && (iopage_addr == 13'o17564) && reg_in[6]);

   assign clear_tx_int =
		(interrupt_ack && asserting_tx_int && !asserting_rx_int);

   assign ld_tx_req = tto_state == 1;
   assign tto_empty = tto_state == 0;
   
   // tti state machine
   // don't become ready until we've clock data out of uart holding reg
   // state 0 - idle; wait for rx_empty to deassert
   // state 1 - wait for rx_empty to assert
   // state 2 - wait for rx_empty to deassert
   // state 3 - wait for iopage read of uart (tti)
   // state 4 - assert interrupt
   reg [2:0] tti_state;
   wire [2:0] tti_state_next;

   assign tti_data_rd = iopage_rd && (iopage_addr == 13'o17562);
   
   always @(posedge clk)
     if (reset)
       tti_state <= 0;
     else
       tti_state <= tti_state_next;

`ifdef fake_uart
`define no_fake_input
//`define v6_unix
//`define v7_unix
//`define bsd29_unix
//`define bsd211_unix
//`define rsts

 `ifdef v6_unix
   parameter fake_max = 9;
 `endif
 `ifdef v7_unix
   parameter fake_max = 18;
 `endif
 `ifdef bsd29_unix
   parameter fake_max = 13;
 `endif
 `ifdef bsd211_unix
   parameter fake_max = 11;
 `endif
 `ifdef rsts
   parameter fake_max = 21;
 `endif
 `ifdef no_fake_input
   parameter fake_max = 0;
 `endif
     
   assign tti_state_next = (tti_state == 0 && fake_count <= fake_max) ? 1 :
			   (tti_state == 1) ? 3 :
			   (tti_state == 3 && tti_data_rd) ? 4 :
			   (tti_state == 4) ? 0 :
			   tti_state;

   initial
`ifdef no_fake_input
     fake_count = 10;
`else
     fake_count = 0;
`endif
   
   always @(posedge clk)
     if (tti_state == 1)
       begin
	  case (fake_count)
`ifdef rsts
	    0: fake_rx_data <= 8'h53; //S
	    1: fake_rx_data <= 8'h54; //T
	    2: fake_rx_data <= 8'h41; //A
	    3: fake_rx_data <= 8'h52; //R
	    4: fake_rx_data <= 8'h54; //T
	    5: fake_rx_data <= 8'h0d; //<ret>
	    6: fake_rx_data <= 8'h30;//0
	    7: fake_rx_data <= 8'h31;//1
	    8: fake_rx_data <= 8'h2d;//-
	    9: fake_rx_data <= 8'h4a;//J
	    10: fake_rx_data <= 8'h41;//A
	    11: fake_rx_data <= 8'h4e;//N
	    12: fake_rx_data <= 8'h2d;//-
	    13: fake_rx_data <= 8'h38;//8
	    14: fake_rx_data <= 8'h35;//5
	    15: fake_rx_data <= 8'h0d;//<ret>
	    16: fake_rx_data <= 8'h31;//1
	    17: fake_rx_data <= 8'h30;//0
	    18: fake_rx_data <= 8'h3a;//:
	    19: fake_rx_data <= 8'h31;//1
	    20: fake_rx_data <= 8'h30;//0
	    21: fake_rx_data <= 8'h0d;//<ret>
`endif
`ifdef v6_unix
	    0: fake_rx_data <= 8'h72; //r
	    1: fake_rx_data <= 8'h6b; //k
	    2: fake_rx_data <= 8'h75; //u
	    3: fake_rx_data <= 8'h6e; //n
	    4: fake_rx_data <= 8'h69; //i
	    5: fake_rx_data <= 8'h78; //x
	    6: fake_rx_data <= 8'h2e; //.
	    7: fake_rx_data <= 8'h34; //4
	    8: fake_rx_data <= 8'h30; //0
	    9: fake_rx_data <= 8'h0d; //<ret>
`endif
`ifdef v7_unix
	    0: fake_rx_data <= 8'h62; //b
	    1: fake_rx_data <= 8'h6f; //o
	    2: fake_rx_data <= 8'h6f; //o
	    3: fake_rx_data <= 8'h74; //t
	    4: fake_rx_data <= 8'h0d; //<ret>
	    5: fake_rx_data <= 8'h72; //r
	    6: fake_rx_data <= 8'h6b; //k
	    7: fake_rx_data <= 8'h28; //(
	    8: fake_rx_data <= 8'h30; //0
	    9: fake_rx_data <= 8'h2c; //,
	    10: fake_rx_data <= 8'h30; //0
	    11: fake_rx_data <= 8'h29; //)
	    12: fake_rx_data <= 8'h72; //r
	    13: fake_rx_data <= 8'h6b; //k
	    14: fake_rx_data <= 8'h75; //u
	    15: fake_rx_data <= 8'h6e; //n
	    16: fake_rx_data <= 8'h69; //i
	    17: fake_rx_data <= 8'h78; //x
	    18: fake_rx_data <= 8'h0d; //<ret>
`endif
`ifdef bsd29_unix
	    0: fake_rx_data <= 8'h72; //r
	    1: fake_rx_data <= 8'h6b; //k
	    2: fake_rx_data <= 8'h28; //(
	    3: fake_rx_data <= 8'h30; //0
	    4: fake_rx_data <= 8'h2c; //,
	    5: fake_rx_data <= 8'h30; //0
	    6: fake_rx_data <= 8'h29; //)
	    7: fake_rx_data <= 8'h72; //r
	    8: fake_rx_data <= 8'h6b; //k
	    9: fake_rx_data <= 8'h75; //u
	    10: fake_rx_data <= 8'h6e; //n
	    11: fake_rx_data <= 8'h69; //i
	    12: fake_rx_data <= 8'h78; //x
	    13: fake_rx_data <= 8'h0d; //<ret>
`endif	    
`ifdef bsd29_unix
	    0: fake_rx_data <= 8'h72; //r
	    1: fake_rx_data <= 8'h6b; //k
	    2: fake_rx_data <= 8'h28; //(
	    3: fake_rx_data <= 8'h30; //0
	    4: fake_rx_data <= 8'h2c; //,
	    5: fake_rx_data <= 8'h30; //0
	    6: fake_rx_data <= 8'h29; //)
	    7: fake_rx_data <= 8'h72; //r
	    8: fake_rx_data <= 8'h6b; //k
	    9: fake_rx_data <= 8'h75; //u
	    10: fake_rx_data <= 8'h6e; //n
	    11: fake_rx_data <= 8'h69; //i
	    12: fake_rx_data <= 8'h78; //x
	    13: fake_rx_data <= 8'h0d; //<ret>
`endif	    
`ifdef bsd211_unix
	    0: fake_rx_data <= 8'h72; //r
	    1: fake_rx_data <= 8'h6b; //k
	    2: fake_rx_data <= 8'h28; //(
	    3: fake_rx_data <= 8'h30; //0
	    4: fake_rx_data <= 8'h2c; //,
	    5: fake_rx_data <= 8'h30; //0
	    6: fake_rx_data <= 8'h29; //)
	    7: fake_rx_data <= 8'h75; //u
	    8: fake_rx_data <= 8'h6e; //n
	    9: fake_rx_data <= 8'h69; //i
	    10: fake_rx_data <= 8'h78; //x
	    11: fake_rx_data <= 8'h0d; //<ret>
`endif	    
	    default: ;
	  endcase
	  $display("tti: sending fake #%d", fake_count);
	  fake_count = fake_count + 1;
       end
	  
   always @(iopage_rd or iopage_addr)
     if (iopage_rd && (iopage_addr == 13'o17562 || iopage_addr == 13'o17563))
       $display("tti: read %o -> %o", iopage_addr, fake_rx_data);
   
	   
`else
   assign tti_state_next = (tti_state == 0 && ~rx_empty) ? 1 :
			   (tti_state == 1 && uld_rx_ack) ? 2 :
			   (tti_state == 2 && ~uld_rx_ack) ? 3 :
			   (tti_state == 3 && tti_data_rd) ? 4 :
			   (tti_state == 4) ? 0 :
			   tti_state;
`endif
   
   assign uld_rx_req = tti_state == 1;
   assign tti_full = tti_state == 3;
   
   assign assert_rx_int = tti_full;

   assign clear_rx_int = (interrupt_ack && asserting_rx_int);

   // interrupts
   assign interrupt = asserting_rx_int || asserting_tx_int;
   
   assign asserting_rx_int = rx_int_enable && rx_int;
   assign asserting_tx_int = tx_int_enable && tx_int;

   // assert vector in int priority order
   assign vector = asserting_rx_int ? 8'o60 :
		   asserting_tx_int ? 8'o64 :
		   8'b0;

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
//	if (interrupt)
//	  $display("tt: interrupt %b %b; %t", 
//		   asserting_rx_int, asserting_tx_int, $time);
	if (interrupt_ack)
	  $display("tt: interrupt_ack; %t", $time);
     end
`endif
   
endmodule


