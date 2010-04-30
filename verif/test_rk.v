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

   wire        dma_rd, dma_wr, dma_req;
   reg dma_ack;
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
	      .rk_state(rk_state)
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

       write_rk_reg(13'o17400, 0); // rkda
       write_rk_reg(13'o17402, 0); // rker

       // read sector 0
       write_rk_reg(13'o17406, 16'hfe00); // rkwc;
       write_rk_reg(13'o17410, 0); // rkba;
       write_rk_reg(13'o17412, 0); // rkda;
       write_rk_reg(13'o17404, 5); // rkcs

       wait_for_rk_busy;
       wait_for_rk_idle;
       
       // read sector 2
       write_rk_reg(13'o17406, 16'hfe00); // rkwc;
       write_rk_reg(13'o17410, 0); // rkba;
       write_rk_reg(13'o17412, 2); // rkda;
       write_rk_reg(13'o17404, 5); // rkcs

       wait_for_rk_busy;
       wait_for_rk_idle;
       
       // read sector 0
       write_rk_reg(13'o17406, 16'hfe00); // rkwc;
       write_rk_reg(13'o17410, 0); // rkba;
       write_rk_reg(13'o17412, 0); // rkda;
       write_rk_reg(13'o17404, 5); // rkcs

       wait_for_rk_busy;
       wait_for_rk_idle;
       
       // write sector 4
       write_rk_reg(13'o17406, 16'hfe00); // rkwc;
       write_rk_reg(13'o17410, 0); // rkba;
       write_rk_reg(13'o17412, 4); // rkda;
       write_rk_reg(13'o17404, 3); // rkcs

       wait_for_rk_busy;
       wait_for_rk_idle;
       
       // read sector 4
       write_rk_reg(13'o17406, 16'hfe00); // rkwc;
       write_rk_reg(13'o17410, 0); // rkba;
       write_rk_reg(13'o17412, 4); // rkda;
       write_rk_reg(13'o17404, 5); // rkcs

       wait_for_rk_busy;
       wait_for_rk_idle;
       
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

   always @(posedge clk)
     #2 begin
	if (rk.ide1.ata_state == 0 ||
	    rk.ide1.ata_state == 5)
	  begin
	$display("rk_state %d (->%d); rkcs %o %o rkba %o rkwc %o",
		 rk.rk_state, rk.rk_state_next,
		 rk.rkcs_done, rk.rkcs_cmd, rk.rkba, rk.rkwc);

	$display("            dma_req %b, dma_ack %b",
		   rk.rk_state, rk.dma_req, rk.dma_ack);
	  end

`ifdef show_ata	
	$display("            ata_state %d ata_rd %d ata_done %o ide_data_bus %o ata_out %o",
		 rk.ide1.ata_state, rk.ide1.ata_rd, rk.ide1.ata_done,
		 rk.ide1.ide_data_bus, rk.ide1.ata_out);
`endif

     end
   
endmodule

