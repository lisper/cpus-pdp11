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

   reg [15:0] 	 clk_csr;

   assign 	 decode = (iopage_addr == 13'o17546);
   
   always @(clk or decode or iopage_addr or iopage_rd or clk_csr)
     begin
	if (decode)
	  case (iopage_addr)
	    13'o17546: data_out = clk_csr;
	    default: data_out = 16'b0;
	  endcase
        else
	  data_out = 16'b0;
     end

   assign interrupt = 1'b0;
   assign vector = 8'b0;
   
   always @(posedge clk)
     if (reset)
       begin
	  clk_csr <= 0;
       end
     else
       if (iopage_wr)
	 case (iopage_addr)
	   13'o17546: clk_csr <= data_in;
	 endcase
   
endmodule

