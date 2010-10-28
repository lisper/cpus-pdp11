//
// pdp-11 in verilog - fpga top level
// copyright Brad Parker <brad@heeltoe.com> 2009-2010
//

module top(rs232_txd, rs232_rxd,
	   button, led, sysclk,
	   sevenseg, sevenseg_an,
	   slideswitch,
	   ram_a, ram_oe_n, ram_we_n,
	   ram1_io, ram1_ce_n, ram1_ub_n, ram1_lb_n,
	   ram2_io, ram2_ce_n, ram2_ub_n, ram2_lb_n,
	   ide_data_bus, ide_dior, ide_diow, ide_cs, ide_da);

   output	rs232_txd;
   input	rs232_rxd;

   input [3:0] 	button;

   output [7:0] led;
   input 	sysclk;

   output [7:0] sevenseg;
   output [3:0] sevenseg_an;

   input [7:0] 	slideswitch;

   output [17:0] ram_a;
   output 	 ram_oe_n;
   output 	 ram_we_n;

   inout [15:0]	 ram1_io;
   output 	 ram1_ce_n;
   output 	 ram1_ub_n;
   output 	 ram1_lb_n;

   inout [15:0]	 ram2_io;
   output 	 ram2_ce_n;
   output 	 ram2_ub_n;
   output 	 ram2_lb_n;
   
   inout [15:0]  ide_data_bus;
   output 	 ide_dior, ide_diow;
   output [1:0]  ide_cs;
   output [2:0]  ide_da;

   //
   wire         reset;
   wire [31:0] 	initial_pc/*verilator public_flat*/;
   wire [15:0] 	pc;
   wire 	halted;
   wire 	waited;
   wire 	trapped;
   wire         soft_reset;

   wire 	mmu_fetch_va;
   wire 	mmu_trap_odd;
   wire 	mmu_abort;
   wire 	mmu_trap;
   wire 	mmu_wr_inhibit;
   
   assign initial_pc = 32'o173000;

`ifndef sim_time
// `define slower
`endif

`ifdef slower
   //-----------
   reg clk;
   reg [24:0] clkdiv;
   wire [24:0] clkmax;

   assign clkmax = (slideswitch[3:0] == 4'd0)  ? 25'h1 :
		   (slideswitch[3:0] == 4'd1)  ? 25'h2 :
		   (slideswitch[3:0] == 4'd2)  ? 25'h1ff :
		   (slideswitch[3:0] == 4'd3)  ? 25'h7ff :
		   (slideswitch[3:0] == 4'd4)  ? 25'h1fff :
		   (slideswitch[3:0] == 4'd5)  ? 25'h7fff :
		   (slideswitch[3:0] == 4'd6)  ? 25'h1ffff :
		   (slideswitch[3:0] == 4'd7)  ? 25'h7ffff :
		   (slideswitch[3:0] == 4'd8)  ? 25'h1fffff :
		   (slideswitch[3:0] == 4'd9)  ? 25'h3fffff :
		   (slideswitch[3:0] == 4'd10) ? 25'h7fffff :
   		   (slideswitch[3:0] == 4'd11) ? 25'hffffff :
		   25'h1ffffff;
     
   always @(posedge sysclk)
     begin
        if (clkdiv == clkmax)
	  begin
             clk <= ~clk;
             clkdiv <= 0;
	  end
	else
          clkdiv <= clkdiv + 25'b1;
     end
   //-----------
`else
   wire 	clk/* verilator public_flat_rw @(sysclk) */;
   assign clk = sysclk;
`endif

wire [3:0] oled;
wire [4:0] rk_state;
   
   reset_btn reset_btn(.clk(sysclk), .in(button[3]), .out(reset));

   display show_pc(.clk(sysclk), .reset(reset),
		   .pc(pc), .dots(pc[15:12]),
		   .led(oled[3:0]),
		   .sevenseg(sevenseg), .sevenseg_an(sevenseg_an));
   assign led = {rk_state, trapped, waited, halted};

//   display_hex show_data(.clk(sysclk), .reset(reset),
//			 .hex(ide_data_bus), .dots(4'b0),
//			 .sevenseg(sevenseg), .sevenseg_an(sevenseg_an));

//   assign led = ide_data_bus[7:0];
//   assign led[7:4] = {ide_da[1], ide_da[0], ide_cs};
//   assign led = {ide_cs, ide_da[1], ide_da[0], ide_data_bus[7:4]};
//   assign led = {ide_cs, ide_dior, ide_diow, ide_data_bus[7:4]};
//   assign led = {ide_cs, ide_diow, rk_state};
   
   //
   wire [15:0] bus_addr_v;
   wire [21:0] bus_addr_p;
   wire [15:0] bus_data_in, bus_data_out;
   wire        bus_rd, bus_wr, bus_byte_op;
   wire        bus_arbitrate, bus_ack, bus_error;
   wire        bus_int;
   wire [7:0]  bus_int_ipl, bus_int_vector;
   wire [7:0]  interrupt_ack_ipl;
   wire [15:0] psw;
   wire        psw_io_wr;
   wire [1:0]  bus_cpu_cm;
   wire        bus_i_access;
   wire        bus_d_access;
   
   pdp11 cpu(.clk(clk),
	     .reset(reset),
	     .initial_pc(initial_pc[15:0]),
	     .halted(halted),
	     .waited(waited),
	     .trapped(trapped),
	     .soft_reset(soft_reset),
	     
	     .bus_addr(bus_addr_v),
	     .bus_data_in(bus_data_out),
	     .bus_data_out(bus_data_in),
	     .bus_rd(bus_rd),
	     .bus_wr(bus_wr),
	     .bus_byte_op(bus_byte_op),
	     .bus_arbitrate(bus_arbitrate),
	     .bus_ack(bus_ack),
	     .bus_error(bus_error),
	     .bus_i_access(bus_i_access),
	     .bus_d_access(bus_d_access),
	     .bus_cpu_cm(bus_cpu_cm),

	     .mmu_fetch_va(mmu_fetch_va),
	     .mmu_trap_odd(mmu_trap_odd),
	     .mmu_abort(mmu_abort),
	     .mmu_trap(mmu_trap),
	     .mmu_wr_inhibit(mmu_wr_inhibit),
	     
	     .bus_int(bus_int),
	     .bus_int_ipl(bus_int_ipl),
	     .bus_int_vector(bus_int_vector),
	     .interrupt_ack_ipl(interrupt_ack_ipl),

	     .pc(pc),
	     .psw(psw),
	     .psw_io_wr(psw_io_wr));
   
   wire [21:0] ram_addr;
   wire [15:0] ram_data_in, ram_data_out;
   wire        ram_rd, ram_wr, ram_byte_op;

   wire        ram_oe_n, ram_we_n, ram_ce_n;
   wire [15:0] ram1_io, ram2_io;

   wire [15:0] switches;
   assign switches = {8'b0, slideswitch};

   wire        pxr_wr;
   wire        pxr_rd;
   wire [1:0]  pxr_be;
   wire [7:0]  pxr_addr;
   wire [15:0] pxr_data_in;
   wire [15:0] pxr_data_out;
   
   wire        signal_mmu_trap;

   bus bus1(.clk(clk),
	    .brgclk(sysclk),
	    .reset(reset),
	    .bus_addr(bus_addr_p),
	    .bus_data_in(bus_data_in),
	    .bus_data_out(bus_data_out),
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

	    .pxr_wr(pxr_wr),
	    .pxr_rd(pxr_rd),
	    .pxr_be(pxr_be),
	    .pxr_addr(pxr_addr),
	    .pxr_data_in(pxr_data_out),
	    .pxr_data_out(pxr_data_in),
	    .pxr_trap(signal_mmu_trap),
	    
   	    .ide_data_bus(ide_data_bus),
	    .ide_dior(ide_dior), .ide_diow(ide_diow),
	    .ide_cs(ide_cs), .ide_da(ide_da),

	    .psw(psw),
	    .psw_io_wr(psw_io_wr),
	    .switches(switches),
	    .rk_state(rk_state),
	    .rs232_tx(rs232_txd),
	    .rs232_rx(rs232_rxd));

`ifdef no_mmu
   null_mmu mmu(.clk(clk),
		.reset(reset),
		.soft_reset(soft_reset),
		.cpu_va(bus_addr_v),
		.cpu_cm(bus_cpu_cm),
		.cpu_rd(bus_rd),
		.cpu_wr(bus_wr),
		.cpu_i_access(bus_i_access),
		.cpu_d_access(bus_d_access),
		.cpu_trap(trapped),
		.cpu_pa(bus_addr_p),
		.fetch_va(mmu_fetch_va),
		.trap_odd(mmu_trap_odd),
		.signal_abort(mmu_abort),
		.signal_trap(mmu_trap),
		.pxr_wr(pxr_wr),
		.pxr_rd(pxr_rd),
		.pxr_be(pxr_be),
		.pxr_addr(pxr_addr),
		.pxr_data_in(pxr_data_in),
		.pxr_data_out(pxr_data_out));
`else   
   mmu mmu1(.clk(clk),
	    .reset(reset),
	    .soft_reset(soft_reset),
	    .cpu_va(bus_addr_v),
	    .cpu_cm(bus_cpu_cm),
	    .cpu_rd(bus_rd),
	    .cpu_wr(bus_wr),
	    .cpu_i_access(bus_i_access),
	    .cpu_d_access(bus_d_access),
	    .cpu_trap(trapped),
	    .cpu_pa(bus_addr_p),
	    .fetch_va(mmu_fetch_va),
	    .trap_odd(mmu_trap_odd),
	    .signal_abort(mmu_abort),
	    .signal_trap(mmu_trap),
	    .pxr_wr(pxr_wr),
	    .pxr_rd(pxr_rd),
	    .pxr_be(pxr_be),
	    .pxr_addr(pxr_addr),
	    .pxr_data_in(pxr_data_in),
	    .pxr_data_out(pxr_data_out));
`endif

   ram_async ram1(.clk(clk),
		  .reset(reset),

		  .addr(ram_addr[17:0]),
		  .data_in(ram_data_out),
		  .data_out(ram_data_in),
		  .rd(ram_rd),
		  .wr(ram_wr),
		  .wr_inhibit(mmu_wr_inhibit),
		  .byte_op(ram_byte_op),

		  .ram_a(ram_a),
		  .ram_oe_n(ram_oe_n), .ram_we_n(ram_we_n),

		  .ram1_io(ram1_io), .ram1_ce_n(ram1_ce_n),
		  .ram1_ub_n(ram1_ub_n), .ram1_lb_n(ram1_lb_n),
		   
		  .ram2_io(ram2_io), .ram2_ce_n(ram2_ce_n), 
		  .ram2_ub_n(ram2_ub_n), .ram2_lb_n(ram2_lb_n));
   
endmodule
