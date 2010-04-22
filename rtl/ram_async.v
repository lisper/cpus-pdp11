//
// interface to async sram
// used on s3board
//
// multiplexes between to high speed SRAMs
//

module ram_async(clk, reset,
		 addr, data_in, data_out, rd, wr, wr_inhibit, byte_op,
		 ram_a, ram_oe_n, ram_we_n,
		 ram1_io, ram1_ce_n, ram1_ub_n, ram1_lb_n,
		 ram2_io, ram2_ce_n, ram2_ub_n, ram2_lb_n);

   input clk;
   input reset;

   input [17:0] addr;
   input [15:0] data_in;
   output [15:0] data_out;
   input 	 rd, wr, wr_inhibit, byte_op;

   output [17:0] ram_a;
   output 	 ram_oe_n, ram_we_n;
   inout [15:0]  ram1_io;
   output 	 ram1_ce_n, ram1_ub_n, ram1_lb_n;
   inout [15:0]  ram2_io;
   output 	 ram2_ce_n, ram2_ub_n, ram2_lb_n;

   wire [15:0]  ram1_io;

   //
   wire 	 ram1_ub, ram1_lb;
   wire 	 ram_wr_short;

   //
   assign ram1_ub = ~byte_op || (byte_op && addr[0]);
   assign ram1_lb = ~byte_op || (byte_op && ~addr[0]);

   assign data_out = ~byte_op ? ram1_io :
		     {8'b0, addr[0] ? ram1_io[15:8] : ram1_io[7:0]};

   //
   assign ram_a = {1'b0, addr[17:1]};
   assign ram_oe_n = ~rd;

   //
   // make sure ram_wr deasserts and
   // also give time (1/2 cycle) for mmu to assert wr_inhibit
   //
   assign ram_wr_short = (wr & ~clk) && ~wr_inhibit;
   assign ram_we_n = ~ram_wr_short;

   //
   assign ram1_io = ~ram_oe_n ? 16'bz :
		    (byte_op ? {data_in[7:0],data_in[7:0]} : data_in);

   assign ram1_ce_n = 1'b0;
   assign ram1_ub_n = ~ram1_ub;
   assign ram1_lb_n = ~ram1_lb;
   
   assign ram2_io = 16'b0;
   assign ram2_ce_n = 1'b1;
   assign ram2_ub_n = 1'b1;
   assign ram2_lb_n = 1'b1;

endmodule

