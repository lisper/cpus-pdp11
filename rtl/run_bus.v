// run_bus.v
// simple testbench for bus interface; memory & rk dma
// read and writes memory
// does dma write from rk to memory
// brad@heeltoe.com 2009

`timescale 1ns / 1ns

`define debug 1
`define debug_bus 1
`define debug_io 1
`define debug_bus_int 1
`define use_rk_model 1
//`define use_ram_sync 1
`define use_ram_async 1

`include "bus.v"
`include "ram_sync.v"
`include "ram_async.v"
`include "ram_s3board.v"

module test;

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
   wire        ram_rd, ram_wr, ram_byte_op;

   wire [15:0] ide_data_bus;
   wire        ide_dior, ide_diow;
   wire [1:0]  ide_cs;
   wire [2:0]  ide_da;

   reg [15:0]  psw;
   wire        psw_io_wr;

   reg [15:0]  switches;
   wire        rs232_tx;
   reg 	       rs232_rx;
   
   bus bus1(.clk(clk),
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
	    .ram_data_in(ram_data_in), .ram_data_out(ram_data_out),
	    .ram_rd(ram_rd), .ram_wr(ram_wr), .ram_byte_op(ram_byte_op),

   	    .ide_data_bus(ide_data_bus),
	    .ide_dior(ide_dior), .ide_diow(ide_diow),
	    .ide_cs(ide_cs), .ide_da(ide_da),

	    .psw(psw), .psw_io_wr(psw_io_wr),
	    .switches(switches), .rs232_tx(rs232_tx), .rs232_rx(rs232_rx)
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

   ram_async ram1(.addr(ram_addr[17:0]),
		  .data_in(ram_data_out),
		  .data_out(ram_data_in),
		  .rd(ram_rd),
		  .wr(ram_wr),
		  .byte_op(ram_byte_op),

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
      input [15:0] addr;
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
       bus_arbitrate = 0;
       
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
       bus_arbitrate = 1;
       write_io_reg(13'o17404, 5); // rkcs

       wait_for_rk;

       write_io_reg(13'o17400, 0); // rkda
       write_io_reg(13'o17402, 0); // rker
       write_io_reg(13'o17406, 16'o177400); // rkwc;
       write_io_reg(13'o17410, 16'o1000); // rkba;
       write_io_reg(13'o17412, 2); // rkda;
       write_io_reg(13'o17404, 5); // rkcs

       wait_for_rk;
       
       bus_arbitrate = 0;
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
	if (0) $display("grant_state %b",
			bus1.grant_state);
     end
   
endmodule

