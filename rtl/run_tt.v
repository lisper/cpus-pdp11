// run_tt.v
// simple testbench for tt_regs.v uart interface
// sends some characters out rs232 and recieves some characters out rs232
// tests basic rs232 and interrupts
// brad@heeltoe.com 2009

`timescale 1ns / 1ns

`include "tt_regs.v"

module test;

   reg clk, reset;

   reg [12:0] iopage_addr;
   reg 	      iopage_wr;
   reg 	      iopage_rd;
   reg [15:0] data_in;
   

   reg 	      rx;
   wire       tx;
   wire       interrupt;
   wire [7:0] vector;
   
   tt_regs tt(.clk(clk),
	      .reset(reset),
	      .iopage_addr(iopage_addr),
	      .data_in(data_in),
	      .data_out(data_out),
	      .decode(decode),
	      .iopage_rd(iopage_rd),
	      .iopage_wr(iopage_wr),
	      .iopage_byte_op(iopage_byte_op),
	      .interrupt(interrupt),
	      .vector(vector),
	      .rs232_tx(tx),
	      .rs232_rx(rx));

   task write_tt_reg;
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

   task read_tt_reg;
      input [12:0] addr;

      begin
	 #20 begin
	    iopage_addr = addr;
	    iopage_rd = 1;
	 end
	 #20 begin
	    iopage_rd = 0;
	 end
      end
   endtask

   // send rs232 data
   task send_tt_rx;
      input [7:0] data;
      begin
`define bitdelay 42000
	 #`bitdelay rx = 0;
	 #`bitdelay rx = data[0];
	 #`bitdelay rx = data[1];
	 #`bitdelay rx = data[2];
	 #`bitdelay rx = data[3];
	 #`bitdelay rx = data[4];
	 #`bitdelay rx = data[5];
	 #`bitdelay rx = data[6];
	 #`bitdelay rx = data[7];
	 #`bitdelay rx = 1;
	 #`bitdelay rx = 1;
      end
   endtask 

   task wait_tto_ready;
      while (!tt.tto_empty) #10;
   endtask
   
   task wait_tti_ready;
      while (tt.tti_empty) #10;
   endtask
   
   initial
     begin
	$timeformat(-9, 0, "ns", 7);

	$dumpfile("tt.vcd");
	$dumpvars(0, test.tt);
     end

   initial
     begin
	clk = 0;
	reset = 0;
	iopage_wr = 0;
	iopage_rd = 0;
	rx = 1;
	
       #1 reset = 1;
       #20 reset = 0;

	write_tt_reg(13'o17566, 16'o15);
	wait_tto_ready;

	write_tt_reg(13'o17566, 16'o12); 
	wait_tto_ready;

	write_tt_reg(13'o17564, 16'o100); 
	wait_tto_ready;

	write_tt_reg(13'o17566, 16'o0); 
	wait_tto_ready;

	write_tt_reg(13'o17566, 16'o0); 
	wait_tto_ready;

	#30 send_tt_rx(8'o15);
	wait_tti_ready;
	read_tt_reg(13'o17562);

	send_tt_rx(8'o12);
	wait_tti_ready;
	read_tt_reg(13'o17562);
	
       #100000 $finish;
    end

  always
    begin
      #10 clk = 0;
      #10 clk = 1;
    end

   always @(posedge clk)
     #2 begin
     end
   
endmodule

