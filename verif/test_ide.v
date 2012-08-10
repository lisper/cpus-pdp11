//

`timescale 1ns / 1ns

`include "../rtl/ide.v"

`define use_ide_pli 1

module test_ide;

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

   ide ide1(.clk(clk),
	    .reset(reset),
	    .ata_rd(ata_rd), .ata_wr(ata_wr), .ata_addr(ata_addr),
	    .ata_in(ata_in), .ata_out(ata_out), .ata_done(ata_done),
	    .ide_data_bus(ide_data_bus),
	    .ide_dior(ide_dior), .ide_diow(ide_diow),
	    .ide_cs(ide_cs), .ide_da(ide_da));

   task wait_for_ide_done;
      begin
	 while (ata_done == 3'b0) #10;
	 #20;
      end
   endtask

   task wait_for_ide_idle;
      begin
	 while (ide1.ata_state != 3'b0) #10;
	 #20;
      end
   endtask

   task failure;
      input [4:0] addr;
      input [15:0] expect;
      
      begin
	 $display("FAILURE addr %x, ata_out %x, expected %x",
		  addr, ata_out, expect);
      end
   endtask


   task test_ide_read;
      input [4:0] addr;
      input [15:0] data;
      input [15:0] expect;

      begin
	 #20 begin
 	    ata_rd = 1;
	    ata_addr = addr;
	    ata_in = data;
	 end
	 wait_for_ide_done;
	 $display("addr %x in %x out %x", addr, ata_in, ata_out);
	 if (ata_out != expect)
	   failure(addr, expect);
	 ata_rd = 0;
	 wait_for_ide_idle;
      end
   endtask
   
   task test_ide_write;
      input [4:0] addr;
      input [15:0] data;

      begin
	 #20 begin
 	    ata_wr = 1;
	    ata_addr = addr;
	    ata_in = data;
	 end
	 wait_for_ide_done;
	 $display("addr %x in %x out %x", addr, ata_in, ata_out);
	 ata_wr = 0;
	 wait_for_ide_idle;
      end
   endtask
   
  initial
    begin
      $timeformat(-9, 0, "ns", 7);

      $dumpfile("test_ide.vcd");
      $dumpvars(0, test_ide);
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
       #50 reset = 0;

       test_ide_read(5'b10111, 16'h0, 16'h0050);
       test_ide_read(5'b10001, 16'h0, 16'h0);
       test_ide_read(5'b10010, 16'h0, 16'h0);
       test_ide_read(5'b10011, 16'h0, 16'h0);
       test_ide_read(5'b10100, 16'h0, 16'h0);
       test_ide_read(5'b10101, 16'h0, 16'h0);
       test_ide_read(5'b10110, 16'h0, 16'h0);

       test_ide_write(5'b10000, 16'h0);
       test_ide_write(5'b10000, 16'h1);
       test_ide_write(5'b10000, 16'h2);

       $finish;
    end

  always
    begin
      #10 clk = 0;
      #10 clk = 1;
    end

`ifdef use_ide_pli
   always @(posedge clk)
     begin
	$pli_ide(ide_data_bus, ide_dior, ide_diow, ide_cs, ide_da);
     end
`endif
   
endmodule

