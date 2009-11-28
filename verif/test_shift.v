
`timescale 1ns / 1ns

`include "../rtl/shift32.v"

module test_shift;

   reg clk, reset;
   reg ready;
   wire done;

   reg [5:0] shift;
   reg [31:0] in;
   wire [31:0] out;

   wire last_bit;
   wire sign_change16;
   wire sign_change32;
   

   shift32 shift32_box(.clk(clk), .reset(reset),
		       .ready(ready),
		       .done(done),
		       .in(in),
		       .out(out),
		       .shift(shift),
		       .last_bit(last_bit),
		       .sign_change16(sign_change16),
		       .sign_change32(sign_change32)); 

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
	 #10;
      end
   endtask

   task failure;
      input [31:0] first;
      input [31:0] last;
      input [5:0]  amt;
      begin
	 $display("FAILURE first %x, in %x, out %x, desired %x, amt %d",
		  first, in, out, last, amt);
	 $display("done %b", done);
      end
   endtask

   task test_shifter;
      input [31:0] first;
      input [5:0]  amt;
      input [31:0] last;

      begin
	 $display("test_shifter %x %d", first, amt);
	 shifter(first, amt);
	 wait_for_shift;
	 if (out != last)
	   failure(first, last, amt);
	 else
	   $display("success");
      end
   endtask
   
  initial
    begin
      $timeformat(-9, 0, "ns", 7);

      $dumpfile("test_shift.vcd");
      $dumpvars(0, test_shift);
    end

  initial
    begin
       clk = 0;
       reset = 0;
       ready = 0;
       in = 0;
       shift = 0;

       #1 reset = 1;
       #50 reset = 0;

       test_shifter(32'h00000002, 6'd0, 32'h00000002);
       test_shifter(32'hffff8000, 6'o61, 32'hffffffff);
       test_shifter(32'h00000001, 6'd4, 32'h00000010);
       test_shifter(32'h00000001, 6'd16, 32'h00010000);
       test_shifter(32'h10000000, 6'd4, 32'h0);
       test_shifter(32'h00000010, -6'd4, 32'h00000001);
       test_shifter(32'h00000010, -6'd4, 32'h00000001);
       test_shifter(32'h00000010, -6'd5, 32'h00000000);
       test_shifter(32'h00000500, -6'd2, 32'h00000140);
       test_shifter(32'hffffffff, 6'd4, 32'hfffffff0);
       
       $finish;
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

