// sr_regs.v

module sr_regs(clk, reset, iopage_addr, data_in, data_out, decode,
	       iopage_rd, iopage_wr, iopage_byte_op, switches);

   input clk;
   input reset;
   input [12:0] iopage_addr;
   input [15:0] data_in;
   input 	iopage_rd, iopage_wr, iopage_byte_op;
   output [15:0] data_out;
   reg [15:0] 	 data_out;
   output 	 decode;
   input [15:0]  switches;

   assign 	 decode = (iopage_addr == 13'o17570);
   
   always @(clk or decode or iopage_addr or iopage_rd or switches)
     begin
	if (decode)
	  case (iopage_addr)
	    13'o17570: data_out = switches;
	    default: data_out = 16'b0;
	  endcase
        else
	  data_out = 16'b0;
     end
   
   always @(posedge clk)
       if (iopage_wr)
	 case (iopage_addr)
	    13'o17570:
	      begin
`ifdef debug
		 $display("display: write %o", data_in);
`endif
	      end
	 endcase
   
endmodule

