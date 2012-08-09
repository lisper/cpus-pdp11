// fake_uart.v
// sim-only fake uart for debug
// brad@heeltoe.com 2012

//`define no_fake_input
//`define v6_unix
//`define v7_unix
//`define bsd29_unix
//`define bsd211_unix
//`define rsts
//`define test_input
`define dir


module fake_uart(clk, reset,
		 txclk, ld_tx_req, ld_tx_ack, tx_data, tx_enable, tx_out, tx_empty,
		 rxclk, uld_rx_req, uld_rx_ack, rx_data, rx_enable, rx_in,rx_empty);
   
   input        clk;
   input        reset;
   input        txclk;
   input        ld_tx_req;
   output 	ld_tx_ack;
   input [7:0] 	tx_data;
   input        tx_enable;
   output       tx_out;
   output       tx_empty;
   input        rxclk;
   input        uld_rx_req;
   output 	uld_rx_ack;
   output [7:0] rx_data;
   input        rx_enable;
   input        rx_in;
   output       rx_empty;

   integer 	 fake_count;
   reg 		 fake_done;
   reg [7:0] 	 rx_data;

   //
   reg [1:0]	rx_uld;
   wire [1:0] 	rx_uld_next;

   reg [1:0]	tx_ld;
   wire [1:0] 	tx_ld_next;
   
   wire  	uld_rx_ack_next;
   wire 	uld_rx_data;
 		
   wire  	ld_tx_ack_next;
   wire 	ld_tx_data;

   reg 		next_fake;
   
   integer 	_tx_delay;
   integer 	_rx_delay;

   reg [7:0] 	hold;
   

   // require uld_rx_req to deassert before sending next char
   always @(posedge clk or posedge reset)
     if (reset)
       begin
	  rx_uld <= 2'b00;
	  uld_rx_ack <= 0;
       end
     else
       begin
	  rx_uld <= rx_uld_next;
	  uld_rx_ack <= uld_rx_ack_next;
       end

   assign rx_uld_next =
		       (rx_uld == 2'b00 && ~uld_rx_req) ? 2'b00 :
		       (rx_uld == 2'b00 && uld_rx_req)  ? 2'b01 :
		       (rx_uld == 2'b01 && uld_rx_req)  ? 2'b01 :
		       (rx_uld == 2'b01 && ~uld_rx_req)  ? 2'b10 :
		       2'b00;

   assign uld_rx_ack_next = rx_uld == 2'b01;
   assign uld_rx_data = rx_uld == 2'b00 && uld_rx_req;
   

   // load output reg from holding cell
   always @ (posedge clk or posedge reset)
     if (reset)
       begin
   	  rx_data <= 0;
	  rx_empty = 1;
       end
     else
       begin
	  if (uld_rx_data)
	    begin
   	       rx_data <= hold;
	       rx_empty = 1;
`ifdef debug
	       $display("fake_uart: rx_data = 0x%2x; empty %t", hold, $time);
`endif
	       _rx_delay = 200;
	    end
       end

   // delay
   always @ (posedge clk or posedge reset)
     if (reset)
       begin
	  next_fake <= 0;
	  if (fake_count >= 0)
	    _rx_delay = 10000000;
	  else
	    _rx_delay = 0;
       end
     else
       begin
	  if (_rx_delay > 0)
	    begin
	       next_fake <= 0;
	       _rx_delay = _rx_delay - 1;
	    end

	  if (_rx_delay == 0 && rx_empty == 1 && ~fake_done)
	    begin
	       next_fake <= 1;
	       _rx_delay = 1000;
`ifdef debug
	       $display("fake_uart: next %t", $time);
`endif
	    end

       end

   
   // require tx_ld_req to deassert before accepting next char
   always @(posedge clk or posedge reset)
     if (reset) begin
	tx_ld <= 2'b00;
	ld_tx_ack <= 0;
     end
     else begin
	tx_ld <= tx_ld_next;
	ld_tx_ack <= ld_tx_ack_next;
     end

   assign tx_ld_next =
		      (tx_ld == 2'b00 && ~ld_tx_req) ? 2'b00 :
		      (tx_ld == 2'b00 && ld_tx_req)  ? 2'b01 :
		      (tx_ld == 2'b01 && ld_tx_req)  ? 2'b01 :
		      (tx_ld == 2'b01 && ~ld_tx_req)  ? 2'b10 :
		      2'b00;

   assign ld_tx_ack_next = tx_ld == 2'b01;
   assign ld_tx_data = tx_ld == 2'b01;
   
   always @ (posedge clk or posedge reset)
     if (reset)
       begin
	  tx_empty <= 1;
	  _tx_delay = 0;
       end
     else
       begin
	  if (ld_tx_data && tx_empty)
	    begin
	       tx_empty <= 0;
	       _tx_delay = 100;
`ifdef debug
	       $display("fake_uart: loading 0x%x %t", tx_data, $time);
`endif
	    end
	  else
	    if (_tx_delay > 0)
	      _tx_delay = _tx_delay - 1;

	  if (_tx_delay == 0 && tx_empty == 0)
	    begin
	       tx_empty <= 1;
	    end
       end

//--------------------------

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
     
   initial
     begin
`ifdef no_fake_input
	fake_count = -1;
`else
	fake_count = 0;
`endif
	fake_done = 0;
     end

   always @(posedge clk)
     if (next_fake)
       begin
	  case (fake_count)
	    -1: ;

`ifdef rsts
	    0: hold = 8'h53; //S
	    1: hold = 8'h54; //T
	    2: hold = 8'h41; //A
	    3: hold = 8'h52; //R
	    4: hold = 8'h54; //T
	    5: hold = 8'h0d; //<ret>
	    6: hold = 8'h30;//0
	    7: hold = 8'h31;//1
	    8: hold = 8'h2d;//-
	    9: hold = 8'h4a;//J
	    10: hold = 8'h41;//A
	    11: hold = 8'h4e;//N
	    12: hold = 8'h2d;//-
	    13: hold = 8'h38;//8
	    14: hold = 8'h35;//5
	    15: hold = 8'h0d;//<ret>
	    16: hold = 8'h31;//1
	    17: hold = 8'h30;//0
	    18: hold = 8'h3a;//:
	    19: hold = 8'h31;//1
	    20: hold = 8'h30;//0
	    21: hold = 8'h0d;//<ret>
`endif
`ifdef v6_unix
	    0: hold = 8'h72; //r
	    1: hold = 8'h6b; //k
	    2: hold = 8'h75; //u
	    3: hold = 8'h6e; //n
	    4: hold = 8'h69; //i
	    5: hold = 8'h78; //x
	    6: hold = 8'h2e; //.
	    7: hold = 8'h34; //4
	    8: hold = 8'h30; //0
	    9: hold = 8'h0d; //<ret>
`endif
`ifdef v7_unix
	    0: hold = 8'h62; //b
	    1: hold = 8'h6f; //o
	    2: hold = 8'h6f; //o
	    3: hold = 8'h74; //t
	    4: hold = 8'h0d; //<ret>
	    5: hold = 8'h72; //r
	    6: hold = 8'h6b; //k
	    7: hold = 8'h28; //(
	    8: hold = 8'h30; //0
	    9: hold = 8'h2c; //,
	    10: hold = 8'h30; //0
	    11: hold = 8'h29; //)
	    12: hold = 8'h72; //r
	    13: hold = 8'h6b; //k
	    14: hold = 8'h75; //u
	    15: hold = 8'h6e; //n
	    16: hold = 8'h69; //i
	    17: hold = 8'h78; //x
	    18: hold = 8'h0d; //<ret>
`endif
`ifdef bsd29_unix
	    0: hold = 8'h72; //r
	    1: hold = 8'h6b; //k
	    2: hold = 8'h28; //(
	    3: hold = 8'h30; //0
	    4: hold = 8'h2c; //,
	    5: hold = 8'h30; //0
	    6: hold = 8'h29; //)
	    7: hold = 8'h72; //r
	    8: hold = 8'h6b; //k
	    9: hold = 8'h75; //u
	    10: hold = 8'h6e; //n
	    11: hold = 8'h69; //i
	    12: hold = 8'h78; //x
	    13: hold = 8'h0d; //<ret>
`endif	    
`ifdef bsd29_unix
	    0: hold = 8'h72; //r
	    1: hold = 8'h6b; //k
	    2: hold = 8'h28; //(
	    3: hold = 8'h30; //0
	    4: hold = 8'h2c; //,
	    5: hold = 8'h30; //0
	    6: hold = 8'h29; //)
	    7: hold = 8'h72; //r
	    8: hold = 8'h6b; //k
	    9: hold = 8'h75; //u
	    10: hold = 8'h6e; //n
	    11: hold = 8'h69; //i
	    12: hold = 8'h78; //x
	    13: hold = 8'h0d; //<ret>
`endif	    
`ifdef bsd211_unix
	    0: hold = 8'h72; //r
	    1: hold = 8'h6b; //k
	    2: hold = 8'h28; //(
	    3: hold = 8'h30; //0
	    4: hold = 8'h2c; //,
	    5: hold = 8'h30; //0
	    6: hold = 8'h29; //)
	    7: hold = 8'h75; //u
	    8: hold = 8'h6e; //n
	    9: hold = 8'h69; //i
	    10: hold = 8'h78; //x
	    11: hold = 8'h0d; //<ret>
`endif	    
`ifdef test_input
	    0: hold = 8'h74; //t
	    1: hold = 8'h68; //h
	    2: hold = 8'h69; //i
	    3: hold = 8'h73; //s
	    4: hold = 8'h20; //<sp>
	    5: hold = 8'h69; //i
	    6: hold = 8'h73; //s
	    7: hold = 8'h20; //<sp>
	    8: hold = 8'h61; //a
	    9: hold = 8'h20; //<sp>
	    10: hold = 8'h74; //t
	    11: hold = 8'h65; //e
	    12: hold = 8'h73; //s
	    13: hold = 8'h74; //t
	    14: hold = 8'h0d; //<ret>
	    15: hold = 8'h0a; //<lf>
	    16: hold = 8'h03; //^C
`endif
`ifdef dir
//	    0: hold = 8'h64; //d
//	    1: hold = 8'h69; //i
//	    2: hold = 8'h72; //r
//	    3: begin
//	       hold = 8'h0d; //<ret>
//	       fake_done = 1;
//	    end
	    3: begin
	       hold = 8'h64;
	       fake_done = 1;
	    end
`endif
	    default: ;
	  endcase // case(fake_count)

	  if (fake_count >= 0)
	    begin
	       $display("fake_uart: holding fake #%0d = 0x%2x %t", fake_count, hold, $time);
	       fake_count = fake_count + 1;
	       rx_empty = 0;
	    end

	  if (fake_done)
	    fake_count = -1;
       end
	  
endmodule // uart
