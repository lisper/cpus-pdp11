//

`include "ram.v"

module ram_sync (clk, reset, addr, data_in, data_out, rd, wr, byte_op);

   input clk;
   input reset;

   input [15:0] addr;
   input [15:0]	data_in;
   output [15:0] data_out;
   input 	 rd, wr, byte_op;

   //
   wire 	 ce_n, we_n;

   assign 	 ce_n = ~(rd || wr);
   assign 	 we_n = ~wr;
   
`ifdef use_ram_model
   ram_16kx16 ram(.clk(clk),
		 .reset(reset),
		 .addr(addr),
		 .DI(data_in),
		 .DO(data_out),
		 .CE_N(ce_n),
		 .WE_N(we_n),
		 .byte_op(byte_op));

   //always @(posedge clk)
   //$display("ce_n %b we_n %b, addr %o", ce_n, we_n, addr);
   
`endif

`ifdef use_ram_pli
   always @(posedge clk or ce_n or we_n or byte_op or addr)
     begin
	$pli_ram(clk, reset, addr, data_in, data_out, ce_n, we_n, byte_op);
     end
`endif
   
endmodule

