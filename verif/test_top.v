// test_top.v
// pdp-11 in verilog - verilator sim top level
// copyright Brad Parker <brad@heeltoe.com> 2009-2010
//

`define minimal_debug	1
`define debug		1
`define sim_time	1

`define fake_uart
`define slower

`define debug_tt_out	1
`define debug_tt_int	1
`define debug_io 	1
//`define debug_bus_io 	1
//`define debug_bus_all 	1
`define debug_bus_ram 	1

`define debug_cpu_int	1

// for RT-11
`define no_mmu
`define use_18bit_phys
`define sim_56k

`include "../rtl/pdp11.v"
`include "../rtl/ipl_below.v"
`include "../rtl/add8.v"

`ifdef no_mmu
 `include "../rtl/null_mmu.v"
`else
 `include "../rtl/mmu.v"
`endif

`include "../rtl/execute.v"
`include "../rtl/mul1616.v"
`include "../rtl/div3216.v"
`include "../rtl/shift32.v"

`include "../rtl/mmu_regs.v"
`include "../rtl/clk_regs.v"
`include "../rtl/sr_regs.v"
`include "../rtl/psw_regs.v"

`include "../rtl/rk_regs.v"
`include "../rtl/ide.v"

`include "../rtl/tt_regs.v"
`include "../rtl/brg.v"

`ifdef fake_uart
 `include "../rtl/fake_uart.v"
`else
 `include "../rtl/uart.v"
`endif

`include "../rtl/bus.v"
`include "../rtl/bootrom.v"
`include "../rtl/iopage.v"
`include "../rtl/reset_btn.v"
`include "../rtl/ram_async.v"


`include "../rtl/sevensegdecode.v"
`include "../rtl/display.v"
`include "../rtl/top.v"

module wrap_ide(clk, ide_data_in, ide_data_out, ide_dior, ide_diow, ide_cs, ide_da);

   input clk;
   input [15:0] ide_data_in;
   output [15:0] ide_data_out;
   input 	 ide_dior;
   input 	 ide_diow;
   input [1:0] 	 ide_cs;
   input [2:0] 	 ide_da;
		
   import "DPI-C" function void dpi_ide(input integer data_in,
					output integer data_out,
				        input integer dior,
				        input integer diow,
				        input integer cs,
				        input integer da);

   integer dbi, dbo;
   wire [31:0] dboo;
      
   assign dbi = {16'b0, ide_data_in};
   assign dboo = dbo;

   assign ide_data_out = dboo[15:0];

   always @(posedge clk)
     begin
	dpi_ide(dbi,
		dbo,
		{31'b0, ide_dior},
		{31'b0, ide_diow},
		{30'b0, ide_cs},
		{29'b0, ide_da});
     end

endmodule

module wrap_s3board_ram(clk,
			ram_a, ram_oe_n, ram_we_n,
			ram1_in, ram1_out, ram1_ce_n, ram1_ub_n, ram1_lb_n,
			ram2_in, ram2_out, ram2_ce_n, ram2_ub_n, ram2_lb_n);

   input clk;
   input [17:0] ram_a;
   input 	ram_oe_n, ram_we_n;
   input [15:0]  ram1_in;
   output [15:0] ram1_out;
   input 	 ram1_ce_n, ram1_ub_n, ram1_lb_n;
   input [15:0]  ram2_in;
   output [15:0] ram2_out;
   input 	 ram2_ce_n, ram2_ub_n, ram2_lb_n;

   wire [31:0] din, dout;
	   
   wire [31:0] addr;
   wire   rd, wr, ub, lb;

   assign addr = {14'b0, ram_a};
   
   assign din = {16'b0, ram1_in};

   assign ram1_out = dout[15:0];
   assign ram2_out = 0;

   assign rd = ~ram_oe_n;
   assign wr = ~ram_we_n;
   assign ub = ~ram1_ub_n;
   assign lb = ~ram1_lb_n;

   import "DPI-C" function void dpi_ram(input integer a,
					input integer r,
					input integer w,
					input integer u,
					input integer l,
					input integer in,
					output integer out);

//   always @(addr or rd or wr or din or dout)
   always @(posedge clk or negedge clk)
     begin
	dpi_ram(addr,
		{31'b0, rd},
		{31'b0, wr},
		{31'b0, ub},
		{31'b0, lb},
		din,
		dout);
     end
   
endmodule


module test_top;

   wire rs232_txd;
   reg 	rs232_rxd;

   reg [3:0] button;
   wire [7:0] led;
   reg sysclk;

   wire [7:0] sevenseg;
   wire [3:0] sevenseg_an;
   reg [7:0] slideswitch;

   wire [17:0] ram_a;
   wire        ram_oe_n;
   wire        ram_we_n;

   wire [15:0]  ram1_io;
   wire 	ram1_ce_n;
   wire 	ram1_ub_n;
   wire 	ram1_lb_n;

   wire [15:0] 	ram2_io;
   wire 	ram2_ce_n;
   wire 	ram2_ub_n;
   wire 	ram2_lb_n;

   wire [15:0] ide_data_bus;
   wire        ide_dior;
   wire        ide_diow;
   wire [1:0]  ide_cs;
   wire [2:0]  ide_da;

   top top(.rs232_txd(rs232_txd),
	   .rs232_rxd(rs232_rxd),
	   .button(button),
	   .led(led),
	   .sysclk(sysclk),
	   .sevenseg(sevenseg),
	   .sevenseg_an(sevenseg_an),
	   .slideswitch(slideswitch),
	   .ram_a(ram_a),
	   .ram_oe_n(ram_oe_n),
	   .ram_we_n(ram_we_n),
	   .ram1_io(ram1_io),
	   .ram1_ce_n(ram1_ce_n),
	   .ram1_ub_n(ram1_ub_n),
	   .ram1_lb_n(ram1_lb_n),
	   .ram2_io(ram2_io),
	   .ram2_ce_n(ram2_ce_n),
	   .ram2_ub_n(ram2_ub_n),
	   .ram2_lb_n(ram2_lb_n),
	  .ide_data_bus(ide_data_bus),
	  .ide_dior(ide_dior),
	  .ide_diow(ide_diow),
	  .ide_cs(ide_cs),
	  .ide_da(ide_da));

   wire [15:0] ide_data_in;
   wire [15:0] ide_data_out;

   assign ide_data_bus = ~ide_dior ? ide_data_out : 16'bz;
   assign ide_data_in = ide_data_bus;

   wrap_ide wrap_ide(.clk(sysclk),
		     .ide_data_in(ide_data_in),
		     .ide_data_out(ide_data_out),
		     .ide_dior(ide_dior),
		     .ide_diow(ide_diow),
		     .ide_cs(ide_cs),
		     .ide_da(ide_da));

   wire [15:0] ram1_in;
   wire [15:0] ram1_out;

   assign ram1_io = ram_we_n ? ram1_out : 16'bz;
   assign ram1_in = ram1_io;

   wire [15:0] ram2_in;
   wire [15:0] ram2_out;

//   assign ram2_io = ram_we_n ? ram2_out : 16'bz;
//   assign ram2_in = ram2_io;
   assign ram2_io = 0;
   assign ram2_in = 0;

   wrap_s3board_ram wrap_s3board_ram(
				     .clk(sysclk),
				     .ram_a(ram_a),
				     .ram_oe_n(ram_oe_n),
				     .ram_we_n(ram_we_n),
				     .ram1_in(ram1_in),
				     .ram1_out(ram1_out),
				     .ram1_ce_n(ram1_ce_n),
				     .ram1_ub_n(ram1_ub_n),
				     .ram1_lb_n(ram1_lb_n),
				     .ram2_in(ram2_in),
				     .ram2_out(ram2_out),
				     .ram2_ce_n(ram2_ce_n),
				     .ram2_ub_n(ram2_ub_n),
				     .ram2_lb_n(ram2_lb_n)
				     );

   // debug
   always @(posedge sysclk)
     if (led[0])
       begin
	  $display("cpu halted");
	  $finish;
       end

`ifdef slower
   initial
     begin
	button = 0;
	slideswitch = 8'b0000_0000;
//	slideswitch = 8'b0000_0001;
//	slideswitch = 8'b0000_0010;
//	slideswitch = 8'b0000_0011;
//	slideswitch = 8'b0000_0100;
//	slideswitch = 8'b0000_1000;
     end
`endif

endmodule
