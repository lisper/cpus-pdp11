
`include "pdp11.v"

`timescale 1ns / 1ns

module test;

  reg clk, reset_n;
  reg[15:0] switches;

  pdp11 cpu(clk, reset_n, switches);

  initial
    begin
      $timeformat(-9, 0, "ns", 7);

      $dumpfile("pdp11.vcd");
      $dumpvars(0, test.cpu);
    end

  initial
    begin
       clk = 0;
       reset_n = 1;

       #1 begin
          reset_n = 0;
       end

       #100 begin
          reset_n = 1;
       end
  
       #200 $finish;
//       #1500000 $finish;
    end

  always
    begin
      #10 clk = 0;
      #10 clk = 1;
    end

  //----
  integer cycle;

  initial
    cycle = 0;

  always @(posedge cpu.clk)
    begin
      cycle = cycle + 1;
      #1 $display("cycle %d, pc %o, psw %o, istate %d",
		  cycle, cpu.pc, cpu.psw, cpu.istate);
    end

endmodule

