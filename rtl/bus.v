// bus.v
// simple pdp11 bus interface
// copyright Brad Parker <brad@heeltoe.com> 2009

`include "iopage.v"

module bus(clk, brgclk, reset, bus_addr, bus_data_in, bus_data_out,
	   bus_rd, bus_wr, bus_byte_op,
	   bus_arbitrate, bus_ack, bus_error,
	   bus_int, bus_int_ipl, bus_int_vector, interrupt_ack_ipl,
	   ram_addr, ram_data_in, ram_data_out, ram_rd, ram_wr, ram_byte_op,
	   pxr_wr, pxr_rd, pxr_addr, pxr_data_in, pxr_data_out, pxr_trap,
	   ide_data_bus, ide_dior, ide_diow, ide_cs, ide_da,
	   psw, psw_io_wr, switches, rs232_tx, rs232_rx
,rk_state
	  );

   input clk;
   input brgclk;
   input reset;
   input [21:0] bus_addr;
   input [15:0] bus_data_in;
   input 	bus_rd, bus_wr, bus_byte_op;
   input 	bus_arbitrate;
   output [15:0] bus_data_out;

output [4:0] rk_state;
   output 	 bus_ack;
   output 	 bus_error;
   output 	 bus_int;
   output [7:0]  bus_int_ipl;
   output [7:0]  bus_int_vector;
   input [7:0] 	 interrupt_ack_ipl;

   output [21:0]  ram_addr;
   input [15:0]   ram_data_in;
   output [15:0]  ram_data_out;
   output 	  ram_rd, ram_wr, ram_byte_op;

   output 	  pxr_wr;
   output 	  pxr_rd;
   output [7:0]	  pxr_addr;
   input [15:0] pxr_data_in;
   output [15:0] pxr_data_out;
   input 	 pxr_trap;
   
   inout [15:0] ide_data_bus;
   output 	ide_dior, ide_diow;
   output [1:0] ide_cs;
   output [2:0] ide_da;

   input [15:0] psw;
   output 	psw_io_wr;

   input [15:0] switches;

   output	rs232_tx;
   input	rs232_rx;
   
   //
   wire 	ram_access;
   wire 	iopage_access;

   wire [15:0] 	iopage_out;

   wire 	iopage_rd, iopage_wr;
   wire 	dma_rd, dma_wr, dma_req, dma_ack;
   wire [17:0] 	dma_addr;
   
   wire [15:0] 	dma_data_out;

   wire 	grant_cpu, grant_dma;
   reg [2:0] 	grant_state;
   reg [3:0] 	grant_count;
   wire [2:0] 	grant_state_next;
 	  
   //
`ifdef xxx
   assign 	ram_access = bus_addr[21:16] == 6'b0 &&
			     bus_addr[15:13] != 3'b111;
   
   assign 	iopage_access = bus_addr[15:13] == 3'b111;
`else
   assign 	iopage_access = (bus_addr[21:13] == 9'o776) ||
				(bus_addr[21:13] == 9'o777);

   assign 	ram_access = ~iopage_access;
`endif
   
   assign 	iopage_rd = bus_rd & iopage_access;
   assign 	iopage_wr = bus_wr & iopage_access;

   assign bus_data_out = ram_access ? ram_data_in :
			 iopage_access ? iopage_out : 16'hffff/*16'b0*/;

   assign ram_addr = grant_cpu ? bus_addr : {6'b0, dma_addr[15:0]};
   assign ram_data_out = grant_cpu ? bus_data_in : dma_data_out;
   assign ram_rd = grant_cpu ? (bus_rd & ram_access) : dma_rd;
   assign ram_wr = grant_cpu ? (bus_wr & ram_access) : dma_wr;
   assign ram_byte_op = grant_cpu ? bus_byte_op : 1'b0;


`ifdef debug_bus
   always @(posedge clk)
     if (ram_access)
       begin
	  if (bus_wr) $display("bus: ram write %o <- %o", bus_addr,bus_data_in);
	  if (bus_rd) $display("bus: ram read %o -> %o", bus_addr,bus_data_out);
	  if (bus_wr || bus_rd)
	    $display("     ram_rd %o, ram_wr %o ram_byte_op %o",
		     ram_rd, ram_wr, ram_byte_op);
       end

//   always @(posedge clk)
//     if (iopage_access)
//       begin
//	  if (bus_wr) $display("bus: io write %o <- %o", bus_addr,bus_data_in);
//	  if (bus_rd) $display("bus: io read %o -> %o", bus_addr,bus_data_out);
//       end
`endif
   
`ifdef debug_bus_dma
   always @(posedge clk)
     if (dma_ack)
       begin
	  if (bus_wr) $display("bus: ram write %o <- %o", bus_addr,bus_data_in);
	  if (bus_rd) $display("bus: ram read %o -> %o", bus_addr,bus_data_out);
	  if (dma_rd || dma_wr)
	    $display("     dma_rd %o dma_wr %o ram_data_in %o, dma_data_out %o",
		     dma_rd, dma_wr, ram_data_in, dma_data_out);
       end
`endif

`ifdef debug_io
   always @(posedge clk)
     if (iopage_access)
       begin
	  if (bus_wr)
	    $display("bus: iopage write %o <- %o (byte %o, error %o)",
		     bus_addr, bus_data_in, bus_byte_op, bus_error);
	  if (bus_rd)
	    $display("bus: iopage read %o -> %o (byte %o, error %o)",
		     bus_addr, bus_data_out, bus_byte_op, bus_error);
       end
`endif

`ifdef debug
   always @(posedge clk)
     if (iopage_access)
       begin
	  if (bus_wr && bus_error)
	    $display("bus: iopage buserr write %o <- %o (byte %o)",
		     bus_addr, bus_data_in, bus_byte_op);
	  if (bus_rd && bus_error)
	    $display("bus: iopage buserr read %o -> %o (byte %o)",
		     bus_addr, bus_data_out, bus_byte_op);
       end
`endif

`ifdef debug_bus_int
   always @(posedge clk)
     if (bus_int)
       $display("bus: XXX bus interrupt, vector %o", bus_int_vector);
`endif
   
   // simple arbiter
   // wait for dma request and cpu to allow
   // then run 4 dma cycles
   always @ (posedge clk)
     if (reset)
       grant_state <= 3'd0;
     else
       begin
	  grant_state <= grant_state_next;
`ifdef debug_bus_state
	  $display("grant_state %b", grant_state_next);
`endif
       end

   assign grant_state_next =
		(grant_state == 3'd0 && dma_req && bus_arbitrate) ? 3'd1 :
		(grant_state == 3'd1 && dma_req) ? 3'd2 :
		(grant_state == 3'd2 && dma_req) ? 3'd3 :
		(grant_state == 3'd3 && dma_req) ? 3'd4 :
		3'd0;

   assign grant_cpu = grant_state == 3'd0;
   assign grant_dma = grant_state != 3'd0;
   
   assign bus_ack = grant_cpu;
   assign dma_ack = grant_dma;

   wire   iopage_bus_error;

   assign bus_error = iopage_bus_error;
	
   iopage iopage1(.clk(clk),
		  .brgclk(brgclk),
		  .reset(reset),
		  .address(bus_addr),
		  .data_in(bus_data_in),
		  .data_out(iopage_out),
		  .iopage_rd(iopage_rd),
		  .iopage_wr(iopage_wr),
		  .iopage_byte_op(bus_byte_op),

		  .no_decode(iopage_bus_error),
		  .interrupt(bus_int),
		  .interrupt_ipl(bus_int_ipl),
		  .vector(bus_int_vector),
		  .ack_ipl(interrupt_ack_ipl),

		  // mmu_regs
		  .pxr_wr(pxr_wr),
		  .pxr_rd(pxr_rd),
		  .pxr_addr(pxr_addr),
		  .pxr_data_in(pxr_data_in),
		  .pxr_data_out(pxr_data_out),
		  .pxr_trap(pxr_trap),
		  
		  // external connection to ide drive
		  .ide_data_bus(ide_data_bus),
		  .ide_dior(ide_dior), .ide_diow(ide_diow),
		  .ide_cs(ide_cs), .ide_da(ide_da),

		  // psw i/o
		  .psw(psw), .psw_io_wr(psw_io_wr),

		  // switches
		  .switches(switches),

		  // rs-232
		  .rs232_tx(rs232_tx), .rs232_rx(rs232_rx),
		  
		  // dma from device to memory
		  .dma_req(dma_req),
		  .dma_ack(dma_ack),
		  .dma_addr(dma_addr),
		  .dma_data_in(ram_data_in),
		  .dma_data_out(dma_data_out),
		  .dma_rd(dma_rd),
		  .dma_wr(dma_wr),
		  .rk_state(rk_state)
		  );

endmodule
