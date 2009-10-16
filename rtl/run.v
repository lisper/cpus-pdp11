// run.v
// pdp-11 in verilog - sim top level
// copyright Brad Parker <brad@heeltoe.com> 2009
//

`timescale 1ns / 1ns

`define sim_time 1

`define minimal_debug 1
//`define debug 1

//`define debug_vcd
//`define debug_log
`define debug_mmu
//`define debug_bus
`define debug_io
`define debug_ram_low
`define debug_tt_out
//`define debug_cpu_int

//`define use_rk_model 1
//`define use_ram_async 1
//`define use_ram_model 1

//`define use_ide_pli

`define use_ram_sync 1
`define use_ram_pli 1

`include "pdp11.v"
`include "mmu.v"
`include "bus.v"
`include "ram_sync.v"
`include "ram_async.v"
`include "ram_s3board.v"
`include "debounce.v"

module test;

   reg clk, reset;
   reg [15:0] switches;

   wire       rs232_tx;
   reg 	      rs232_rx;

   wire [15:0] ide_data_bus;
   wire        ide_dior, ide_diow;
   wire [1:0]  ide_cs;
   wire [2:0]  ide_da;

   reg [15:0]  starting_pc;

   wire [15:0] bus_addr_v;
   wire [21:0] bus_addr_p;
   wire [15:0] bus_data_in, bus_data_out;
   wire        bus_rd, bus_wr, bus_byte_op;
   wire        bus_arbitrate, bus_ack, bus_error;
   wire        interrupt;
   wire [7:0]  bus_int_ipl, bus_int_vector;
   wire [7:0]  interrupt_ack_ipl;
   wire [15:0] psw;
   wire        psw_io_wr;

   wire        bus_int;

   wire [15:0] pc;
   wire        halted;
   wire        waited;
   wire        trapped;

   wire [1:0]  bus_cpu_cm;
   wire        bus_i_access;

   wire        mmu_fetch_va;
   wire        mmu_abort;
   wire        mmu_trap;

   reg 	       tracing;
   
   
   pdp11 cpu(.clk(clk),
	     .reset(reset),
	     .initial_pc(starting_pc),
	     .halted(halted),
	     .waited(waited),
	     .trapped(trapped),
	     
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
	     .bus_cpu_cm(bus_cpu_cm),

	     .mmu_fetch_va(mmu_fetch_va),
	     .mmu_abort(mmu_abort),
	     .mmu_trap(mmu_trap),
	     
	     .bus_int(bus_int),
	     .bus_int_ipl(bus_int_ipl),
	     .bus_int_vector(bus_int_vector),
	     .interrupt_ack_ipl(interrupt_ack_ipl),

	     .pc(pc),
	     .psw(psw),
	     .psw_io_wr(psw_io_wr));

   wire        pxr_wr;
   wire        pxr_rd;
   wire [7:0]  pxr_addr;
   wire [15:0] pxr_data_in;
   wire [15:0] pxr_data_out;
   
   mmu mmu1(.clk(clk),
	    .reset(reset),
	    .cpu_va(bus_addr_v),
	    .cpu_cm(bus_cpu_cm),
	    .cpu_rd(bus_rd),
	    .cpu_wr(bus_wr),
	    .cpu_i_access(bus_i_access),
	    .cpu_pa(bus_addr_p),
	    .fetch_va(mmu_fetch_va),
	    .signal_abort(mmu_abort),
	    .signal_trap(mmu_trap),
	    .pxr_wr(pxr_wr),
	    .pxr_rd(pxr_rd),
	    .pxr_addr(pxr_addr),
	    .pxr_data_in(pxr_data_in),
	    .pxr_data_out(pxr_data_out));
   
     
   wire [21:0] ram_addr;
   wire [15:0] ram_data_in, ram_data_out;
   wire        ram_rd, ram_wr, ram_byte_op;
   wire [4:0]  rk_state;
   wire signal_mmu_trap;
   
   bus bus1(.clk(clk),
	    .brgclk(clk),
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
	    .rs232_tx(rs232_tx),
	    .rs232_rx(rs232_rx));

`ifdef use_ram_sync
   wire        ram_wr_short;
   assign      ram_wr_short = ram_wr & ~clk;

   ram_sync ram1(.clk(clk),
		 .reset(reset),
		 .addr(ram_addr[15:0]),
		 .data_in(ram_data_out),
		 .data_out(ram_data_in),
		 .rd(ram_rd),
		 .wr(ram_wr_short),
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

/*
    ram_s3board ram2(.ram_a(ram_a),
		    .ram_oe_n(ram_oe_n),
		    .ram_we_n(ram_we_n),
		    .ram1_io(ram1_io),
		    .ram1_ce_n(ram1_ce_n),
		    .ram1_ub_n(ram1_ub_n), .ram1_lb_n(ram1_lb_n),
		    .ram2_io(ram2_io),
		    .ram2_ce_n(ram2_ce_n),
		    .ram2_ub_n(ram2_ub_n), .ram2_lb_n(ram2_lb_n));
*/
`endif

   
   reg [1023:0] arg;
   integer 	n;

   initial
     begin
`ifndef verilator
	$timeformat(-9, 0, "ns", 7);
`endif

	starting_pc = 16'o173000;
//	starting_pc = 16'o0200;
//	starting_pc = 16'o0500;

`ifdef __ICARUS__
 `define no_scan
`endif
`ifdef verilator
 `define no_scan
`endif

`ifdef no_scan
	n = 0;
`else
 	n = $scan$plusargs("pc=", arg);
`endif
	if (n > 0)
	  begin
//	     $sformat(arg, "%o", starting_pc);
//	     $sformat(starting_pc, "%o", arg);
//	     $sscanf(arg, "%o", starting_pc);
	     $display("arg %s pc %o", arg, starting_pc);
	  end
	
`ifdef debug_log
`else
`ifdef __CVER__
	$nolog;
`endif
`endif
	
`ifdef debug_vcd
	$dumpfile("pdp11.vcd");
	$dumpvars(0, test.cpu, test.bus1);
`endif
     end

   initial
     begin
	clk = 0;
	reset = 0;
	switches = 0;
	rs232_rx = 0;
	
	#1 begin
           reset = 1;
	end

	#40 begin
           reset = 0;
	end
	
//       #5000000 $finish;
     end

`ifdef use_ide_pli
   always @(posedge clk)
     begin
	$pli_ide(ide_data_bus, ide_dior, ide_diow, ide_cs, ide_da);
     end
`endif
   
   always
     begin
	#10 clk = 0;
	#10 clk = 1;
     end

   //----
   integer cycle;

   initial
     cycle = 0;

   always @(posedge cpu.clk)
     begin
`ifdef minimal_debug
	if (cpu.istate == 1 && cpu.pc == 16'o137046) tracing = 1;
	
	if (cpu.istate == 1 && tracing) //f1
	  $pli_pdp11dis(cpu.pc, cpu.isn, 0, 0);
`endif
`ifdef debug
	cycle = cycle + 1;
	#1 begin
	   if (cpu.istate == 1)
	     $display("------------------------------");
	   $display("cycle %d, pc %o, psw %o, istate %d",
		    cycle, cpu.pc, cpu.psw, cpu.istate);
	end

	if (cpu.istate == 1) //f1
	  $pli_pdp11dis(cpu.pc, cpu.isn, 0, 0);
	
	if (0) $display("[%o %o %o %o %o %o %o %o %b %o %o]",
			bus_addr_v, bus_addr_p, bus_rd, bus_wr,
			bus_data_in, bus_data_out, 
			bus1.ram_data_in, bus1.iopage_out, bus1.iopage_out,
			mmu1.pxr_data_out,
			bus1.iopage1.mmu_regs1.data_out);

	if (0) $display("[[%o %o %o %o %o %o %o]]",
			bus1.iopage1.bootrom_data_out,
			bus1.iopage1.mmu_data_out ,
			bus1.iopage1.tt_data_out ,
			bus1.iopage1.clk_data_out ,
			bus1.iopage1.sr_data_out ,
			bus1.iopage1.psw_data_out ,
			bus1.iopage1.rk_data_out);
`endif

	if (cpu.istate == 0)
	  $finish;
     end

endmodule

