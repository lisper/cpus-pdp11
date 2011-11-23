// iopage.v
// iopage decoding and muxing
// copyright Brad Parker <brad@heeltoe.com> 2009


`define use_rk_model 1
//`define use_rk_pli

module iopage(clk, brgclk, reset, address, data_in, data_out,
	      iopage_rd, iopage_wr, iopage_byte_op,
	      no_decode, interrupt, interrupt_ipl, ack_ipl, vector,
	      pxr_wr, pxr_rd, pxr_be,
	      pxr_addr, pxr_data_in, pxr_data_out, pxr_trap,
	      ide_data_bus, ide_dior, ide_diow, ide_cs, ide_da,
	      psw, psw_io_wr,
	      switches,
	      rs232_tx, rs232_rx,
   	      dma_req, dma_ack, dma_addr, dma_data_in, dma_data_out,
	      dma_rd, dma_wr
,rk_state
	      );

   input clk;
   input brgclk;
   input reset;
   input [21:0] address;
   input [15:0] data_in;
   input 	iopage_rd, iopage_wr, iopage_byte_op;
   output [15:0] data_out;

output [4:0] rk_state;
   output 	 no_decode;
   output 	 interrupt;
   output [7:0]	 interrupt_ipl;
   output [7:0]  vector;
   input [7:0] 	 ack_ipl;

   output 	 pxr_wr;
   output 	 pxr_rd;
   output [1:0]  pxr_be;
   output [7:0]  pxr_addr;
   input [15:0]  pxr_data_in;
   output [15:0] pxr_data_out;
   input 	 pxr_trap;

   inout [15:0] ide_data_bus;
   output 	ide_dior, ide_diow;
   output [1:0] ide_cs;
   output [2:0] ide_da;

   input [15:0] psw;
   output 	psw_io_wr;

   input [15:0] switches;

   input	rs232_rx;
   output	rs232_tx;

   output 	dma_req;
   input 	dma_ack;
   output [17:0] dma_addr;
   output [15:0]  dma_data_out;
   input [15:0] dma_data_in;
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
		     16'h002f;
//		     16'b0;

   assign good_decode = bootrom_decode | mmu_decode | tt_decode | clk_decode |
			sr_decode | psw_decode | rk_decode;

   assign no_decode = (iopage_rd | iopage_wr) & ~good_decode;


   wire clk_interrupt, tt_interrupt, rk_interrupt;
   wire clk_interrupt_ack, tt_interrupt_ack, rk_interrupt_ack;

   wire mmu_trap;

   wire [7:0] tt_vector, clk_vector, rk_vector, mmu_vector;

   assign interrupt = tt_interrupt | clk_interrupt | rk_interrupt;

   // note priority order
   assign interrupt_ipl = { 1'b0,
			    clk_interrupt, rk_interrupt, tt_interrupt,
			    4'b0000 };

   assign clk_interrupt_ack = ack_ipl[6];
   assign rk_interrupt_ack = ack_ipl[5];
   assign tt_interrupt_ack = ack_ipl[4];

   // note: these need to be in priority order
   assign vector = 
		   clk_interrupt ? clk_vector :
		   rk_interrupt ? rk_vector :
		   tt_interrupt ? tt_vector :
		   8'b0;

   
   bootrom bootrom(.clk(clk),
		    .reset(reset),
		    .iopage_addr(iopage_addr),
		    .data_in(data_in),
		    .data_out(bootrom_data_out),
		    .decode(bootrom_decode),
		    .iopage_rd(iopage_rd),
		    .iopage_wr(iopage_wr),
		    .iopage_byte_op(iopage_byte_op));

   mmu_regs mmu_regs(.clk(clk),
		      .reset(reset),
		      .iopage_addr(iopage_addr),
		      .data_in(data_in),
		      .data_out(mmu_data_out),
		      .decode(mmu_decode),
		      .trap(mmu_trap),
		      .vector(mmu_vector),
		      .iopage_rd(iopage_rd),
		      .iopage_wr(iopage_wr),
		      .iopage_byte_op(iopage_byte_op),
		      .pxr_wr(pxr_wr),
		      .pxr_rd(pxr_rd),
		      .pxr_be(pxr_be),
		      .pxr_addr(pxr_addr),
		      .pxr_data_in(pxr_data_in),
		      .pxr_data_out(pxr_data_out),
		      .pxr_trap(pxr_trap));

   tt_regs tt_regs(.clk(clk),
		    .brgclk(brgclk),
		    .reset(reset),
		    .iopage_addr(iopage_addr),
		    .data_in(data_in),
		    .data_out(tt_data_out),
		    .decode(tt_decode),
		    .iopage_rd(iopage_rd),
		    .iopage_wr(iopage_wr),
		    .iopage_byte_op(iopage_byte_op),
		    .interrupt(tt_interrupt),
		    .interrupt_ack(tt_interrupt_ack),
		    .vector(tt_vector),
		    // connection to rs-232
		    .rs232_tx(rs232_tx),
		    .rs232_rx(rs232_rx));

   clk_regs clk_regs(.clk(clk),
		      .reset(reset),
		      .iopage_addr(iopage_addr),
		      .data_in(data_in),
		      .data_out(clk_data_out),
		      .decode(clk_decode),
		      .iopage_rd(iopage_rd),
		      .iopage_wr(iopage_wr),
		      .iopage_byte_op(iopage_byte_op),
		      .interrupt(clk_interrupt),
		      .interrupt_ack(clk_interrupt_ack),
		      .vector(clk_vector));

   sr_regs sr_regs(.clk(clk),
		    .reset(reset),
		    .iopage_addr(iopage_addr),
		    .data_in(data_in),
		    .data_out(sr_data_out),
		    .decode(sr_decode),
		    .iopage_rd(iopage_rd),
		    .iopage_wr(iopage_wr),
		    .iopage_byte_op(iopage_byte_op),
		    .switches(switches));

   psw_regs psw_regs(.clk(clk),
		     .reset(reset),
		     .iopage_addr(iopage_addr),
		     .data_in(data_in),
		     .data_out(psw_data_out),
		     .decode(psw_decode),
		     .iopage_rd(iopage_rd),
		     .iopage_wr(iopage_wr),
		     .iopage_byte_op(iopage_byte_op),

		     // psw i/o
		     .psw_in(psw),
		     .psw_io_wr(psw_io_wr));

`ifdef use_rk_model
   rk_regs rk_regs(.clk(clk),
		    .reset(reset),
		    .iopage_addr(iopage_addr),
		    .data_in(data_in),
		    .data_out(rk_data_out),
		    .decode(rk_decode),
		    .iopage_rd(iopage_rd),
		    .iopage_wr(iopage_wr),
		    .iopage_byte_op(iopage_byte_op),

		    .interrupt(rk_interrupt),
		    .interrupt_ack(rk_interrupt_ack),
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
		    .dma_rd(dma_rd), .dma_wr(dma_wr),

		    .rk_state_out(rk_state)
		    );
`endif

`ifdef use_rk_pli
   always @(posedge clk or iopage_addr)
     begin
	$pli_rk(clk, reset, iopage_addr, data_in, rk_data_out, rk_decode,
		iopage_rd, iopage_wr, iopage_byte_op,
		rk_interrupt, rk_interrupt_ack, rk_vector,
		ide_data_bus, ide_dior,ide_diow, ide_cs, ide_da,
		dma_req, dma_ack, dma_addr, dma_data_in, dma_data_out,
		dma_rd, dma_wr);
     end
`endif
   
endmodule
