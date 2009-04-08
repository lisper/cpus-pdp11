// psw_regs.v

module psw_regs(clk, reset, iopage_addr, data_in, data_out, decode,
		iopage_rd, iopage_wr, iopage_byte_op,
		psw, psw_io_wr);

   input clk;
   input reset;
   input [12:0] iopage_addr;
   input [15:0] data_in;
   input 	iopage_rd, iopage_wr, iopage_byte_op;
   output [15:0] data_out;
   reg [15:0] 	 data_out;
   output 	 decode;

   input [15:0]  psw;
   output 	 psw_io_wr;

   //
   assign decode = (iopage_addr == 13'o17776);
   assign psw_io_wr = iopage_wr && decode;
   
   always @(clk or iopage_addr or iopage_rd or psw)
     begin
	if (decode)
	  case (iopage_addr)
	    13'o17776: data_out = psw;
	    default: data_out = 16'b0;
	  endcase
     end

endmodule

