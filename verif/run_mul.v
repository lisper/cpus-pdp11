
`timescale 1ns / 1ns

`include "mul1616.v"

module test;

   reg clk, reset;
   reg ready;
   wire done;

   reg [15:0] multiplier;
   reg [15:0] multiplicand;
   wire [31:0] product;

   mul1616 mul(.clk(clk), .reset(reset),
	       .ready(ready), .done(done),
	       .multiplier(multiplier),
	       .multiplicand(multiplicand),
	       .product(product)); 

   task mult;
      input [15:0] arg1;
      input [15:0] arg2;

      begin
	 #20 begin
	    multiplicand = arg1;
	    multiplier = arg2;
	    ready = 1;
	 end
	 #120 begin
	    ready = 0;
	 end
      end
   endtask

   task wait_for_mul;
      begin
	 while (done == 1'b0) #10;
	 #40;
      end
   endtask

  initial
    begin
      $timeformat(-9, 0, "ns", 7);

      $dumpfile("mul.vcd");
      $dumpvars(0, test.mul);
    end

  initial
    begin
       clk = 0;
       reset = 0;
       ready = 0;
       
       #1 reset = 1;
       #20 reset = 0;

       mult(16'd2, 16'd1); wait_for_mul;
       mult(16'd2, 16'd2); wait_for_mul;
       mult(16'd10, 16'd10); wait_for_mul;

       mult(-16'd2, 16'd2); wait_for_mul;
       mult(-16'd2, -16'd2); wait_for_mul;
       mult(16'd2, -16'd2); wait_for_mul;

       mult(16'ha5a5, -16'ha5a5); wait_for_mul;
       mult(16'ha5a5, -16'd1); wait_for_mul;
       mult(16'ha5a5, 16'd1); wait_for_mul;
       mult(-16'ha5a5, -16'd1); wait_for_mul;

       mult(-16'd1, -16'd1); wait_for_mul;
       mult(16'd1, -16'd1); wait_for_mul;
       mult(16'h7fff, 16'h7fff); wait_for_mul;
       
       #100 $finish;
    end

  always
    begin
      #10 clk = 0;
      #10 clk = 1;
    end

   always @(posedge clk)
     #2 begin
	$display("reset %d, ready %d, done %d, state %b bit %d %x %x -> %x",
		 mul.reset, mul.ready, mul.done,
		 mul.state, mul.bitnum,
		 mul.multiplier,
		 mul.multiplicand,
		 mul.product);
	
     end

endmodule

