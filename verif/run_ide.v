
`timescale 1ns / 1ns

//`include "ide.v"
`include "rk_regs.v"

module test;

   reg clk, reset;
   reg ata_rd, ata_wr;
   reg [4:0] ata_addr;
   reg [15:0] ata_in;
   wire [15:0] ata_out;
   wire ata_done;

   wire [15:0] ide_data_bus;
   wire        ide_dior, ide_diow;
   wire [1:0]  ide_cs;
   wire [2:0]  ide_da;

   ide ide1(.clk(clk), .reset(reset),
	    .ata_rd(ata_rd), .ata_wr(ata_wr), .ata_addr(ata_addr),
	    .ata_in(ata_in), .ata_out(ata_out), .ata_done(ata_done),
	    .ide_data_bus(ide_data_bus),
	    .ide_dior(ide_dior), .ide_diow(ide_diow),
	    .ide_cs(ide_cs), .ide_da(ide_da));

  initial
    begin
      $timeformat(-9, 0, "ns", 7);

      $dumpfile("ide.vcd");
      $dumpvars(0, test.ide1);
    end

  initial
    begin
       clk = 0;
       reset = 0;
       ata_rd = 0;
       ata_wr = 0;
       ata_in = 0;
       ata_addr = 0;

       #1 reset = 1;
       #20 reset = 0;

       #20 begin
	  ata_rd = 1;
	  ata_addr = 5'b00111;
	  ata_in = 16'h1234;
       end

       #100 ata_rd = 0;
       
       
       #200 $finish;
//       #1500000 $finish;
    end

  always
    begin
      #10 clk = 0;
      #10 clk = 1;
    end

endmodule

