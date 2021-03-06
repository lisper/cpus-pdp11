// test_rk.v
// simple testbench for rk_regs.v RK05 interface
// reads a block
// brad@heeltoe.com 2009
//

`timescale 1ns / 1ns

`include "../rtl/rk_regs.v"
`include "../rtl/ide.v"

module test_rk;

   reg clk, reset;

   reg [12:0] iopage_addr;
   reg 	      iopage_wr;
   reg [15:0] data_in;
   
   wire [15:0] ide_data_bus;
   wire        ide_dior, ide_diow;
   wire [1:0]  ide_cs;
   wire [2:0]  ide_da;

   wire        dma_rd, dma_wr;
   wire        dma_req;
   reg 	       dma_ack;
   wire [17:0] 	dma_addr;

   wire 	interrupt;
   wire 	interrupt_ack;
   wire [7:0] 	vector;
   wire [4:0] 	rk_state;

   rk_regs rk(.clk(clk),
	      .reset(reset),
	      .iopage_addr(iopage_addr),
	      .data_in(data_in),
	      .data_out(data_out),
	      .decode(decode),
	      .iopage_rd(iopage_rd),
	      .iopage_wr(iopage_wr),
	      .iopage_byte_op(iopage_byte_op),

	      .interrupt(interrupt),
	      .interrupt_ack(interrupt_ack),
	      .vector(vector),
	      
	      // connection to ide drive
	      .ide_data_bus(ide_data_bus),
	      .ide_dior(ide_dior),
	      .ide_diow(ide_diow),
	      .ide_cs(ide_cs),
	      .ide_da(ide_da),

	      // dma upward to memory
	      .dma_req(dma_req),
	      .dma_ack(dma_ack),
	      .dma_addr(dma_addr),
	      .dma_data_in(dma_data_in),
	      .dma_data_out(dma_data_out),
	      .dma_rd(dma_rd),
	      .dma_wr(dma_wr),
	      .rk_state_out(rk_state)
	      );

   //
   task write_rk_reg;
      input [12:0] addr;
      input [15:0] data;

      begin
	 #20 begin
	    iopage_addr = addr;
	    data_in = data;
	    iopage_wr = 1;
	 end
	 #20 begin
	    iopage_wr = 0;
	    data_in = 0;
	 end
      end
   endtask

   task wait_for_rk_busy;
      begin
	 $display("waiting for rk busy");
	 while (rk.rk_state == 0) #10;
	 #20;
      end
   endtask
   
   task wait_for_rk_idle;
      begin
	 $display("waiting for rk idle");
	 while (rk.rk_state != 0) #10;
	 #20;
	 $display("rk controller idle");
      end
   endtask

   task rk_read_block;
      input [15:0] blk;
      input [15:0] wc;
      input [15:0] ma;

      begin
	 write_rk_reg(13'o17402, 0); // rker
	 
	 write_rk_reg(13'o17406, -wc); // rkwc;
	 write_rk_reg(13'o17410, ma); // rkba;
	 write_rk_reg(13'o17412, blk); // rkda;
	 write_rk_reg(13'o17404, 5); // rkcs

	 wait_for_rk_busy;
	 wait_for_rk_idle;
      end
   endtask

   task rk_write_block;
      input [15:0] blk;
      input [15:0] wc;
      input [15:0] ma;

      begin
	 write_rk_reg(13'o17402, 0); // rker

	 write_rk_reg(13'o17406, -wc); // rkwc;
	 write_rk_reg(13'o17410, ma); // rkba;
	 write_rk_reg(13'o17412, blk); // rkda;
	 write_rk_reg(13'o17404, 3); // rkcs

	 wait_for_rk_busy;
	 wait_for_rk_idle;
      end
   endtask

   task failure;
      input [15:0] addr;
      input [15:0] data;
      input [15:0] expected;
      
      begin
	 $display("FAILURE addr %o, data %o, expected %o",
		  addr, data, expected);
      end
   endtask

   task basic_rk_test;
      begin
	 $display("***** basic rk05 tests");

	 // write sector zero
	 rk_write_block(0, 256, 0);

	 // read sector 2
//	 rk_read_block(2, 256, 0);
	 
	 // write sector 4
//	 rk_write_block(4, 256, 0);
      end
   endtask // basic_rk_test

   task odd_rk_test;
      begin
	 $display("***** odd rk05 tests");

	 // read sector 0 short
	 rk_read_block(0, 128, 0);

	 // read sector 0
	 rk_read_block(0, 256, 0);

	 // write sector 1 short
	 rk_read_block(1, 128, 0);

	 // write sector 0
	 rk_read_block(1, 256, 0);
      end
   endtask // odd_rk_test
   
   task rk_test;
      integer 	   blk;

      begin
	 $display("***** block rk05 tests");

	 for (blk = 0; blk < 10; blk = blk + 1)
	   begin
	      rk_read_block(blk, 256, 0);
	   end

	 for (blk = 0; blk < 10; blk = blk + 1)
	   begin
	      rk_write_block(blk, 256, 0);
	   end

	 for (blk = 0; blk < 10; blk = blk + 1)
	   begin
	      rk_read_block(blk, 256, 0);
	      rk_write_block(blk, 256, 0);
	   end
      end
   endtask // odd_rk_test

   task prep_rk;
      reg [15:0] v;
      integer   b, i, file;

      begin
	 $display("***** preparing rk-test.dsk");
	 
	 file = $fopen("rk-test.dsk", "wb");

	 // block 0
	 for (i = 0; i < 128; i = i + 1)
	   begin
	      v = i;
	      $fwrite(file, "%u", 8'h11);
	      $fwrite(file, "%u", v+1);
	   end

	 for (i = 0; i < 128; i = i + 1)
	   begin
	      v = i;
	      $fwrite(file, "%u", 8'h22);
	      $fwrite(file, "%u", v+1);
	   end

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
   
  initial
    begin
      $timeformat(-9, 0, "ns", 7);

      $dumpfile("test_rk.vcd");
       $dumpvars(0, test_rk);
    end

  initial
    begin
       clk = 0;
       reset = 0;
       data_in = 0;
		  
       #1 reset = 1;
       #50 reset = 0;

       dma_ack = 1;

       prep_rk;
       basic_rk_test;
       odd_rk_test;
//       rk_test;

       $display("done");

       $finish;
    end

  always
    begin
      #10 clk = 0;
      #10 clk = 1;
    end

   always @(posedge clk)
     begin
	$pli_ide(ide_data_bus, ide_dior, ide_diow, ide_cs, ide_da);
     end

`ifdef show_rk
   always @(posedge clk)
     #2 begin
	if (rk.ide1.ata_state == 0 ||
	    rk.ide1.ata_state == 5)
	  begin
	$display("rk_state %d (->%d); rkcs %o %o rkba %o rkwc %o",
		 rk.rk_state, rk.rk_state_next,
		 rk.rkcs_done, rk.rkcs_cmd, rk.rkba, rk.rkwc);

	$display("            dma_req %b, dma_ack %b",
		 rk.dma_req, rk.dma_ack);
	  end

`ifdef show_ata	
	$display("            ata_state %d ata_rd %d ata_done %o ide_data_bus %o ata_out %o",
		 rk.ide1.ata_state, rk.ide1.ata_rd, rk.ide1.ata_done,
		 rk.ide1.ide_data_bus, rk.ide1.ata_out);
`endif

     end
`endif
   
endmodule

