//

module mmu_regs (clk, reset, iopage_addr, data_in, data_out, decode,
		 iopage_rd, iopage_wr, iopage_byte_op);

   input clk;
   input reset;
   input [12:0] iopage_addr;
   input [15:0] data_in;
   input 	iopage_rd, iopage_wr, iopage_byte_op;
   output [15:0] data_out;
   reg [15:0] 	 data_out;
   output 	 decode;

   reg [15:0] 	 mmr0, mmr1, mmr2, mmr3;

   assign 	 decode = (iopage_addr == 13'o17516) |
			  (iopage_addr == 13'o17572) |
			  (iopage_addr == 13'o17574) |
			  (iopage_addr == 13'o17576);
   
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

`include "clk_regs.v"
`include "sr_regs.v"
`include "psw_regs.v"
`include "rk_regs.v"
`include "tt_regs.v"

module iopage(clk, reset, address, data_in, data_out,
	      iopage_rd, iopage_wr, iopage_byte_op,

	      no_decode, interrupt, vector,

	      ide_data_bus, ide_dior, ide_diow, ide_cs, ide_da,

	      psw, psw_io_wr
	      );

   input clk;
   input reset;
   input [21:0] address;
   input [15:0] data_in;
   input 	iopage_rd, iopage_wr, iopage_byte_op;
   output [15:0] data_out;

   output 	 no_decode;
   output 	 interrupt;
   output [7:0]  vector;

   inout [15:0] ide_data_bus;
   output 	ide_dior, ide_diow;
   output [1:0] ide_cs;
   output [2:0] ide_da;

   input [15:0] psw;
   output 	psw_io_wr;
   
   //
   wire [12:0] 	iopage_addr;

   wire [15:0] mmu_data_out, tt_data_out, clk_data_out,
	       sr_data_out, psw_data_out,
	       rk_data_out;

   wire 	mmu_decode, tt_decode, clk_decode,
		sr_decode, psw_decode,
		rk_decode;

   wire 	good_decode, no_decode;
 	
   assign 	iopage_addr = address[12:0];

   assign data_out = mmu_decode ? mmu_data_out :
		     tt_decode ? tt_data_out :
		     clk_decode ? clk_data_out :
		     sr_decode ? sr_data_out :
		     psw_decode ? psw_data_out :
		     rk_decode ? rk_data_out :
		     data_out;

   assign good_decode = mmu_decode | tt_decode | clk_decode |
			sr_decode | psw_decode | rk_decode;

   assign no_decode = (iopage_rd | iopage_wr) & ~good_decode;


   assign interrupt = tt_interrupt | clk_interrupt | rk_interrupt;

   assign vector = tt_interrupt ? tt_vector :
		     clk_interrupt ? clk_vector :
		     rk_interrupt ? rk_vector :
		     vector;

   
   mmu_regs mmu_regs1(.clk(clk),
		      .reset(reset),
		      .iopage_addr(iopage_addr),
		      .data_in(data_in),
		      .data_out(mmu_data_out),
		      .decode(mmu_decode),
		      .iopage_rd(iopage_rd),
		      .iopage_wr(iopage_wr),
		      .iopage_byte_op(iopage_byte_op));

   tt_regs tt_regs1(.clk(clk),
		    .reset(reset),
		    .iopage_addr(iopage_addr),
		    .data_in(data_in),
		    .data_out(tt_data_out),
		    .decode(tt_decode),
		    .iopage_rd(iopage_rd),
		    .iopage_wr(iopage_wr),
		    .iopage_byte_op(iopage_byte_op),
		    .interrupt(tt_interrupt),
		    .vector(tt_vector),

		    // connection to rs-232
		    .rs232_tx(rs232_tx), .rs232_rx(rs232_rx));

   clk_regs clk_regs1(.clk(clk),
		      .reset(reset),
		      .iopage_addr(iopage_addr),
		      .data_in(data_in),
		      .data_out(clk_data_out),
		      .decode(clk_decode),
		      .iopage_rd(iopage_rd),
		      .iopage_wr(iopage_wr),
		      .iopage_byte_op(iopage_byte_op),
		      .interrupt(clk_interrupt),
		      .vector(clk_vector));

   sr_regs sr_regs1(.clk(clk),
		      .reset(reset),
		      .iopage_addr(iopage_addr),
		      .data_in(data_in),
		      .data_out(sr_data_out),
		      .decode(sr_decode),
		      .iopage_rd(iopage_rd),
		      .iopage_wr(iopage_wr),
		      .iopage_byte_op(iopage_byte_op));

   psw_regs psw_regs1(.clk(clk),
		      .reset(reset),
		      .iopage_addr(iopage_addr),
		      .data_in(data_in),
		      .data_out(psw_data_out),
		      .decode(psw_decode),
		      .iopage_rd(iopage_rd),
		      .iopage_wr(iopage_wr),
		      .iopage_byte_op(iopage_byte_op),

		      // psw i/o
		      .psw(psw),
		      .psw_io_wr(psw_io_wr));

   rk_regs rk_regs1(.clk(clk),
		    .reset(reset),
		    .iopage_addr(iopage_addr),
		    .data_in(data_in),
		    .data_out(rk_data_out),
		    .decode(rk_decode),
		    .iopage_rd(iopage_rd),
		    .iopage_wr(iopage_wr),
		    .iopage_byte_op(iopage_byte_op),

		    .interrupt(rk_interrupt),
		    .vector(rk_vector),

		    // connection to ide drive
		    .ide_data_bus(ide_data_bus),
		    .ide_dior(ide_dior), .ide_diow(ide_diow),
		    .ide_cs(ide_cs), .ide_da(ide_da));

endmodule
