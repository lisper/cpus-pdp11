// run_bus.v
// simple testbench for bus interface; memory & rk dma
// read and writes memory
// does dma read from rk to memory
// brad@heeltoe.com 2009

`timescale 1ns / 1ns

`include "bus.v"

module test;

   reg clk, reset;

   reg [21:0] bus_addr;
   reg 	      bus_wr;
   reg 	      bus_rd;
   reg [15:0] bus_in;
   wire [15:0] bus_out;
   reg 	       bus_byte_op;

   wire        bus_ack;
   wire        bus_error;
   wire        interrupt;
   wire [7:0]  vector;

   wire [15:0] ide_data_bus;
   wire        ide_dior, ide_diow;
   wire [1:0]  ide_cs;
   wire [2:0]  ide_da;

   bus bus1(.clk(clk), .reset(reset),
	    .bus_addr(bus_addr),
	    .data_in(bus_in),
	    .data_out(bus_out),
	    .bus_rd(bus_rd),
	    .bus_wr(bus_wr),
	    .bus_byte_op(bus_byte_op),

	    .bus_ack(bus_ack),
	    .bus_error(bus_error),
	    .interrupt(interrupt),
	    .vector(vector),

   	    .ide_data_bus(ide_data_bus),
	    .ide_dior(ide_dior), .ide_diow(ide_diow),
	    .ide_cs(ide_cs), .ide_da(ide_da),

	    .psw(psw), .psw_io_wr(psw_io_wr)
	    );
   
   task write_io_reg;
      input [12:0] addr;
      input [15:0] data;

      begin
	 #20 begin
	    bus_addr = {6'b0, 6'b1111, addr};
	    bus_in = data;
	    bus_wr = 1;
	    bus_byte_op = 0;
	 end
	 #20 begin
	    bus_wr = 0;
	    bus_in = 0;
	 end
      end
   endtask

   task read_io_reg;
      input [12:0] addr;

      begin
	 #20 begin
	    bus_addr = {6'b0, 6'b1111, addr};
	    bus_rd = 1;
	    bus_byte_op = 0;
	 end
	 #20 begin
	    bus_rd = 0;
	 end
      end
   endtask

   task write_mem;
      input [16:0] addr;
      input [15:0] data;

      begin
	 #20 begin
	    bus_addr = {6'b0, addr};
	    bus_in = data;
	    bus_wr = 1;
	    bus_byte_op = 0;
	 end
	 #20 begin
	    bus_wr = 0;
	    bus_in = 0;
	 end
      end
   endtask

   task read_mem;
      input [16:0] addr;

      begin
	 #20 begin
	    bus_addr = {6'b0, addr};
	    bus_rd = 1;
	    bus_byte_op = 0;
	 end
	 #20 begin
	    bus_rd = 0;
	 end
      end
   endtask

   task wait_for_rk;
      begin
	 read_io_reg(13'o17404);
	 while (bus_out[7] == 1'b0) #10;
      end
   endtask
   
  initial
    begin
      $timeformat(-9, 0, "ns", 7);

      $dumpfile("bus.vcd");
       $dumpvars(0, test.bus1);
    end

  initial
    begin
       clk = 0;
       reset = 0;

       bus_wr = 0;
       bus_rd = 0;
       bus_byte_op = 0;

       #1 reset = 1;
       #20 reset = 0;

       write_mem(16'h0000, 16'ha5a5);
       write_mem(16'h0002, 16'h5a5a);
       write_mem(16'h0004, 16'h1234);

       read_mem(16'h0000);
       read_mem(16'h0002);
       read_mem(16'h0004);
       
       write_io_reg(13'o17400, 0); // rkda
       write_io_reg(13'o17402, 0); // rker
       write_io_reg(13'o17406, 16'o177400); // rkwc;
       
       write_io_reg(13'o17410, 0); // rkba;
       write_io_reg(13'o17412, 0); // rkda;
       write_io_reg(13'o17404, 5); // rkcs

       wait_for_rk;

       write_io_reg(13'o17400, 0); // rkda
       write_io_reg(13'o17402, 0); // rker
       write_io_reg(13'o17406, 16'o177400); // rkwc;
       write_io_reg(13'o17410, 16'o1000); // rkba;
       write_io_reg(13'o17412, 2); // rkda;
       write_io_reg(13'o17404, 5); // rkcs

       wait_for_rk;
       
       read_mem(16'h0000);
       read_mem(16'h0002);
       read_mem(16'h0004);
       
       #6000 $finish;
    end

   always @(posedge clk)
     begin
	$pli_ide(ide_data_bus, ide_dior, ide_diow, ide_cs, ide_da);
     end

  always
    begin
      #10 clk = 0;
      #10 clk = 1;
    end

   always @(posedge clk)
     #2 begin
//	$display("",
//		 rk.rkcs_done, rk.rkcs_cmd, rk.rkba, rk.rkwc);
     end
   
endmodule

