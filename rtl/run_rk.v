
`timescale 1ns / 1ns

`include "rk_regs.v"

module test;

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

   rk_regs rk(.clk(clk),
	      .reset(reset),
	      .iopage_addr(iopage_addr),
	      .data_in(data_in),
	      .data_out(data_out),
	      .decode(decode),
	      .iopage_rd(iopage_rd),
	      .iopage_wr(iopage_wr),
	      .iopage_byte_op(iopage_byte_op),

	      // connection to ide drive
	      .ide_data_bus(ide_data_bus),
	      .ide_dior(ide_dior), .ide_diow(ide_diow),
	      .ide_cs(ide_cs), .ide_da(ide_da),

	      // dma upward to memory
	      .dma_req(dma_req), .dma_ack(dma_ack),
	      .dma_addr(dma_addr),
	      .dma_data_in(dma_data_in),
	      .dma_data_out(dma_data_out),
	      .dma_rd(dma_rd), .dma_wr(dma_wr)
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

  initial
    begin
      $timeformat(-9, 0, "ns", 7);

      $dumpfile("rk.vcd");
       $dumpvars(0, test.rk);
    end

  initial
    begin
       clk = 0;
       reset = 0;

       #1 reset = 1;
       #20 reset = 0;

       dma_ack = 1;
       
       write_rk_reg(13'o17400, 0); // rkda
       write_rk_reg(13'o17402, 0); // rker
       write_rk_reg(13'o17406, 16'hfffc); // rkwc;
       write_rk_reg(13'o17410, 0); // rkba;
       write_rk_reg(13'o17412, 0); // rkda;
       write_rk_reg(13'o17404, 5); // rkcs
       
       #5000 $finish;
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
	
	$display("            ata_state %d ata_rd %d ata_done %o ide_data_bus %o ata_out %o",
		 rk.ide1.ata_state, rk.ide1.ata_rd, rk.ide1.ata_done,
		 rk.ide1.ide_data_bus, rk.ide1.ata_out);
//		 rk.ide1.ata_in, rk.ide1.ata_out);


     end
   
endmodule

