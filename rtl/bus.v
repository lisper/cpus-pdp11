// bus.v
// simple pdp11 bus interface
// copyright Brad Parker <brad@heeltoe.com> 2009

//
// 22-bit
//   1   7   7 4-7   X   X   X   X		17760000-17777777
//   2 211 111 111 11
//   1 098 765 432 109 876 543 210
//   1 111 111 1xx xxx xxx xxx xxx io-page
// 18-bit
//           7   7 6-7   X   X   X		  776000-  777777
//   x xxx 111 111 11
//   x xxx 765 432 109 876 543 210
//   x xxx 111 1xx xxx xxx xxx xxx io-page
// 16-bit
//           1 6-7   X   X   X   X		  160000-  177777
//   x xxx xx1 111 11
//   x xxx xx5 432 109 876 543 210
//   x xxx xx1 11x xxx xxx xxx xxx io-page
//

//`define debug_buserr

module bus(clk, brgclk, reset, bus_addr, bus_data_in, bus_data_out,
	   bus_rd, bus_wr, bus_byte_op,
	   bus_arbitrate, bus_ack, bus_error,
	   bus_int, bus_int_ipl, bus_int_vector, interrupt_ack_ipl,
	   ram_addr, ram_data_in, ram_data_out, ram_rd, ram_wr, ram_byte_op,
	   pxr_wr, pxr_rd, pxr_be,
	   pxr_addr, pxr_data_in, pxr_data_out, pxr_trap,
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
   output [1:0]	  pxr_be;
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

   wire		bus_req;
 	
   wire 	grant_cpu, grant_dma;
   reg [2:0] 	grant_state;
   wire [2:0] 	grant_state_next;

   reg [21:0] 	hold_bus_addr;
   reg [15:0] 	hold_bus_data_in;
   reg 		hold_bus_rd, hold_bus_wr, hold_bus_byte_op;

`ifdef use_18bit_phys
   // 18 bit physical addressing
   assign 	iopage_access = bus_addr[17:13] == 5'o37;
   
   assign 	ram_access = ~iopage_access;
`else
   // 22 bit physical addressing
   assign 	iopage_access = (bus_addr[21:13] == 9'o777);
  
   assign 	ram_access = ~iopage_access;
`endif
   
   assign 	iopage_rd = grant_cpu & /*hold_*/bus_rd & iopage_access;
   assign 	iopage_wr = grant_cpu & /*hold_*/bus_wr & iopage_access;

   assign bus_data_out = ram_access ? ram_data_in :
			 iopage_access ? iopage_out : 16'hffff/*16'b0*/;

   wire   cpu_drives;

   assign cpu_drives = grant_state < 3'd4 /*grant_cpu*/;
   
   assign ram_addr = cpu_drives ? /*hold_*/bus_addr : {4'b0, dma_addr};
   assign ram_data_out = cpu_drives ? /*hold_*/bus_data_in : dma_data_out;
   assign ram_rd = grant_cpu ? (/*hold_*/bus_rd & ram_access) : dma_rd;
   assign ram_wr = grant_cpu ? (/*hold_*/bus_wr & ram_access) : dma_wr;
   assign ram_byte_op = grant_cpu ? /*hold_*/bus_byte_op : 1'b0;


`ifdef debug_bus_all
   always @(posedge clk)
     if (bus_wr || bus_rd)
       begin
	  if (bus_wr) $display("bus: write %o <- %o", bus_addr,bus_data_in);
	  if (bus_rd) $display("bus: read %o -> %o", bus_addr,bus_data_out);
	  $display("     ram_rd %o, ram_wr %o ram_byte_op %o",
		   ram_rd, ram_wr, ram_byte_op);
       end
`endif
   
`ifdef debug_bus_ram
   always @(posedge clk)
     if (ram_access)
       begin
	  if (bus_wr) $display("bus: ram write %o <- %o %o%o%o",
			       bus_addr, bus_data_in,
			       ram_rd, ram_wr, ram_byte_op);
	  if (bus_rd) $display("bus: ram read %o -> %o %o%o%o",
			       bus_addr, bus_data_out,
			       ram_rd, ram_wr, ram_byte_op);
       end
`endif
   
`ifdef debug_bus_io
   always @(posedge clk)
     if (iopage_access)
       begin
	  if (bus_wr) $display("bus: io write %o <- %o", bus_addr,bus_data_in);
	  if (bus_rd) $display("bus: io read %o -> %o", bus_addr,bus_data_out);
       end
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

`ifdef debug_buserr
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
       grant_state <= grant_state_next;

   assign grant_state_next =
		// cpu 0-3
		(grant_state == 3'd0 && dma_req && bus_arbitrate) ? 3'd4 :
		(grant_state == 3'd0 && bus_req) ? 3'd1 :
		(grant_state == 3'd1) ? 3'd0 :
		// dma 4-7
		(grant_state == 3'd4 && dma_req) ? 3'd5 :
		(grant_state == 3'd5 && dma_req) ? 3'd6:
		(grant_state == 3'd6 && dma_req) ? 3'd7 :
		3'd0;

`ifdef debug_bus_state
   always @(posedge clk)
     $display("grant_state %b, cpu %b %b, arb %b, dma %b %b; %t",
	      grant_state, bus_req, bus_ack, bus_arbitrate, dma_req, dma_ack,
	      $time);
`endif
   
   // cpu is requesting bus
   assign bus_req = bus_rd || bus_wr;
   
   assign grant_cpu = grant_state == 3'd1;
   assign grant_dma = grant_state >= 3'd5;

//   reg 	  bus_ack;
//   always @(posedge clk)
//     if (reset)
//       bus_ack <= 0;
//     else
//       bus_ack <= ~bus_req || grant_cpu;
//   assign bus_ack = ~bus_req || grant_cpu;
   assign bus_ack = ~bus_req || grant_cpu;

   assign dma_ack = grant_dma;

   wire   iopage_bus_error;
   wire   ram_bus_error;
   wire   ram_present;

   assign bus_error = iopage_bus_error || ram_bus_error;

   // register request from cpu
   always @(posedge clk)
     if (reset)
       begin
	  hold_bus_addr <= 0;
	  hold_bus_data_in <= 0;
	  hold_bus_rd <= 0;
	  hold_bus_wr <= 0;
	  hold_bus_byte_op <= 0;
       end
     else
       if (grant_state == 3'd0)
	 begin
	    hold_bus_addr <= bus_addr;
	    hold_bus_data_in <= bus_data_in;
	    hold_bus_rd <= bus_rd;
	    hold_bus_wr <= bus_wr;
	    hold_bus_byte_op <= bus_byte_op;
	 end

   
`ifdef sim_56k
   //
   // for diagnostics, pretend we have 56k of ram
   //
   assign ram_present = bus_addr < 22'o160000;
`else
   // 256k
   assign ram_present = bus_addr < 22'o760000;
//   // 128k
//   assign ram_present = bus_addr < 22'o400000;
`endif
   
   assign ram_bus_error = ram_access && (bus_rd || bus_wr) && ~ram_present;

`ifdef debug
   always @(posedge clk)
     if (bus_error)
       begin
	  $display("bus: bus error, io %b, ram %b; bus_addr %o; %t",
		   iopage_bus_error, ram_bus_error, bus_addr, $time);

	  $display("ram_rd %b, ram_wr %b", ram_rd, ram_wr);
	  $display("ram_access %b iopage_access %b bus_addr[21:13] %b",
   	    ram_access, iopage_access, bus_addr[21:13]);
       end
   
`endif
	
   iopage iopage(.clk(clk),
		  .brgclk(brgclk),
		  .reset(reset),
		  .address(/*hold_*/bus_addr),
		  .data_in(/*hold_*/bus_data_in),
		  .data_out(iopage_out),
		  .iopage_rd(iopage_rd),
		  .iopage_wr(iopage_wr),
		  .iopage_byte_op(/*hold_*/bus_byte_op),

		  .no_decode(iopage_bus_error),
		  .interrupt(bus_int),
		  .interrupt_ipl(bus_int_ipl),
		  .vector(bus_int_vector),
		  .ack_ipl(interrupt_ack_ipl),

		  // mmu_regs
		  .pxr_wr(pxr_wr),
		  .pxr_rd(pxr_rd),
		  .pxr_be(pxr_be),
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
