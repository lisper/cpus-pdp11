
`timescale 1ns / 1ns

`include "../rtl/mul1616.v"

module test_mul;

   reg clk, reset;
   reg ready;
   wire done;

   reg [15:0] multiplier;
   reg [15:0] multiplicand;
   wire [31:0] product;
   wire        overflow;
   
   mul1616 mul(.clk(clk),
	       .reset(reset),
	       .ready(ready),
	       .done(done),
	       .multiplier(multiplier),
	       .multiplicand(multiplicand),
	       .product(product),
	       .overflow(overflow)); 

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
	 #20;
      end
   endtask

   task failure;
      input [15:0] arg1;
      input [15:0] arg2;
      input [31:0] result;
      
      begin
	 $display("FAILURE arg1 %x, arg2 %x, product %x, desired %x",
		  arg1, arg2, product, result);
	 $display("done %b", done);
      end
   endtask

   task test_mult;
      input [15:0] arg1;
      input [15:0] arg2;
      input [31:0] result;

      begin
	 $display("test_mult %x %x", arg1, arg2);
	 mult(arg1, arg2);
	 wait_for_mul;
	 if (product != result)
	   failure(arg1, arg2, result);
	 else
	   $display("success");
      end
   endtask
   
  initial
    begin
      $timeformat(-9, 0, "ns", 7);

      $dumpfile("test_mul.vcd");
      $dumpvars(0, test_mul);
    end

  initial
    begin
       clk = 0;
       reset = 0;
       ready = 0;
       multiplier = 0;
       multiplicand = 0;
       
       #1 reset = 1;
       #50 reset = 0;

       test_mult(16'd2, 16'd1, 32'd2);
       test_mult(16'd2, 16'd2, 32'd4);
       test_mult(16'd10, 16'd10, 32'd100);

       test_mult(-16'd2, 16'd2, -32'd4);
       test_mult(-16'd2, -16'd2, 32'd4);
       test_mult(16'd2, -16'd2, -32'd4);

       test_mult(16'ha5a5, -16'ha5a5, 32'he01be3a7); /* ? */
       test_mult(16'ha5a5, -16'd1, 32'h00005a5b);
       test_mult(16'ha5a5, 16'd1, 32'hffffa5a5);
       test_mult(-16'ha5a5, -16'd1, 32'hffffa5a5);

       test_mult(16'h5a5a, -16'h5a5a, 32'he01c985c); /* ? */
       test_mult(16'h5a5a, -16'd1, 32'hffffa5a6);
       test_mult(16'h5a5a, 16'd1, 32'h00005a5a);
       test_mult(-16'h5a5a, -16'd1, 32'h00005a5a);

       test_mult(-16'd1, -16'd1, 32'd1);
       test_mult(16'd1, -16'd1, -32'd1);
       test_mult(-16'd1, 16'd1, -32'd1);

       test_mult(16'h7fff, 16'h7fff, 32'h3fff0001);
       
       $finish;
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

