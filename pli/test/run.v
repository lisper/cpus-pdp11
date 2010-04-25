`timescale 1ns / 1ns

module thing(clk, a1, a2);
   input clk;
   input a1;
   output a2;
   reg 	  a2;

   always @(posedge clk)
     begin
	a2 <= a1;
     end
   
endmodule
   
module test;

   reg clk, reset;

   always
     begin
	#10 clk = 0;
	#10 clk = 1;
     end

   wire w1, w2;

   thing thing1(clk, w1, w2);

   wire [15:0] dout;
   
   wire [17:0] dma_addr_out;
   wire [17:0] dma_addr;
   wire        dma_rd, dma_wr, dma_req, interrupt;
   wire [7:0]  vector;
   wire        decode;
   wire [15:0] data_out;

   always @(posedge clk)
     begin
	$pli_test(w1, w2);
//	$pli_ram(clk, 1'b0, 1'b0, 1'b0, dout, 1'b0, 1'b0, 1'b0);
//	$pli_rk(clk, 1'b0, 1'b0, 1'b0, data_out, decode,
//		1'b0, 1'b0, 1'b0,
//		interrupt, 1'b0, vector,
//		1'b0, 1'b0, 1'b0, 1'b0, 1'b0,
//		dma_req, 1'b0, dma_addr, 1'b0, 1'b0,
//		dma_rd, dma_wr);
     end

endmodule
