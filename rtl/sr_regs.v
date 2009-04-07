// sr_regs.v

module sr_regs(clk, reset, iopage_addr, data_in, data_out, decode,
	       iopage_rd, iopage_wr, iopage_byte_op);

   input clk;
   input reset;
   input [12:0] iopage_addr;
   input [15:0] data_in;
   input 	iopage_rd, iopage_wr, iopage_byte_op;
   output [15:0] data_out;
   reg [15:0] 	 data_out;
   output 	 decode;

   assign 	 decode = (iopage_addr == 13'o17570);
   
   always @(clk or decode or iopage_addr or iopage_rd or iopage_byte_op)
     begin
	if (decode)
	  case (iopage_addr)
	    13'o17570: data_out = 0;
	  endcase
     end
   
   
endmodule

