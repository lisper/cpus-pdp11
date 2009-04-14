// bus.v
// bus interface to pdp11
// copyright Brad Parker <brad@heeltoe.com> 2009

`include "ram.v"
`include "iopage.v"

module bus(clk, reset, bus_addr, data_in, data_out,
	   bus_rd, bus_wr, bus_byte_op,
	   bus_arbitrate, bus_ack, bus_error,
	   interrupt, interrupt_ipl, ack_ipl, vector,

	   ide_data_bus, ide_dior, ide_diow, ide_cs, ide_da,

	   psw, psw_io_wr,
	   switches,
	   rs232_tx, rs232_rx
	  );

   input clk;
   input reset;
   input [21:0] bus_addr;
   input [15:0] data_in;
   input 	bus_rd, bus_wr, bus_byte_op;
   input [7:0] 	ack_ipl;
   input 	bus_arbitrate;
   output [15:0] data_out;

   output 	 bus_ack;
   output 	 bus_error;
   output 	 interrupt;
   output [7:0]  interrupt_ipl;
   output [7:0]  vector;
   
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
   wire 	ram_ce_n;
   wire 	ram_we_n;
   wire 	ram_access;
   wire 	iopage_access;

   wire [15:0] 	ram_bus_out;
   wire [15:0] 	iopage_out;

   wire [15:0] 	ram_addr;

   
   assign 	ram_access = bus_addr[21:16] == 6'b0 &&
			     bus_addr[15:13] != 3'b111;
   
   assign 	iopage_access = bus_addr[15:13] == 3'b111;

   wire 	iopage_rd, iopage_wr;
   wire 	dma_rd, dma_wr, dma_req, dma_ack;
   wire [17:0] 	dma_addr;
   
   assign 	iopage_rd = bus_rd & iopage_access;
   assign 	iopage_wr = bus_wr & iopage_access;

   assign data_out = ram_access ? ram_bus_out :
		     iopage_access ? iopage_out : 16'hffff/*16'b0*/;

//   assign ram_ce_n = ~( ((bus_rd | bus_wr) & ram_access) ||
//			(dma_rd | dma_wr) );
   assign ram_ce_n = ~( grant_cpu ? ((bus_rd | bus_wr) & ram_access) :
			(dma_rd | dma_wr) );
   
//   assign ram_we_n = ~( (bus_wr & ram_access) || dma_wr );
   assign ram_we_n = ~( grant_cpu ? (bus_wr & ram_access) : dma_wr );

   wire [15:0] ram_data_in, dma_data_in;
   wire        ram_byte_op;

   wire        grant_cpu, grant_dma;
   reg [2:0] 	  grant_state;
   reg [3:0] 	  grant_count;
   wire [1:0] 	  grant_state_next;
 	  
   assign ram_addr = grant_cpu ? bus_addr[15:0] : dma_addr[15:0];
   assign ram_data_in = grant_cpu ? data_in : dma_data_in;
   assign ram_byte_op = grant_cpu ? bus_byte_op : 1'b0;
   
`ifdef use_ram_model
   ram_16kx16 ram(.clk(clk),
		 .reset(reset),
		 .addr(ram_addr[15:0]),
		 .DI(ram_data_in),
		 .DO(ram_bus_out),
		 .CE_N(ram_ce_n),
		 .WE_N(ram_we_n),
		 .byte_op(ram_byte_op));
`else
   always @(posedge clk or ram_ce_n or ram_we_n or ram_byte_op or ram_addr)
     begin
	$pli_ram(clk, reset, ram_addr[15:0],
		 ram_data_in, ram_bus_out, ram_ce_n, ram_we_n, ram_byte_op);
     end
`endif

`ifdef debug_bus
   always @(posedge clk)
     if (ram_access)
       begin
	  if (bus_wr) $display("bus: ram write %o <- %o", bus_addr, data_in);
	  if (bus_rd) $display("bus: ram read %o -> %o", bus_addr, data_out);

	  if (bus_wr || bus_rd)
	    $display("     ram_ce_n %o, ram_we_n %o ram_byte_op %o",
		     ram_ce_n, ram_we_n, ram_byte_op);
       end
`endif

`define debug_io
`ifdef debug_io
   always @(posedge clk)
     if (iopage_access)
       begin
	  if (bus_wr)
	    $display("bus: iopage write %o <- %o (byte %o, error %o)",
		     bus_addr, data_in, bus_byte_op, bus_error);
	  if (bus_rd)
	    $display("bus: iopage read %o -> %o (byte %o, error %o)",
		     bus_addr, data_out, bus_byte_op, bus_error);
       end
`endif

`ifdef debug
   always @(posedge clk)
     if (iopage_access)
       begin
	  if (bus_wr && bus_error)
	    $display("bus: iopage buserr write %o <- %o (byte %o)",
		     bus_addr, data_in, bus_byte_op);
	  if (bus_rd && bus_error)
	    $display("bus: iopage buserr read %o -> %o (byte %o)",
		     bus_addr, data_out, bus_byte_op);
       end
`endif

`ifdef debug_bus_int
   always @(posedge clk)
     if (interrupt)
       $display("bus: XXX bus interrupt, vector %o", vector);
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
		  .reset(reset),
		  .address(bus_addr),
		  .data_in(data_in),
		  .data_out(iopage_out),
		  .iopage_rd(iopage_rd),
		  .iopage_wr(iopage_wr),
		  .iopage_byte_op(bus_byte_op),

		  .no_decode(iopage_bus_error),
		  .interrupt(interrupt),
		  .interrupt_ipl(interrupt_ipl),
		  .ack_ipl(ack_ipl),
		  .vector(vector),

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
		  .dma_data_in(dma_data_in),
		  .dma_data_out(ram_bus_out),
		  .dma_rd(dma_rd),
		  .dma_wr(dma_wr)
		  );

endmodule
