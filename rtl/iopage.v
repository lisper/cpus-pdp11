// iopage.v
// iopage decoding and muxing
// copyright Brad Parker <brad@heeltoe.com> 2009

`include "mmu_regs.v"
`include "clk_regs.v"
`include "sr_regs.v"
`include "psw_regs.v"
`include "rk_regs.v"
`include "tt_regs.v"
`include "bootrom.v"

module iopage(clk, reset, address, data_in, data_out,
	      iopage_rd, iopage_wr, iopage_byte_op,

	      no_decode, interrupt, vector,

	      ide_data_bus, ide_dior, ide_diow, ide_cs, ide_da,

	      psw, psw_io_wr,

	      switches,

   	      dma_req, dma_ack, dma_addr, dma_data_in, dma_data_out,
	      dma_rd, dma_wr
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

   input [15:0] switches;

   output 	dma_req;
   input 	dma_ack;
   output [17:0] dma_addr;
   output [15:0]  dma_data_in;
   input [15:0] dma_data_out;
   output 	 dma_rd;
   output 	 dma_wr;
   
   //
   wire [12:0] 	iopage_addr;

   wire [15:0] bootrom_data_out, mmu_data_out, tt_data_out, clk_data_out,
	       sr_data_out, psw_data_out,
	       rk_data_out;

   wire 	bootrom_decode, mmu_decode, tt_decode, clk_decode,
		sr_decode, psw_decode,
		rk_decode;

   wire 	good_decode, no_decode;
 	
   assign 	iopage_addr = address[12:0];

   assign data_out = bootrom_decode ? bootrom_data_out :
		     mmu_decode ? mmu_data_out :
		     tt_decode ? tt_data_out :
		     clk_decode ? clk_data_out :
		     sr_decode ? sr_data_out :
		     psw_decode ? psw_data_out :
		     rk_decode ? rk_data_out :
		     16'b0/*data_out*/;

   assign good_decode = bootrom_decode | mmu_decode | tt_decode | clk_decode |
			sr_decode | psw_decode | rk_decode;

   assign no_decode = (iopage_rd | iopage_wr) & ~good_decode;


   wire tt_interrupt, clk_interrupt, rk_interrupt;
   wire [7:0] tt_vector, clk_vector, rk_vector;

   assign interrupt = tt_interrupt | clk_interrupt | rk_interrupt;

   assign vector = tt_interrupt ? tt_vector :
		     clk_interrupt ? clk_vector :
		     rk_interrupt ? rk_vector :
		     0;

   
   bootrom bootrom1(.clk(clk),
		    .reset(reset),
		    .iopage_addr(iopage_addr),
		    .data_in(data_in),
		    .data_out(bootrom_data_out),
		    .decode(bootrom_decode),
		    .iopage_rd(iopage_rd),
		    .iopage_wr(iopage_wr),
		    .iopage_byte_op(iopage_byte_op));

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
		    .iopage_byte_op(iopage_byte_op),
		    .switches(switches));

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
		    .ide_cs(ide_cs), .ide_da(ide_da),

		    // dma upward to memory
		    .dma_req(dma_req), .dma_ack(dma_ack),
		    .dma_addr(dma_addr),
		    .dma_data_in(dma_data_in),
		    .dma_data_out(dma_data_out),
		    .dma_rd(dma_rd), .dma_wr(dma_wr)
		    );

endmodule