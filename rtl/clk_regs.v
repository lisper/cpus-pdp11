// clk_regs.v
//
// simulated KW11L line clock for pdp11
// copyright Brad Parker <brad@heeltoe.com> 2009

module clk_regs(clk, reset, iopage_addr, data_in, data_out, decode,
		iopage_rd, iopage_wr, iopage_byte_op,
		interrupt, interrupt_ack, vector);

   input clk;
   input reset;
   input [12:0] iopage_addr;
   input [15:0] data_in;
   input 	iopage_rd, iopage_wr, iopage_byte_op;
   input 	interrupt_ack;
   
   output [15:0] data_out;
   reg [15:0] 	 data_out;
   output 	 decode;

   output 	 interrupt;
   output [7:0]  vector;

   reg [19:0] 	 counter;
   reg 		 clk_int_enable;
   reg 		 clk_done;
 		 
   assign 	 decode = (iopage_addr == 13'o17546);

   parameter SYS_CLK = 26'd50000000;
   parameter CLK_RATE = 26'd60;
   parameter CLK_DIV = SYS_CLK / CLK_RATE;

   // register read
   always @(clk or decode or iopage_addr or iopage_rd or
	    clk_int_enable or clk_done)
     begin
	if (iopage_rd && decode)
	  case (iopage_addr)
	    13'o17546:
	      begin
		 data_out = {8'b0, clk_done, clk_int_enable, 6'b0};
`ifdef debug
		 $display("clk: read %o",
			  {8'b0, clk_done, clk_int_enable, 6'b0});
`endif
	      end
	    default: data_out = 16'b0;
	  endcase
        else
	  data_out = 16'b0;
     end

   assign interrupt = clk_int_enable && clk_done;
   assign vector = 8'b0100;

   wire   clk_fired;
   assign clk_fired = (counter == CLK_DIV) || (counter == CLK_DIV-1);

   // register write   
   always @(posedge clk)
     if (reset)
       begin
	  clk_int_enable <= 0;
	  clk_done <= 0;
       end
     else
       if (iopage_wr)
	 case (iopage_addr)
	   13'o17546:
	     begin
		clk_int_enable <= data_in[6];
		clk_done <= data_in[7];
`ifdef debug
		$display("clk: write %o", data_in);
`endif
	     end
	 endcase
       else
	 if (clk_fired)
	   begin
	      clk_done <= 1;
`ifdef debug
	      $display("clk: fired");
`endif
	   end
   
   always @(posedge clk)
     if (reset)
       counter <= 0;
     else
       if (counter != CLK_DIV)
	 counter <= counter + 20'd1;
       else
	 counter <= 0;

endmodule

