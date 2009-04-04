//

`include "ram.v"
`include "iopage.v"

module memory(clk, reset, mem_addr, data_in, data_out,
	      mem_rd, mem_wr, mem_byte_op,
	      bus_error, interrupt, vector,

	      ide_data_bus, ide_dior, ide_diow, ide_cs, ide_da,

	      psw, psw_io_wr);

   input clk;
   input reset;
   input [21:0] mem_addr;
   input [15:0] data_in;
   input 	mem_rd, mem_wr, mem_byte_op;
   output [15:0] data_out;

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

   wire [15:0] 	ram_mem_out;
   wire [15:0] 	iopage_out;
 	
   assign 	ram_access = ~&mem_addr[15:13];
   assign 	iopage_access = &mem_addr[15:13];
   
   assign 	ram_ce_n = ~((mem_rd | mem_wr) & ram_access);
   assign 	ram_we_n = ~(mem_wr & ram_access);

   assign 	iopage_rd = mem_rd & iopage_access;
   assign 	iopage_wr = mem_wr & iopage_access;

   assign data_out = ram_access ? ram_mem_out :
		     iopage_access ? iopage_out : data_out;
   
   ram_4kx16 ram(.A(mem_addr[15:0]),
		 .DI(data_in),
		 .DO(ram_mem_out),
		 .CE_N(ram_ce_n),
		 .WE_N(ram_we_n),
		 .BYTE_OP(mem_byte_op));

   iopage iopage1(.clk(clk),
		  .reset(reset),
		  .address(mem_addr),
		  .data_in(data_in),
		  .data_out(iopage_out),
		  .iopage_rd(iopage_rd),
		  .iopage_wr(iopage_wr),
		  .iopage_byte_op(mem_byte_op),

		  .no_decode(bus_error),
		  .interrupt(interrupt),
		  .vector(vector),

		  // external connection to ide drive
		  .ide_data_bus(ide_data_bus),
		  .ide_dior(ide_dior), .ide_diow(ide_diow),
		  .ide_cs(ide_cs), .ide_da(ide_da),

		  // psw i/o
		  .psw(psw), .psw_io_wr(psw_io_wr));

endmodule
