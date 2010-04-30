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

//   // 17760 - 17777
//   assign decode = iopage_addr == 13'b1_111_111_11x_xxx;
   //
//   assign decode = (iopage_addr == 13'o17776 || iopage_addr == 13'o17777) ||
//		   (iopage_addr == 13'o17774 || iopage_addr == 13'o17775);
   assign decode = (iopage_addr == 13'o17776 || iopage_addr == 13'o17777) ||
		   (iopage_addr == 13'o17774 || iopage_addr == 13'o17775) ||
   		   (iopage_addr == 13'o17772 || iopage_addr == 13'o17773) ||
      		   (iopage_addr == 13'o17770 || iopage_addr == 13'o17771) ||
		   (iopage_addr == 13'o17760 || iopage_addr == 13'o17762) ||
		   (iopage_addr == 13'o17766 || iopage_addr == 13'o17767);
   
   assign psw_io_wr = iopage_wr && decode;
   
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

