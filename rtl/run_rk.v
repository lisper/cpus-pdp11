
`timescale 1ns / 1ns

`include "rk_regs.v"

module test;

   reg clk, reset;

   reg [12:0] iopage_addr;
   reg 	      iopage_wr;
   reg [15:0] data_in;
   
   
   rk_regs rk(.clk(clk),
	      .reset(reset),
	      .iopage_addr(iopage_addr),
	      .data_in(data_in),
	      .data_out(data_out),
	      .decode(decode),
	      .iopage_rd(iopage_rd),
	      .iopage_wr(iopage_wr),
	      .iopage_byte_op(iopage_byte_op));

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

      $dumpfile("disk.vcd");
       $dumpvars(0, test.rk);
    end

  initial
    begin
       clk = 0;
       reset = 0;

       #1 reset = 1;
       #20 reset = 0;

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
     #2 begin
	$display("rk_state %d (->%d); ata_state %d ata_done %o; rkcs %o %o rkba %o rkwc %o",
		 rk.rk_state, rk.rk_state_next,
		 rk.ide1.ata_state, rk.ata_done,
		 rk.rkcs_done, rk.rkcs_cmd, rk.rkba, rk.rkwc);
     end
   
endmodule

