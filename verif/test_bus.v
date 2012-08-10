// run_bus.v
// simple testbench for bus interface; memory & rk dma
// read and writes memory
// does dma write from rk to memory
// brad@heeltoe.com 2009

`timescale 1ns / 1ns

`define debug		1
`define debug_bus	1
//`define debug_bus_ram	1
//`define debug_bus_state	1
//`define debug_bus_io	1
//`define debug_io	1
//`define debug_rk_regs	1
//`define debug_ram	1

`define debug_bus_int	1

`define use_rk_model	1

//`define debug_ram_low	1

//`define use_ram_sync	1
// requires either 'use_ram_model' or 'use_ram_pli' to be defined

`define use_ram_async	1

`include "rtl.v"

module test_bus;

   reg clk, reset;

   reg [21:0] bus_addr;
   reg 	      bus_wr;
   reg 	      bus_rd;
   reg [15:0] bus_in;
   wire [15:0] bus_out;
   reg 	       bus_byte_op;

   reg 	       bus_arbitrate;
   wire        bus_ack;
   wire        bus_error;

   wire        bus_int;
   wire [7:0]  bus_int_ipl;
   wire [7:0]  bus_int_vector;
   wire [7:0]  interrupt_ack_ipl;

   wire [21:0] ram_addr;
   wire [15:0] ram_data_in, ram_data_out;
   wire        ram_rd, ram_wr, ram_byte_op, ram_done;

   wire        pxr_wr;
   wire        pxr_rd;
   wire [7:0]  pxr_addr;
   wire [1:0]  pxr_be;
   wire [15:0] pxr_data_in;
   wire [15:0] pxr_data_out;
   
   wire [15:0] ide_data_bus;
   wire        ide_dior, ide_diow;
   wire [1:0]  ide_cs;
   wire [2:0]  ide_da;

   reg [15:0]  psw;
   wire        psw_io_wr;

   reg [15:0]  switches;
   wire        rs232_tx;
   reg 	       rs232_rx;

   wire [4:0]  rk_state;

   reg 	       wr_inhibit;
   
   bus bus(.clk(clk),
	    .brgclk(brgclk),
	    .reset(reset),

	    .bus_addr(bus_addr),
	    .bus_data_in(bus_in),
	    .bus_data_out(bus_out),
	    .bus_rd(bus_rd),
	    .bus_wr(bus_wr),
	    .bus_byte_op(bus_byte_op),
	    .bus_arbitrate(bus_arbitrate),
	    .bus_ack(bus_ack),
	    .bus_error(bus_error),

	    .bus_int(bus_int),
	    .bus_int_ipl(bus_int_ipl),
	    .bus_int_vector(bus_int_vector),
	    .interrupt_ack_ipl(interrupt_ack_ipl),

	    .ram_addr(ram_addr),
	    .ram_data_in(ram_data_in),
	    .ram_data_out(ram_data_out),
	    .ram_rd(ram_rd),
	    .ram_wr(ram_wr),
	    .ram_byte_op(ram_byte_op),
	    .ram_done(ram_done),

	    .pxr_wr(pxr_wr),
	    .pxr_rd(pxr_rd),
	    .pxr_be(pxr_be),
	    .pxr_addr(pxr_addr),
	    .pxr_data_in(pxr_data_in),
	    .pxr_data_out(pxr_data_out),
	    .pxr_trap(pxr_trap),

   	    .ide_data_bus(ide_data_bus),
	    .ide_dior(ide_dior),
	    .ide_diow(ide_diow),
	    .ide_cs(ide_cs),
	    .ide_da(ide_da),

	    .psw(psw),
	    .psw_io_wr(psw_io_wr),
	    .switches(switches),
	    .rk_state(rk_state),
	    .rs232_tx(rs232_tx),
	    .rs232_rx(rs232_rx)
	    );

`ifdef use_ram_sync
   ram_sync ram1(.clk(clk),
		 .reset(reset),
		 .addr(ram_addr[15:0]),
		 .data_in(ram_data_out),
		 .data_out(ram_data_in),
		 .rd(ram_rd),
		 .wr(ram_wr),
		 .byte_op(ram_byte_op));
`endif

`ifdef use_ram_async
   wire [17:0] ram_a;
   wire        ram_oe_n, ram_we_n;
   wire [15:0] ram1_io;
   wire        ram1_ce_n, ram1_ub_n, ram1_lb_n;
   wire [15:0] ram2_io;
   wire        ram2_ce_n, ram2_ub_n, ram2_lb_n;

   ram_async ram1(.clk(clk),
		  .reset(reset),
		  .addr(ram_addr[17:0]),
		  .data_in(ram_data_out),
		  .data_out(ram_data_in),
		  .rd(ram_rd),
		  .wr(ram_wr),
		  .wr_inhibit(wr_inhibit),
		  .byte_op(ram_byte_op),
		  .done(ram_done),
		  
		  .ram_a(ram_a),
		  .ram_oe_n(ram_oe_n), .ram_we_n(ram_we_n),

		  .ram1_io(ram1_io), .ram1_ce_n(ram1_ce_n),
		  .ram1_ub_n(ram1_ub_n), .ram1_lb_n(ram1_lb_n),
		   
		  .ram2_io(ram2_io), .ram2_ce_n(ram2_ce_n), 
		  .ram2_ub_n(ram2_ub_n), .ram2_lb_n(ram2_lb_n));

   ram_s3board ram2(.ram_a(ram_a),
		    .ram_oe_n(ram_oe_n),
		    .ram_we_n(ram_we_n),
		    .ram1_io(ram1_io),
		    .ram1_ce_n(ram1_ce_n),
		    .ram1_ub_n(ram1_ub_n), .ram1_lb_n(ram1_lb_n),
		    .ram2_io(ram2_io),
		    .ram2_ce_n(ram2_ce_n),
		    .ram2_ub_n(ram2_ub_n), .ram2_lb_n(ram2_lb_n));
`endif

   task bus_transaction;
      input [21:0] addr;
      input [15:0] data_out;
      output [15:0] data_in;
      input 	   wr;
      input 	   rd;
      input 	   op;
      
      begin
	 @(posedge clk);
	 @(posedge clk);
	 bus_addr = addr;
	 bus_in = data_out;
	 #1 bus_wr = wr;
	 #1 bus_rd = rd;
	 bus_byte_op = op;

	 @(posedge clk);
	 @(posedge clk);
	 while (bus_ack == 1'b0) @(posedge clk);
@(posedge clk);
	 data_in = bus_out;
	 bus_wr = 0;
	 bus_rd = 0;
	 bus_byte_op = 0;
      end
   endtask
   
   task write_io_reg;
      input [12:0] addr;
      input [15:0] data;
      reg [15:0] d;

      begin
	 d = 0;
	 bus_transaction({12'b111111111111, addr}, data, d, 1, 0, 0);
      end
   endtask

   task read_io_reg;
      input [12:0] addr;
      output [15:0]   data;

      begin
	 bus_transaction({12'b111111111111, addr}, 0, data, 0, 1, 0);
      end
   endtask

   task write_mem22;
      input [21:0] addr;
      input [15:0] data;
      reg [15:0] d;

      begin
	 d = 0;
	 bus_transaction(addr, data, d, 1, 0, 0);
      end
   endtask

   task write_mem;
      input [15:0] addr;
      input [15:0] data;
      reg [15:0]   d;
      
      begin
	 d = 0;
	 bus_transaction(addr, data, d, 1, 0, 0);
      end
   endtask

   task read_mem;
      input [15:0] addr;
      output [15:0] result;
      
      begin
	 bus_transaction(addr, 0, result, 0, 1, 0);
      end
   endtask

   task fill_memory;
      input [15:0] start;
      input 	   size;
      input [15:0] value;

      integer size;

      reg [15:0] l;
      
      begin
	 for (l = start; l < start+size-2; l = l + 2)
	   begin
	      write_mem(l, value);
	   end
      end
   endtask

   task failure;
      input [15:0] addr;
      input [15:0] data;
      input [15:0] expected;
      
      begin
	 $display("FAILURE addr %o, data %o, expected %o",
		  addr, data, expected);
	 #60 $finish;
      end
   endtask

   task read_mem_expect;
      input [15:0] addr;
      input [15:0] data;

      reg [15:0]   result;
      
      begin
	 read_mem(addr, result);
	 if (result != data)
	   failure(addr, result, data);
      end
   endtask
      
   task prep_rk;
      reg [7:0] v;
      integer   b, i, file;

      begin
	 file = $fopen("rk-test.dsk", "wb");

	 // block 0
	 for (i = 0; i < 256; i = i + 1)
	   begin
      	      $fwriteb(file, "%u", i);
	      $fwriteb(file, "%u", i);
	   end

	 // %$%#%$# - can't write zeros!
	 $fwriteb(file, "%u", i);
	 $fwriteb(file, "%u", i);

	 // block 1..15
	 for (b = 1; b < 16; b = b + 1)
	   begin
	      v = b;
	      for (i = 0; i < 256; i = i + 1)
		begin
		   $fwrite(file, "%u", v);
		   $fwrite(file, "%u", v);
		end
	   end

	 $fclose(file);
      end
   endtask
   
   task wait_for_rk;
      reg [15:0]   result;
      begin
	 read_io_reg(13'o17404, result);
	 while (result[7] == 1'b0)
	   begin
	      read_io_reg(13'o17404, result);
	   end
      end
   endtask

   task test_bus_ram_test;
      begin
	 $display("\n***** bus ram test");
	 bus_arbitrate = 0;

	 // should bus error
         write_mem22(22'o0776000, 16'h0000);

	 $display("\n--write");
         write_mem(16'o0000, 16'ha5a5);
	 write_mem(16'o0002, 16'h5a5a);
	 write_mem(16'o0004, 16'o1234);
	 write_mem(16'o0006, 16'o177777);
	 write_mem(16'o0010, 16'o54321);

	 $display("\n--read");
	 read_mem_expect(16'o0000, 16'ha5a5);
	 read_mem_expect(16'o0002, 16'h5a5a);
	 read_mem_expect(16'o0004, 16'o1234);
	 read_mem_expect(16'o0006, 16'o177777);
	 read_mem_expect(16'o0010, 16'o54321);
      end
   endtask
   
   task test_bus_ram_retest;
      begin
	 $display("\n***** bus ram re-test");
	 bus_arbitrate = 0;
	 read_mem_expect(16'o0000, 16'ha5a5);
	 read_mem_expect(16'o0002, 16'h5a5a);
	 read_mem_expect(16'o0004, 16'o1234);
	 read_mem_expect(16'o0006, 16'o177777);
	 read_mem_expect(16'o0010, 16'o54321);
      end
   endtask
   
   // ------------------------------------------------------------------

   task bus_rk_test;
      begin
       write_io_reg(13'o17406, 16'o177400); // rkwc;
       write_io_reg(13'o17410, 16'o1000); // rkba;
       write_io_reg(13'o17412, 0); // rkda;
       bus_arbitrate = 1;
       write_io_reg(13'o17404, 5); // rkcs
       wait_for_rk;
      end
   endtask // bus_rk_test
   
   task rk_read_block;
      input [15:0] blk;
      input [15:0] wc;
      input [15:0] ma;

      begin
	 write_io_reg(13'o17402, 0); // rker
	 
	 write_io_reg(13'o17406, -wc); // rkwc;
	 write_io_reg(13'o17410, ma); // rkba;
	 write_io_reg(13'o17412, blk); // rkda;
	 write_io_reg(13'o17404, 5); // rkcs
	 wait_for_rk;
      end
   endtask

   task rk_write_block;
      input [15:0] blk;
      input [15:0] wc;
      input [15:0] ma;

      begin
	 write_io_reg(13'o17402, 0); // rker

	 write_io_reg(13'o17406, -wc); // rkwc;
	 write_io_reg(13'o17410, ma); // rkba;
	 write_io_reg(13'o17412, blk); // rkda;
	 write_io_reg(13'o17404, 3); // rkcs
	 wait_for_rk;
      end
   endtask

   task rk_check_blk0_pattern;
      input [15:0] ma;
      reg [15:0] l;
      reg [15:0] lp1;
      reg [15:0] tv;

      begin
	 // which should contain 0, 1, 2...
	 for (l = 0; l < 256; l = l + 1)
	   begin
	      lp1 = l + 1;
	      tv = { lp1[7:0], lp1[7:0] };

	      // hack due to inability to write zeros
	      if (l == 255) tv = 16'h0101;
	      
	      read_mem_expect(ma + l*2,  tv);
	   end
      end
   endtask

   task basic_rk_test;
      reg [15:0] l;
      reg [15:0] tv;
      reg [15:0] ma;

      begin
	 $display("\n***** basic rk test");

	 ma = 16'o1000;
	 
	 $display("\n--1/read sector 0");
	 fill_memory(ma, 512, 16'hffff);
	 rk_read_block(0, 512, ma);

	 rk_check_blk0_pattern(ma);

	 $display("\n--2/write sector 2 with zeros");
	 fill_memory(ma, 512, 16'o0);
	 rk_write_block(2, 512, ma);

	 $display("\n--3/read sector 0");
	 rk_read_block(0, 512, ma);

	 rk_check_blk0_pattern(ma);

	 $display("\n--4/read sector 2");
	 rk_read_block(2, 512, ma);
	 
	 for (l = 0; l < 256; l = l + 2)
	   read_mem_expect(ma + l,  0);

	 $display("\n--5/write sector 0 with zeros");
	 fill_memory(ma, 512, 16'o0);
	 rk_write_block(0, 512, ma);

	 $display("\n--6/read sector 0");
	 rk_read_block(0, 512, ma);

	 for (l = 0; l < 256; l = l + 2)
	   read_mem_expect(ma + l,  0);
	 
	 $display("\n--7/write sector 4");
	 fill_memory(ma, 512, 16'o1234);
	 rk_write_block(4, 512, ma);
       
	 $display("\n--8/read sector 0");
	 rk_read_block(0, 512, ma);
       
	 $display("\n--9/read sector 4");
	 rk_read_block(4, 512, ma);

	 for (l = 0; l < 256; l = l + 2)
	   read_mem_expect(ma + l,  16'o1234);

	 $display("\n--basic rk test done");
      end
   endtask // basic_rk_test

   task odd_rk_test;
      reg [15:0] ma;
      begin
	 $display("\n***** odd rk test");

	 ma = 16'o2000;

	 // read sector 0
	 rk_read_block(0, 256, ma);

	 // read sector 1
	 rk_read_block(1, 256, ma);

	 // write sector 0
	 rk_read_block(0, 256, ma);

	 // write sector 1
	 rk_read_block(1, 256, ma);
      end
   endtask // odd_rk_test
   
   task rk_test;
      integer 	   blk;
      reg [15:0] ma;

      begin
	 $display("\n***** block rk test");
	 
	 ma = 16'o4000;

	 for (blk = 0; blk < 10; blk = blk + 1)
	   begin
	      rk_read_block(blk, 256, ma);
	   end

	 for (blk = 0; blk < 10; blk = blk + 1)
	   begin
	      rk_write_block(blk, 256, ma);
	   end

	 for (blk = 0; blk < 10; blk = blk + 1)
	   begin
	      rk_read_block(blk, 256, ma);
	      rk_write_block(blk, 256, ma);
	   end
      end
   endtask // odd_rk_test

   task test_bus_rk_test;
      begin
	 bus_rk_test;
	 basic_rk_test;
	 odd_rk_test;
//       rk_test;
      end
   endtask
   
   // ------------------------------------------------------------------
   
  initial
    begin
       $timeformat(-9, 0, "ns", 7);

       $dumpfile("test_bus.vcd");
       $dumpvars(0, test_bus);

       prep_rk;
    end

  initial
    begin
       clk = 0;
       reset = 0;

       bus_wr = 0;
       bus_rd = 0;
       bus_byte_op = 0;
       bus_arbitrate = 0;
       wr_inhibit = 0;

       switches = 0;
       rs232_rx = 0;
       psw = 0;

       #1 reset = 1;
       #50 reset = 0;

       test_bus_ram_test;
       test_bus_rk_test;
       test_bus_ram_retest;
       
       $finish;
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
	if (0) $display("grant_state %b",
			bus.grant_state);
     end
   
endmodule

