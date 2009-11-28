// test_tt.v
// simple testbench for tt_regs.v uart interface
// sends some characters out rs232 and recieves some characters out rs232
// tests basic rs232 and interrupts
// brad@heeltoe.com 2009

`timescale 1ns / 1ns

`include "../rtl/tt_regs.v"
`include "../rtl/brg.v"
`include "../rtl/uart.v"

module test_tt;

   reg clk, reset;
   wire brgclk;
   
   reg [12:0] iopage_addr;
   reg 	      iopage_wr;
   reg 	      iopage_rd;
   reg [15:0] data_in;
   
   reg 	      rx;
   wire       tx;
   wire       interrupt;
   reg 	      interrupt_ack;
   wire [7:0] vector;

   // write iopage register
   tt_regs tt(.clk(clk),
	      .brgclk(brgclk),
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
	      .rs232_tx(tx),
	      .rs232_rx(rx));

   assign     brgclk = clk;
   
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

   // read iopage register
   task read_tt_reg;
      input [12:0] addr;

      begin
	 #20 begin
	    iopage_addr = addr;
	    iopage_rd = 1;
	 end
	 #20 begin
	    //$display("read_tt_reg %o, data_out %o", addr, data_out);
	    iopage_rd = 0;
	 end
      end
   endtask

   // send rs232 data by wiggling rs232 input
   //
   // 50,000,000Mhz clock = 20ns cycle
   // 9600 baud is 1 bit every 1/9600 = 1.04e-4 = .000104 = 104us
   // 104ns = 104166ns
`define bitdelay 104166

   task send_tt_rx;
      input [7:0] data;
      begin
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
      begin
	 while (tt.tti_full == 0) #10;
	 //#2 $display("tti_full %b, rx_data %x", tt.tti_full, tt.rx_data);
	 #20;
      end
   endtask

   task failure;
      input [15:0] addr;
      input [15:0] got;
      input [15:0] wanted;
      
      begin
	 $display("FAILURE addr %o, read %x, desired %x",
		  addr, got, wanted);
      end
   endtask

   task wait_and_read_tt_data;
      input [7:0] expect;
      begin
	 wait_tti_ready;
	 read_tt_reg(13'o17562);
	 //$display("tt_reg %o, data_out %o, expect %o", 13'o17562, data_out, expect);
	 if (data_out != expect)
	   failure(13'o17562, data_out, expect);
	 else
	   $display("injected %o via rs-232, recieved via uart correctly",
		    expect);
      end
   endtask
   
   
   initial
     begin
	$timeformat(-9, 0, "ns", 7);

	$dumpfile("test_tt.vcd");
	$dumpvars(0, test_tt);
     end

   initial
     begin
	clk = 0;
	reset = 0;
	iopage_wr = 0;
	iopage_rd = 0;
	rx = 1;

	interrupt_ack = 0;
	
       #1 reset = 1;
       #50 reset = 0;

	// send some output
	write_tt_reg(13'o17566, 16'o15);
	wait_tto_ready;

	write_tt_reg(13'o17566, 16'o12); 
	wait_tto_ready;

	// enable tto interrupts
	write_tt_reg(13'o17564, 16'o100); 

	write_tt_reg(13'o17566, 16'o0); 
	wait_tto_ready;

	write_tt_reg(13'o17566, 16'o0); 
	wait_tto_ready;

	// disable tto interrupts
	write_tt_reg(13'o17564, 16'o000); 

	// recieve input
	#30 send_tt_rx(8'o15);
	wait_and_read_tt_data(8'o15);
	
	send_tt_rx(8'o12);
	wait_and_read_tt_data(8'o12);

	// enable interrupts
	write_tt_reg(13'o17560, 16'o100); 

	send_tt_rx(8'o20);
	wait_and_read_tt_data(8'o20);

	send_tt_rx(8'o21);
	wait_and_read_tt_data(8'o21);

	// disable interrupts
	write_tt_reg(13'o17560, 16'o000); 

	send_tt_rx(8'o22);
	wait_and_read_tt_data(8'o22);

	#100000;
	read_tt_reg(13'o17562);
	#100000;
	
	$finish;
    end

  always
    /* 50MHz clock */
    begin
      #10 clk = 0;
      #10 clk = 1;
    end

   always @(posedge clk)
     #2 begin
//	if (interrupt)
//	  $display("interrupt, vector %o", vector);
     end
   
endmodule

