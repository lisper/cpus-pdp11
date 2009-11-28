// run_div.v
// testbench for div3216

`timescale 1ns / 1ns

`include "../rtl/div3216.v"

module test_div;

   reg clk, reset;
   reg ready;
   wire done;

   reg [31:0] dividend;
   reg [15:0] divider;
   wire [15:0] quotient;
   wire [15:0] remainder;
   wire        overflow;
   
   div3216 div(.clk(clk), .reset(reset),
	       .ready(ready), .done(done),
	       .dividend(dividend),
	       .divider(divider),
	       .quotient(quotient),
   	       .remainder(remainder),
	       .overflow(overflow));

   task divide;
      input [31:0] arg1;
      input [15:0] arg2;

      begin
	 #20 begin
	    dividend = arg1;
	    divider = arg2;
	    ready = 1;
	 end
	 #120 begin
	    ready = 0;
	 end
      end
   endtask

   task wait_for_div;
      begin
	 while (done == 1'b0) #10;
	 #20;
      end
   endtask

   task failure;
      input [31:0] arg1;
      input [15:0] arg2;
      input [15:0] quo;
      input [15:0] rem;
      input 	   oflo;
      
      begin
	 $display("FAILURE arg1 %x, arg2 %x, quotient %x desired %x, remainder %x desired %x, oflo %b desired %b",
		  arg1, arg2, quotient, quo, remainder, rem, overflow, oflo);
	 $display("done %b", done);
      end
   endtask

   task test_divi;
      input [31:0] arg1;
      input [15:0] arg2;
      input [15:0] quo;
      input [15:0] rem;
      input 	   oflo;
      
      begin
	 $display("test_div %x %x", arg1, arg2);
	 divide(arg1, arg2);
	 wait_for_div;
	 if (quotient != quo || remainder != rem || overflow != oflo)
	   failure(arg1, arg2, quo, rem, oflo);
	 else
	   $display("success");
      end
   endtask
   
  initial
    begin
      $timeformat(-9, 0, "ns", 7);

      $dumpfile("test_div.vcd");
      $dumpvars(0, test_div);
    end

  initial
    begin
       clk = 0;
       reset = 0;
       ready = 0;
       
       #1 reset = 1;
       #50 reset = 0;

       test_divi({16'o77777,16'o177777}, 16'o2, 16'hffff, 16'h0001, 1'b1);
       test_divi({16'o25253,16'o1}, 16'o125252, 16'h8000, 16'h0001, 1'b0);
       test_divi({16'o177777,16'o177776}, 16'd3, 16'h0000, 16'hfffe, 1'b0);
       test_divi({16'o177777, 16'o177767}, 16'd2, 16'hfffc, 16'hffff, 1'b0);

       test_divi(32'd10, 16'd3, 16'd3, 16'd1, 1'b0);
       test_divi(32'd2, 16'd1, 16'd2, 16'd0, 1'b0);
       test_divi(32'd2, 16'd2, 16'd1, 16'd0, 1'b0);
       test_divi(32'd10, 16'd10, 16'd1, 16'd0, 1'b0);
       test_divi(32'd88, 16'd10, 16'd8, 16'd8, 1'b0);

       test_divi(-32'd2, 16'd2, -16'd1, 16'd0, 1'b0);
       test_divi(-32'd2, -16'd2, 16'd1, 16'd0, 1'b0);
       test_divi(32'd2, -16'd2, -16'd1, 16'd0, 1'b0);

       test_divi(32'd100, 16'd10, 16'd10, 16'd0, 1'b0);
       test_divi(32'd1000, 16'd33, 16'd30, 16'd10, 1'b0);

       test_divi(32'ha5a5, -16'ha5a5, 16'd1, 16'h4b4a, 1'b0);
       test_divi(32'ha5a5, -16'd1, 16'h5a5b, 16'd0, 1'b1);
       test_divi(32'ha5a5, 16'd1, 16'ha5a5, 16'd0, 1'b1);
       test_divi(-32'ha5a5, -16'd1, 16'ha5a5, 16'd0, 1'b1);

       test_divi(-32'd1, -16'd1, 16'd1, 16'd0, 1'b0);
       test_divi(32'd1, -16'd1, -16'd1, 16'd0, 1'b0);
       test_divi(32'h7fff, 16'h7fff, 16'd1, 16'd0, 1'b0);
       
       $finish;
    end

  always
    begin
      #10 clk = 0;
      #10 clk = 1;
    end

   always @(posedge clk)
     #2 begin
	$display("reset %d, ready %d, done %d, state %b bit %d %x %x -> %x %x",
		 div.reset, div.ready, div.done,
		 div.state, div.bitnum,
		 div.dividend,
		 div.divider,
		 div.quotient,
		 div.remainder);

	$display(" quot-temp %x, d-end %x d-ider %x, overflow %x",
		 div.quotient_temp, div.dividend_copy, div.divider_copy,
		 div.overflow);
     end

endmodule

