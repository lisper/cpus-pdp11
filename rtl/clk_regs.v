// clk_regs.v

module clk_regs(clk, reset, iopage_addr, data_in, data_out, decode,
		iopage_rd, iopage_wr, iopage_byte_op,
		interrupt, vector);

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

   reg [15:0] 	 clk_csr;

   assign 	 decode = (iopage_addr == 13'o17546);
   
   always @(clk or iopage_addr or iopage_rd or iopage_byte_op)
     begin
	if (decode)
	  case (iopage_addr)
	    13'o17546: data_out = clk_csr;
	  endcase
     end
   
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

