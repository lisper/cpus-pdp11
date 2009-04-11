
`timescale 1ns / 1ns

`include "shift32.v"

module test;

   reg clk, reset;
   reg ready;
   wire done;

   reg [5:0] shift;
   reg [31:0] in;
   wire [31:0] out;

   shift32 shift32_box(.clk(clk), .reset(reset),
		       .ready(ready),
		       .done(done),
		       .in(in),
		       .out(out),
		       .shift(shift));

   task shifter;
      input [31:0] arg;
      input [5:0] amt;

      begin
	 #20 begin
	    in = arg;
	    shift = amt;
	    ready = 1;
	 end
	 #120 begin
	    ready = 0;
	 end
      end
   endtask

   task wait_for_shift;
      begin
	 while (done == 1'b0) #10;
	 #40;
      end
   endtask

  initial
    begin
      $timeformat(-9, 0, "ns", 7);

      $dumpfile("shift.vcd");
      $dumpvars(0, test.shift);
    end

  initial
    begin
       clk = 0;
       reset = 0;
       ready = 0;
       
       #1 reset = 1;
       #20 reset = 0;

//       shifter(32'h00000002, 6'd0); wait_for_shift;
       shifter(32'hffff8000, 6'o61); wait_for_shift;
`ifdef xx
       shifter(32'h00000001, 6'd4); wait_for_shift;
       shifter(32'h00000001, 6'd16); wait_for_shift;
       shifter(32'h10000000, 6'd4); wait_for_shift;
       shifter(32'h00000010, -6'd4); wait_for_shift;
       shifter(32'h00000010, -6'd5); wait_for_shift;
       shifter(32'h00000500, -6'd2); wait_for_shift;
       shifter(32'hffffffff, 6'd4); wait_for_shift;
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
	$display("reset %d, ready %d, done %d, state %b count %d %x -> %x %x",
		 shift32_box.reset, shift32_box.ready, shift32_box.done,
		 shift32_box.state, shift32_box.count,
		 shift32_box.in, shift32_box.out, shift32_box.last_bit);
     end

endmodule

