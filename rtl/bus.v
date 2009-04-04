// bus.v
// bus interface to pdp11
// copyright Brad Parker <brad@heeltoe.com> 2009

`include "ram.v"
`include "iopage.v"

module bus(clk, reset, bus_addr, data_in, data_out,
	   bus_rd, bus_wr, bus_byte_op,
	   bus_ack, bus_error, interrupt, vector,

	   ide_data_bus, ide_dior, ide_diow, ide_cs, ide_da,

	   psw, psw_io_wr);

   input clk;
   input reset;
   input [21:0] bus_addr;
   input [15:0] data_in;
   input 	bus_rd, bus_wr, bus_byte_op;
   output [15:0] data_out;

   output 	 bus_ack;
   output 	 bus_error;
   output 	 interrupt;
   output [7:0]  vector;
   
   inout [15:0] ide_data_bus;
   output 	ide_dior, ide_diow;
   output [1:0] ide_cs;
   output [2:0] ide_da;

   input [15:0] psw;
   output 	psw_io_wr;
   
   //
   wire 	ram_ce_n;
   wire 	ram_we_n;
   wire 	ram_access;
   wire 	iopage_access;

   wire [15:0] 	ram_bus_out;
   wire [15:0] 	iopage_out;

   wire [15:0] 	ram_addr;

   
   assign 	ram_access = ~&bus_addr[15:13];
   assign 	iopage_access = &bus_addr[15:13];
   
   assign 	iopage_rd = bus_rd & iopage_access;
   assign 	iopage_wr = bus_wr & iopage_access;

   assign data_out = ram_access ? ram_bus_out :
		     iopage_access ? iopage_out : data_out;

   assign ram_ce_n = ~( ((bus_rd | bus_wr) & ram_access) ||
			(dma_rd | dma_wr) );
   
   assign ram_we_n = ~( (bus_wr & ram_access) || dma_wr );

   assign ram_addr = ram_access ? bus_addr[15:0] : dma_addr[15:0];
   assign ram_data_in = ram_access ? data_in : dma_data_in;
   assign ram_byte_op = ram_access ? bus_byte_op : 1'b0;
   
   ram_4kx16 ram(.A(ram_addr[15:0]),
		 .DI(ram_data_in),
		 .DO(ram_bus_out),
		 .CE_N(ram_ce_n),
		 .WE_N(ram_we_n),
		 .BYTE_OP(ram_byte_op));

   // simple arbiter
   reg 	  grant_cpu, grant_dma;

   always @ (posedge clk or reset)
     if (reset)
       begin
	  grant_cpu <= 1;
	  grant_dma <= 0;
       end
     else
       if (dma_req && !grant_dma)
	 begin
	    grant_cpu <= 0;
	    grant_dma <= 1;
	 end
       else
	 begin
	    grant_cpu <= 1;
	    grant_dma <= 0;
	 end

   assign bus_ack = grant_cpu;
   assign dma_ack = grant_dma;
   
   iopage iopage1(.clk(clk),
		  .reset(reset),
		  .address(bus_addr),
		  .data_in(data_in),
		  .data_out(iopage_out),
		  .iopage_rd(iopage_rd),
		  .iopage_wr(iopage_wr),
		  .iopage_byte_op(bus_byte_op),

		  .no_decode(bus_error),
		  .interrupt(interrupt),
		  .vector(vector),

		  // external connection to ide drive
		  .ide_data_bus(ide_data_bus),
		  .ide_dior(ide_dior), .ide_diow(ide_diow),
		  .ide_cs(ide_cs), .ide_da(ide_da),

		  // psw i/o
		  .psw(psw), .psw_io_wr(psw_io_wr),

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
