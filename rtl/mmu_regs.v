module mmu_regs (clk, reset, iopage_addr, data_in, data_out, decode,
		 iopage_rd, iopage_wr, iopage_byte_op);
   
   input clk;
   input reset;
   input [12:0] iopage_addr;
   input [15:0] data_in;
   input 	iopage_rd, iopage_wr, iopage_byte_op;
   output [15:0] data_out;
//   reg [15:0] 	 data_out;
   output 	 decode;

   reg [15:0] 	 mmr0, mmr1, mmr2, mmr3;

   assign 	 decode = (iopage_addr == 13'o17516) |
			  (iopage_addr == 13'o17572) |
			  (iopage_addr == 13'o17574) |
			  (iopage_addr == 13'o17576);

   assign data_out = 16'b0;

   always @(posedge clk)
     if (reset)
       begin
	  mmr3 <= 0;
       end
     else
       if (iopage_wr)
	 case (iopage_addr)
	   13'o17572: mmr0 <= data_in;
	   13'o17574: mmr1 <= data_in;
	   13'o17576: mmr2 <= data_in;
	   13'o17516: mmr3 <= data_in;
	 endcase
   
endmodule

