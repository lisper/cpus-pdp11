
`timescale 1ns / 1ns

`include "div3216.v"

module test;

   reg clk, reset;
   reg ready;
   wire done;

   reg [31:0] dividend;
   reg [15:0] divider;
   wire [31:0] quotient;

   div3216 div(.clk(clk), .reset(reset),
	       .ready(ready), .done(done),
	       .dividend(dividend),
	       .divider(divider),
	       .quotient(quotient)); 

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

   task wait_for_mul;
      begin
	 while (done == 1'b0) #10;
	 #40;
      end
   endtask

  initial
    begin
      $timeformat(-9, 0, "ns", 7);

      $dumpfile("div.vcd");
      $dumpvars(0, test.div);
    end

  initial
    begin
       clk = 0;
       reset = 0;
       ready = 0;
       
       #1 reset = 1;
       #20 reset = 0;

       divide(32'd2, 16'd1); wait_for_mul;
       divide(32'd2, 16'd2); wait_for_mul;
       divide(32'd10, 16'd10); wait_for_mul;

       divide(-32'd2, 16'd2); wait_for_mul;
       divide(-32'd2, -16'd2); wait_for_mul;
       divide(32'd2, -16'd2); wait_for_mul;

       divide(32'd100, 16'd10); wait_for_mul;
       
       divide(32'd1000, 16'd33); wait_for_mul;
`ifdef xxx       
       divide(32'ha5a5, -16'ha5a5); wait_for_mul;
       divide(32'ha5a5, -16'd1); wait_for_mul;
       divide(32'ha5a5, 16'd1); wait_for_mul;
       divide(-32'ha5a5, -16'd1); wait_for_mul;

       divide(-32'd1, -16'd1); wait_for_mul;
       divide(32'd1, -16'd1); wait_for_mul;
       divide(32'h7fff, 16'h7fff); wait_for_mul;
`endif
       
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
		 div.reset, div.ready, div.done,
		 div.state, div.bit,
		 div.dividend,
		 div.divider,
		 div.quotient);

	$display(" quot-temp %x, d-end %x d-ider %x",
		 div.quotient_temp, div.dividend_copy, div.divider_copy);
     end

endmodule

