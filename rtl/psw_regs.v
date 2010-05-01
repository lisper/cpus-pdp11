// psw_regs.v
//
// 177776 psw

module psw_regs(clk, reset, iopage_addr, data_in, data_out, decode,
		iopage_rd, iopage_wr, iopage_byte_op,
		psw_in, psw_io_wr);

   input clk;
   input reset;
   input [12:0] iopage_addr;
   input [15:0] data_in;
   input 	iopage_rd, iopage_wr, iopage_byte_op;
   input [15:0]  psw_in;

   output [15:0] data_out;
   reg [15:0] 	 data_out;
   output 	 decode;
   output 	 psw_io_wr;

   reg [15:0] 	 psw;
   
   //
   assign decode = (iopage_addr == 13'o17776 || iopage_addr == 13'o17777) ||
		   (iopage_addr == 13'o17774 || iopage_addr == 13'o17775);
   
   assign psw_io_wr = iopage_wr && decode;

   alway @(posedge clk)
     if (reset)
       psw <= 0;
     else
       psw <= psw_in;
   
   always @(clk or decode or iopage_addr or iopage_rd or psw)
     begin
	if (decode)
	  case (iopage_addr)
	    13'o17776: data_out = psw;
	    default: data_out = 16'b0;
	  endcase
	else
	  data_out = 16'b0;
     end

`ifdef debug
   always @(posedge clk)
       if (iopage_wr)
	 case (iopage_addr)
	    13'o17776, 13'o17777:
	      begin
		 $display("psw: write %o", data_in);
	      end
	 endcase
`endif

endmodule

