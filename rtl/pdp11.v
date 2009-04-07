//
// pdp-11 in verilog
// copyright Brad Parker <brad@heeltoe.com> 2009
//

`include "ipl_below.v"
`include "add8.v"
`include "bus.v"
`include "execute.v"

module pdp11(clk, reset_n, switches,
	     ide_data_bus, ide_dior, ide_diow, ide_cs, ide_da);

   input clk, reset_n;
   input [15:0] switches;

   inout [15:0] ide_data_bus;
   output 	ide_dior, ide_diow;
   output [1:0] ide_cs;
   output [2:0] ide_da;


   // state
   reg 		halted;
   reg 		waited;

   reg 		trap;
   reg 		trap_bpt;
   reg 		trap_iot;
   reg 		trap_emt;
   reg 		trap_trap;
   reg 		trap_odd;
   reg 		trap_bus;
   reg 		trap_ill;
   reg 		trap_priv;

   reg 		interrupt;
   reg [7:0] 	interrupt_vector;

   reg [15:0] 	psw;

   wire 	cc_n, cc_z, cc_v, cc_c;
   wire [2:0] 	ipl;

   reg [15:0] 	regs[0:7];

   wire [15:0] 	pc;
   wire [15:0] 	sp;
   
   // wires 
   wire 	assert_wait;
   wire 	assert_halt;
   wire 	assert_reset;
   wire 	assert_bpt;
   wire 	assert_iot;
   wire 	assert_trap_odd;
   wire 	assert_trap_ill;
   wire 	assert_trap_priv;
   wire 	assert_trap_emt;
   wire 	assert_trap_trap;
   wire 	assert_trap_bus;

   wire 	assert_int;
   wire [7:0] 	assert_int_vec;
   wire [7:0] 	assert_int_ipl;

   wire [3:0] 	isn_15_12;
   wire [6:0] 	isn_15_9;
   wire [10:0] 	isn_15_6;
   wire [5:0] 	isn_11_6;
   wire [2:0] 	isn_11_9;
   wire [5:0] 	isn_5_0;
   wire [3:0] 	isn_3_0;

   wire 	is_isn_rss;
   wire 	is_isn_rdd;
   wire 	is_isn_rxx;
   wire 	is_isn_r32;
   wire 	is_isn_lowbyte;
   wire 	is_isn_byte;
   wire 	is_illegal;
   wire 	no_operand;
   wire 	store_result;
   wire 	store_result32;
   wire 	store_ss_reg;

   wire 	need_srcspec_dd_word,
		need_srcspec_dd_byte,
		need_srcspec_dd;

   wire 	need_destspec_dd_word,
		need_destspec_dd_byte,
		need_destspec_dd,
		need_pop_reg,
		need_pop_pc_psw,
		need_push_state,
		need_dest_data,
		need_src_data;

   wire 	need_s1,
		need_s2,
		need_s4,
		need_d1,
		need_d2,
		need_d4;

   wire [15/*21*/:0] 	dd_ea_mux;
   wire [15/*21*/:0] 	ss_ea_mux;

   wire [15/*21*/:0] 	new_pc;
   wire 	latch_pc;

   wire 	new_cc_n, new_cc_z, new_cc_v, new_cc_c;
   wire 	latch_cc;

   wire [3:0] 	new_psw_cc;
   wire [2:0] 	new_psw_prio;
   wire 	latch_psw_prio;

   // regs
   reg [15:0] 	isn;
   reg [15/*21*/:0] 	dd_ea;
   reg [15/*21*/:0] 	ss_ea;

   wire [2:0] 	dd_mode, dd_reg;

   wire 	dd_dest_mem,
		dd_dest_reg,
		dd_ea_ind,
		dd_post_incr,
		dd_pre_dec;

   wire [2:0] 	ss_mode, ss_reg;

   // regs
   reg [15:0] 	ss_data;	// result of s states
   reg [15:0] 	dd_data;	// result of d states
   reg [15:0] 	e1_data;	// result of e states

   wire 	ss_dest_mem,
		ss_dest_reg,
		ss_ea_ind,
		ss_post_incr,
		ss_pre_dec;

   wire [15:0] 	pc_mux;
   wire [15:0] 	sp_mux;

   wire [15:0] 	ss_data_mux;
   wire [15:0] 	dd_data_mux;
   wire [15:0] 	e1_data_mux;

   wire [15:0] 	e1_result;
   wire [31:0] 	e32_result;

   // cpu modes
   parameter 	mode_kernel = 3'd0;
   parameter 	mode_super = 3'd1;
   parameter 	mode_undef = 3'd2;
   parameter 	mode_user = 3'd3;

   reg [2:0] 	current_mode;

   //
   // main cpu states
   //
   // f1 fetch;	   clock isn
   // c1 decode;
   //
   // s1 source1;	   clock ss_data
   // s2 source2;	   clock ss_data
   // s3 source3;	   clock ss_data
   // s4 source4;	   clock ss_data
   //
   // d1 dest1;	   clock dd_data
   // d2 dest2;	   clock dd_data
   // d3 dest3;	   clock dd_data
   // d4 dest4;	   clock dd_data
   //
   // e1 execute;	   clock pc, sp & reg+-
   // w1 writeback1;  clock e1_result
   //
   // o1 pop pc	   mem read
   // o2 pop psw	   mem read
   // o3 pop reg	   mem read
   //
   // p1 push sp	   mem write
   //
   // t1 push psw	   mem write
   // t2 push pc	   mem write
   // t3 read pc	   mem read
   // t4 read psw	   mem read
   //
   // i1 interrupt wait
   //
   // minimum states/instructions = 3
   // maximum states/instructions = 10
   //
   // mode,symbol,ea1	ea2		ea3		data		side-eff
   // 
   // 0	R	x	x		x		R		x
   // 1	(R)	R	x		x		M[R]		x
   // 2	(R)+	R	X		x		M[R]		R<-R+n
   // 3	@(R)+	R	M[R]		x		M[M[R]]		R<-R+2
   // 4	-(R)	R-2	x		x		M[R-2]		R<-R-n
   // 5	@-(R)	R-2	M[R-2]		x		M[M[R-2]]	R<-R-2
   // 6	X(R)	PC	M[PC]+R		x		M[M[PC]+R]	x
   // 7	@X(R)	PC	M[PC]+R		M[M[PC]+R]	M[M[M[PC]+R]]	x
   //
   // mode 0 -
   //  f1  pc++
   //  c1
   //  d1  dd_data = r
   //  e1  e1_result
   //             
   // mode 1 -
   //  f1  pc++
   //  c1
   //  d1  dd_ea = r
   //  d4  dd_data = bus_data	optional
   //  e1  e1_result
   //
   // mode 2 -
   //  f1  pc++
   //  c1
   //  d1  dd_ea = r, r++
   //  d4  dd_data = bus_data	optional
   //  e1  e1_result
   //
   // mode 3 -
   //  f1  pc++
   //  c1  dd_ea = r
   //  d1  dd_ea = bus_data, r++
   //  d4  dd_data = bus_data	optional
   //  e1  e1_result
   //
   // mode 4 -
   //  f1  pc++
   //  c1
   //  d1  dd_ea = r-x, r--
   //  d4  dd_data = bus_data	optional
   //  e1  e1_result
   //
   // mode 5 -
   //  f1  pc++
   //  c1
   //  d1  dd_ea = r-x, r--
   //  d2  dd_ea = bus_data
   //  d4  dd_data = bus_data	optional
   //  e1  e1_result
   //
   // mode 6 -
   //  f1  pc++
   //  c1
   //  d1  dd_ea = pc, pc++
   //  d2  dd_ea = bus_data + regs[dd_reg]
   //  d4  dd_data = bus_data	optional
   //  e1  e1_result
   //
   // mode 7 -
   //  f1  pc++
   //  c1
   //  d1  dd_ea = pc, pc++
   //  d2  dd_ea = bus_data + regs[dd_reg]
   //  d3  dd_ea = bus_data
   //  d4  dd_data = bus_data	optional
   //  e1  e1_result
   //

   parameter 	h1 = 5'b00000;
   parameter 	f1 = 5'b00001;
   parameter 	c1 = 5'b00010;
   parameter 	s1 = 5'b00011;
   parameter 	s2 = 5'b00100;
   parameter 	s3 = 5'b00101;
   parameter 	s4 = 5'b00110;
   parameter 	d1 = 5'b00111;
   parameter 	d2 = 5'b01000;
   parameter 	d3 = 5'b01001;
   parameter 	d4 = 5'b01010;
   parameter 	e1 = 5'b01011;
   parameter 	w1 = 5'b01100;
   parameter 	o1 = 5'b01101;
   parameter 	o2 = 5'b01110;
   parameter 	o3 = 5'b01111;
   parameter 	p1 = 5'b10001;
   parameter 	t1 = 5'b10010;
   parameter 	t2 = 5'b10011;
   parameter 	t3 = 5'b10100;
   parameter 	t4 = 5'b10101;
   parameter 	i1 = 5'b10110;

   wire [4:0] 	new_istate;
   reg [4:0] 	istate;


   //
   // bus unit
   //
   wire [15:0] bus_in;
   wire [15:0] bus_out;
   wire [15:0] bus_addr;
   wire        bus_wr, bus_rd, bus_byte_op;

   wire        psw_io_wr;
   wire        bus_ack;
        
   bus bus1(.clk(clk), .reset(~reset_n),
	    .bus_addr({ 6'b0, bus_addr }),
	    .data_in(bus_in),
	    .data_out(bus_out),
	    .bus_rd(bus_rd),
	    .bus_wr(bus_wr),
	    .bus_byte_op(bus_byte_op),

	    .bus_ack(bus_ack),
	    .bus_error(assert_trap_bus),
	    .interrupt(assert_int),
	    .vector(assert_int_vec),

   	    .ide_data_bus(ide_data_bus),
	    .ide_dior(ide_dior), .ide_diow(ide_diow),
	    .ide_cs(ide_cs), .ide_da(ide_da),

	    .psw(psw), .psw_io_wr(psw_io_wr),
	    .switches(switches)
	    );
   
  
   //
   // execute unit
   //
   execute exec1(.clk(clk), .reset(~reset_n),
		 .enable(istate == e1 ? 1'b1 : 1'b0),
		 .pc(pc), .psw(psw),
		 .ss_data(ss_data), .dd_data(dd_data),
		 .cc_n(cc_n), .cc_z(cc_z), .cc_v(cc_v), .cc_c(cc_c),
		 .current_mode(current_mode),

		 .dd_ea(dd_ea),
		 .ss_reg(ss_reg),
		 
		 .ss_reg_value(regs[ss_reg]),
		 .ss_rego1_value(regs[ss_reg | 1]),
		 
		 .isn(isn), 
		 .isn_15_12(isn_15_12), .isn_11_9(isn_11_9),
		 .isn_11_6(isn_11_6), .isn_5_0(isn_5_0), .isn_3_0(isn_3_0), 

		 .assert_halt(assert_halt),
		 .assert_wait(assert_wait),
		 .assert_trap_priv(assert_trap_priv),
		 .assert_trap_emt(assert_trap_emt), 
		 .assert_trap_trap(assert_trap_trap),
		 .assert_bpt(assert_bpt),
		 .assert_iot(assert_iot),
		 .assert_reset(assert_reset),
		 
		 .e1_result(e1_result), .e32_result(e32_result),

		 .new_pc(new_pc), .latch_pc(latch_pc), 
		 .new_cc_n(new_cc_n), .new_cc_z(new_cc_z), 
		 .new_cc_v(new_cc_v), .new_cc_c(new_cc_c),

		 .latch_cc(latch_cc),
		 .latch_psw_prio(latch_psw_prio),
		 .new_psw_prio(new_psw_prio));
   
   //
   // effective address mux;
   // set address of next memory operation in various states
   //

   //
   // source ea calculation:
   // decode - load from register or pc
   // s1 - if mode 6,7 add result of pc+2 fetch to reg
   // s2 - result of fetch
   // t1 - trap vector
   // t4 - incr ea
   //
   
   assign ss_ea_mux =
     (istate == c1 || istate == s1) ?
		     (ss_mode == 0 ? regs[ss_reg] :
		      ss_mode == 1 ? regs[ss_reg] :
		      ss_mode == 2 ? regs[ss_reg] :
		      ss_mode == 3 ? regs[ss_reg] :
		      ss_mode == 4 ? (is_isn_byte ? (regs[ss_reg]-1) :
				      (regs[ss_reg]-2)) :
		      ss_mode == 5 ? (is_isn_byte ? (regs[ss_reg]-1) :
				      (regs[ss_reg]-2)) :
		      ss_mode == 6 ? pc :
		      ss_mode == 7 ? pc :
		      0) :

     istate == s2 ?
		     (ss_mode == 3 ? bus_out :
		      ss_mode == 5 ? bus_out :
		      ss_mode == 6 ? (bus_out + regs[ss_reg]) & 16'hffff :
		      ss_mode == 7 ? (bus_out + regs[ss_reg]) & 16'hffff :
		      ss_ea) :
		     
     istate == s3 ? bus_out :

     istate == t1 ? ((trap_odd | trap_bus) ? 16'o4 :
		     trap_ill ? 16'o10 :
		     trap_priv ? 16'o10 :
		     trap_bpt ? 16'o14 :
		     trap_iot ? 16'o20 :
		     trap_emt ? 16'o30 :
		     trap_trap ? 16'o34 :
		     interrupt ? { 8'b0, interrupt_vector } :
		     0) :

     istate == t2 ? ss_ea :
     istate == t3 ? ss_ea + 2 :
     istate == t4 ? ss_ea + 2 :
		     0;

   //
   // dest ea calculation:
   // decode - load from register or pc
   // d1 - if mode 6,7 add result of pc+2 fetch to reg
   // d2 - result of fetch
   //
   assign dd_ea_mux = (istate == c1 || istate == d1) ?
		      (dd_mode == 0 ? regs[dd_reg] :
		       dd_mode == 1 ? regs[dd_reg] :
		       dd_mode == 2 ? regs[dd_reg] :
		       dd_mode == 3 ? regs[dd_reg] :
		       dd_mode == 4 ? regs[dd_reg] - (is_isn_byte ? 1 : 2) :
		       dd_mode == 5 ? regs[dd_reg] - (is_isn_byte ? 1 : 2) :
		       dd_mode == 6 ? pc :
		       dd_mode == 7 ? pc :
		       0) :

		      istate == d2 ?
		      (dd_mode == 3 ? bus_out :
		       dd_mode == 5 ? bus_out :
		       dd_mode == 6 ? (bus_out + regs[dd_reg]) :
		       dd_mode == 7 ? (bus_out + regs[dd_reg]) :
		       dd_ea) :

		      istate == d3 ? bus_out :
		      dd_ea;


   //
   // mux various data sources into ss, dd & e1 data registers
   //
   assign ss_data_mux =
	       (istate == c1 && (ss_mode == 0 || is_isn_rxx)) ? regs[ss_reg] :
	       (istate == s4) ? bus_out :
	       ss_data_mux;

   assign dd_data_mux =
	       (istate == c1 && dd_mode == 0) ? regs[dd_reg] :
	       (istate == c1 && (isn_15_6 == 10'o1067)) ? psw[7:0] : // mfps 
	       (istate == d4) ? bus_out :
	       dd_data_mux;

   assign e1_data_mux =
	       (istate == e1) ? e1_result :
	       e1_data_mux;


   //
   // mux sources of pc changes into new pc
   //
   wire trap_or_int = trap || interrupt;

   assign pc_mux =
	  (istate == f1 && !trap_or_int                  ) ? pc + 2 :
	  (istate == s1 && (ss_mode == 6 || ss_mode == 7)) ? pc + 2 :
	  (istate == d1 && (dd_mode == 6 || dd_mode == 7)) ? pc + 2 :
	  (istate == e1 && latch_pc                      ) ? new_pc :
	  (istate == o1 || istate == t3                  ) ? bus_out :
	  pc;

   //
   // mux source of sp changes
   //
   assign sp_mux =
		  (istate == o1 || istate == o2 || istate == o3) ? sp + 2 :
		  (istate == p1 ) ? sp - 2 :
		  (istate == t1 ) ? sp - 2 :
		  (istate == t2 ) ? sp - 2 :
		  sp;


   // shorthand
   assign cc_n = psw[3];
   assign cc_z = psw[2];
   assign cc_v = psw[1];
   assign cc_c = psw[0];
   assign ipl  = psw[7:5];
   
   assign sp = regs[6];
   assign pc = regs[7];
   

   //
   // psw_mux
   //
   // after e1, latch new psw cc bits or latch new prio
   // after o2|t4, latch new psw from memory read
   //
   assign new_psw_cc = {new_cc_n, new_cc_z, new_cc_v, new_cc_c};
   
   always @(posedge clk)
     if (reset_n == 0)
	  psw <= 16'o0340;
     else
       if (istate == e1 && latch_cc)
	 psw <= { psw[15:4], new_psw_cc};
       else
	 if (istate == e1 && latch_psw_prio)
	   psw <= { psw[15:8], new_psw_prio, psw[3:0]};
       else
         if (istate == o2 || istate == t4)
	   psw <= bus_out;
	 else
	   if (istate == w1 && psw_io_wr)
	     psw <= bus_out;

   //
   // instruction decode
   //
   assign isn_15_12 = isn[15:12];
   assign isn_15_9  = isn[15:9];
   assign isn_15_6  = isn[15:6];
   assign isn_11_6  = isn[11:6];
   assign isn_11_9  = isn[11:9];
   assign isn_5_0   = isn[5:0];
   assign isn_3_0   = isn[3:0];

   assign need_destspec_dd_word =
	(isn_15_6 == 10'o0001) ||				      // jmp
	(isn_15_6 == 10'o0003) ||				      // swab
	(isn_15_12 == 0 && (isn_11_6 >= 6'o40 && isn_11_6 <= 6'o63))||// jsr-asl
	(isn_15_12 == 0 && (isn_11_6 >= 6'o65 && isn_11_6 <= 6'o67))||// m*,sxt
	(isn_15_12 >= 4'o01 && isn_15_12 <= 4'o06) ||		      // mov-add
	(isn_15_9 >= 7'o070 && isn_15_9 <= 7'o074) || 		      // mul-xor
	(isn_15_12 == 4'o10 && (isn_11_6 >= 6'o65 && isn_11_6 <= 6'o66))||// mtpx
	(isn_15_12 == 4'o16);					      // sub

   assign need_destspec_dd_byte =
	(isn_15_12 == 4'o10 && (isn_11_6 >= 6'o50 && isn_11_6 <= 6'o64))||// xxxb
	(isn_15_12 == 4'o10 && (isn_11_6 == 6'o67)) ||		      // mfps
	(isn_15_12 >= 4'o11 && isn_15_12 < 10'o0016);		      // xxxb

   assign need_destspec_dd = need_destspec_dd_word | need_destspec_dd_byte;

   assign need_srcspec_dd_word = 
	 (isn_15_12 >= 4'o01 && isn_15_12 <= 4'o06) ||		// mov-add
	 (isn_15_12 == 4'o16);					// sub

   assign need_srcspec_dd_byte = 
	 (isn_15_12 >= 4'o11 && isn_15_12 <= 4'o15);		// movb-sub

   assign need_srcspec_dd = need_srcspec_dd_word | need_srcspec_dd_byte;

   assign no_operand =
	(isn_15_6 == 10'o0000 && isn_5_0 < 6'o10) ||
	(isn_15_6 == 10'o0002 && (isn_5_0 >= 6'o30 && isn_5_0 <= 6'o77)) ||
	(isn_15_12 == 0 && (isn_11_6 >= 6'o04 && isn_11_6 <= 6'o37)) ||
//	(isn_15_12 == 4'o07 && (isn_11_9 == 7)) ||
	(isn_15_9 == 7'o104) ||					// trap/emt
	0;
//	(isn_15_6 == 10'o0047);

   assign is_illegal =
	(isn_15_6 == 10'o0000 && isn_5_0 > 6'o07) ||
	(isn_15_6 == 10'o0001 && isn_5_0 < 6'o10) ||	// jmp rx
	(isn_15_6 == 10'o0002 && (isn_5_0 >= 6'o10 && isn_5_0 <= 6'o27)) ||
	(isn_15_12 == 4'o07 && (isn_11_9 == 5 || isn_11_9 == 6)) ||
	(isn_15_12 == 4'o17);

   assign need_pop_reg =
			(isn_15_6 == 10'o0002 && (isn_5_0 < 6'o10)) ||// rts
			(isn_15_6 == 10'o0064) ||		// mark
			(isn_15_6 == 10'o0070) ||		// csm
			(isn_15_6 == 10'o1065);			// mtpd
   
   assign need_pop_pc_psw =					// rti, rtt
	(isn_15_6 == 0 && (isn_5_0 == 6'o02 || isn_5_0 == 6'o06));
   
   assign need_push_state =
			   (isn_15_9 == 7'o004) ||		// jsr
			   (isn_15_6 == 10'o1065);		// mfpd

   assign assert_trap_ill = is_illegal;

   assign assert_trap_odd = (istate == f1) && (pc & 1);
   
   
   assign store_result32 =
			  (isn_15_9 == 7'o070) ||		// mul
			  (isn_15_9 == 7'o071) ||		// div
			  (isn_15_9 == 7'o072);			// ashc
   
   assign store_result =
		!no_operand &&
		!store_result32 &&
		!(isn_15_6 == 10'o0001) &&			// jmp
		!(isn_15_6 == 10'o0057) &&
		!(isn_15_6 == 10'o1057) &&			// tst/tstb
		!(isn_15_12 == 4'o02) &&
		!(isn_15_12 == 4'o12) &&			// cmp/cmpb
		!(isn_15_12 == 4'o03) &&			// bit
		!(isn_15_9 == 7'o004) &&			// jsr
		!((isn_15_6 >= 10'o1000) && (isn_15_6 <= 10'o1037)) &&// bcs-blo
		!((isn_15_6 >= 10'o0004) && (isn_15_6 <= 10'o0034));  // br-ble

   assign need_dest_data = 
		   !(isn_15_12 == 4'o01) &&			// mov
		   !(isn_15_12 == 4'o11) &&			// movb
		   !(isn_15_9 == 7'o04) &&			// jsr
		   !((isn_15_6 == 10'o0050) ||
		     (isn_15_6 == 10'o1050)) && 		// clr/clrb
		   !(isn_15_6 == 10'o0001);			// jmp

   assign is_isn_byte = isn[15] && !(isn_15_12 == 4'o16); 	// sub

   assign is_isn_rdd = 
		       (isn_15_9 == 7'o004) ||			// jsr
		       (isn_15_9 == 7'o074) ||			// xor
		       (isn_15_9 == 7'o077);			// sob

   assign is_isn_rss = 
		       (isn_15_9 == 7'o070) ||			// mul
		       (isn_15_9 == 7'o071) ||			// div
		       (isn_15_9 == 7'o072) ||			// ashc
		       (isn_15_9 == 7'o073);			// ash

   assign is_isn_rxx = is_isn_rdd || is_isn_rss;

   assign is_isn_r32 = (isn_15_9 == 7'o071);			// div

   assign need_src_data =
			 !((isn_15_6 == 10'o0050) ||
			   (isn_15_6 == 10'o1050));		// clr/clrb

   // ea setup - ss
   assign ss_mode = isn[11:9];
   assign ss_reg = isn[8:6];

   assign ss_dest_mem = ss_mode != 0;
   assign ss_dest_reg = ss_mode == 0;
   assign ss_ea_ind = ss_mode == 7;

   assign store_ss_reg = (isn_15_9 == 004 && ss_reg != 7);	// jsr

   assign ss_post_incr = need_srcspec_dd &&
			 (ss_mode == 2 || ss_mode == 3);

   assign ss_pre_dec = need_srcspec_dd &&
		       (ss_mode == 4 || ss_mode == 5);

   // ea setup - dd
   assign dd_mode = isn[5:3];
   assign dd_reg = isn[2:0];

   assign dd_dest_mem = dd_mode != 0;
   assign dd_dest_reg = dd_mode == 0;
   assign dd_ea_ind = dd_mode == 7;

   assign dd_post_incr = need_destspec_dd &&
			 (dd_mode == 2 || dd_mode == 3);

   assign dd_pre_dec = need_destspec_dd &&
		       (dd_mode == 4 || dd_mode == 5);

   // decide on next state
   assign need_s1 = need_srcspec_dd;
   assign need_d1 = need_destspec_dd;

   assign need_s2 = need_srcspec_dd && (ss_mode == 3 || ss_mode >= 5);
   assign need_d2 = need_destspec_dd && (dd_mode == 3 || dd_mode >= 5);

   assign need_s4 = need_srcspec_dd && ss_mode != 0 && need_src_data;
   assign need_d4 = need_destspec_dd && dd_mode != 0 && need_dest_data;


   // memory i/o
   assign bus_rd =
		  istate == f1 ||
		  (istate == s2 || istate == s3 || istate == s4) ||
		  (istate == d2 || istate == d3 || istate == d4) ||
		  (istate == o1 || istate == o2 || istate == o3) ||
		  (istate == t3 || istate == t4);
	   
   assign bus_wr =
	   (istate == w1 && store_result && dd_dest_mem) ||
	   istate == p1 ||
	   istate == t1 ||
	   istate == t2;

   assign bus_in =
		  istate == w1 ? e1_data :
		  istate == p1 ? regs[ss_reg] :
		  istate == t1 ? psw :
   		  istate == t2 ? pc : bus_in;
   
   assign bus_addr =
		    istate == f1 ? pc :
		    istate == s2 ? ss_ea :
		    istate == s3 ? ss_ea :
		    istate == s4 ? ss_ea :
		    istate == d2 ? dd_ea :
    		    istate == d3 ? dd_ea :
    		    istate == d4 ? dd_ea :
		    istate == w1 ? dd_ea :
		    istate == o1 ? sp :
		    istate == o2 ? sp :
		    istate == o3 ? sp :
		    istate == t1 ? sp - 2 :
		    istate == t2 ? sp - 2 :
		    istate == t3 ? ss_ea/*_mux*/ :
		    istate == t4 ? ss_ea/*_mux*/ : bus_addr;
   

   assign bus_byte_op = (istate == w1 || istate == s4 || istate == d4) ?
			is_isn_byte: 0;

   //
   // clock data
   //
   always @(posedge clk)
     if (reset_n == 0)
       begin
	  regs[0] <= 0;
	  regs[1] <= 0;
	  regs[2] <= 0;
	  regs[3] <= 0;
	  regs[4] <= 0;
	  regs[5] <= 0;
	  regs[6] <= 0;
	  regs[7] <= 16'o0500;
//  	  regs[7] <= 16'o173000;

	  isn <= 0;
       end
     else
       begin

	  if (istate != w1)
	    begin
	       regs[7] <= pc_mux; // pc
	       regs[6] <= sp_mux; // sp
	    end

	  case (istate)
	    f1:
	    begin
	       isn <= bus_out;
	    end

	  c1:
	    begin
	    end

	  s1:
	    begin
	       if (ss_post_incr)
		 begin
		    regs[ss_reg] <= regs[ss_reg] +
				    ((need_srcspec_dd_byte &&
				      ss_reg < 6 && ss_mode == 2) ? 1 : 2);
		    $display(" R%d <- (ss r++)", ss_reg);
		 end
	       else
		 if (ss_pre_dec)
		   begin
		      regs[ss_reg] <= regs[ss_reg] -
				      ((need_srcspec_dd_byte &&
					ss_reg < 6 && ss_mode == 4) ? 1 : 2);
		      $display(" R%d <- (ss r--)", ss_reg);
		   end
	    end // case: s1
	  
	  s2:
	    begin
	    end

	  s3:
	    begin
	    end

	  s4:
	    begin
	    end

	  d1:
	    begin
               if (dd_post_incr)
		 begin
		    regs[dd_reg] <= regs[dd_reg] +
				    ((need_destspec_dd_byte &&
				      dd_reg < 6 && dd_mode == 2) ? 1 : 2);
		    $display(" R%d <- (dd r++)", dd_reg);
		 end
               else
		 if (dd_pre_dec)
		   begin
		      regs[dd_reg] <= regs[dd_reg] - 
				      ((need_destspec_dd_byte &&
					dd_reg < 6 && dd_mode == 4) ? 1 : 2);
		      $display(" R%d <- (dd r--)", dd_reg);
		   end
	    end

	  d2:
	    begin
	       // note: cycle needs to be long enough for memory read and
	       // addition to take place
	       // (or, possibly move mode 6/7 +reg to ea for d3)
	    end

	  d3:
	    begin
	    end

	  d4:
	    begin
	    end

	  e1:
	    begin
	    end
	  
	  w1:
	    begin
	       if (store_result && dd_dest_mem)
		 begin
		 end
	       else
		 if (store_result && dd_dest_reg)
		   begin
		      $display(" r%d <- %0o (dd)", dd_reg, e1_data);
		      regs[dd_reg] <= e1_data;
		   end
		 else
		   if (store_ss_reg)
		     begin
			$display(" r%d <- %0o (ss)", ss_reg, e1_data);
			regs[ss_reg] <= e1_data;
		     end
		   else
		     if (store_result32)
		       begin
			  $display(" r%d <- %0o (e32)",
				   ss_reg, e32_result[31:16]);
			  $display(" r%d <- %0o (e32)",
				   ss_reg|1, e32_result[15:0]);
			  regs[ss_reg    ] <= e32_result[31:16];
			  regs[ss_reg | 1] <= e32_result[15:0];
		       end
	    end // case: w1
	  

	  o1:
	    begin
	    end

	  o2:
	    begin
	    end

	  o3:
	    begin
	    end


	  p1:
	    begin
	    end

	  t1:
	    begin
	    end

	  t2:
	    begin
	    end

	  t3:
	    begin
	    end

	  t4:
	    begin
	    end

	  i1:
	    begin
	    end
	endcase // case(istate)
       end // else: !if(reset_n == 0)
   
   
   //
   // check_for_traps
   //
   wire ok_to_assert_trap;
   wire ok_to_reset_trap;

   assign ok_to_assert_trap =
			     istate == f1 || istate == c1 || istate == e1 ||
			     istate == s4 || istate == d4 || istate == w1;
   
   assign ok_to_reset_trap = istate == t1;

   always @(posedge clk)
     if (reset_n == 0)
       begin
	     trap_odd <= 0;
	     trap_ill <= 0;
	     trap_priv <= 0;
	     trap_bpt <= 0;
	     trap_iot <= 0;
	     trap_emt <= 0;
	     trap_trap <= 0;
	     trap_bus <= 0;
	     trap <= 0;
       end
   else
     begin
	if (ok_to_reset_trap)
	  begin
	     trap_odd <= 0;	// f1
	     trap_ill <= 0;	// c1
	     trap_priv <= 0;	// e1
	     trap_bpt <= 0;
	     trap_iot <= 0;
	     trap_emt <= 0;
	     trap_trap <= 0;
	     trap_bus <= 0;
	     trap <= 0;
	  end
	else
	  if (ok_to_assert_trap)
	    begin
	       if (assert_trap_odd)
		    trap_odd <= 1;
	
	       if (assert_trap_bus)
		    trap_bus <= 1;

	       if (assert_trap_ill)
		 begin
		    trap_ill <= 1;
		 end

               if (istate == e1)
		 begin
		    if (assert_trap_priv)
                      trap_priv <= 1;

		    if (assert_bpt)
                      trap_bpt <= 1;

		    if (assert_iot)
                      trap_iot <= 1;

		    if (assert_trap_emt)
                      trap_emt <= 1;

		    if (assert_trap_trap)
                      trap_trap <= 1;
		 end // if (istate == e1)
	    end // if (ok_to_assert_trap)

        trap <=
            trap_bpt || trap_iot || trap_emt || trap_trap ||
            trap_ill || trap_odd || trap_priv || trap_bus;

        if (trap)
	  begin
            $display("trap: asserts ");
	     $display(" %o %o %o %o %o %o %o %o",
		      trap_bpt, trap_iot, trap_emt, trap_trap,
		      trap_ill, trap_odd, trap_priv, trap_bus);

            if (assert_trap_priv) $display("PRIV ");
            if (assert_trap_odd) $display("ODD ");
            if (assert_trap_ill) $display("ILL ");
            if (assert_bpt) $display("BPT ");
            if (assert_iot) $display("IOT ");
            if (assert_trap_emt) $display("EMT ");
            if (assert_trap_trap) $display("TRAP ");
            if (assert_trap_bus) $display("BUS ");
            $display("");

            $display("trap: %d", trap);
	  end // if (trap)
     end // always @ (posedge clk)

   //
   // halt & wait entry
   //
   always @(posedge clk)
     if (reset_n == 0)
       begin
	  halted <= 0;
	  waited <= 0;
       end
     else
       if (istate == e1)
	 begin
	    if (assert_halt) begin
	       $display("assert_halt");
	       halted <= 1;
	    end

	    if (assert_wait) begin
	       $display("assert_wait");
	       waited <= 1;
	    end
	 end

   //
   // check_for_interrupts
   //
   wire ok_to_assert_int;

   assign ok_to_assert_int = istate == f1 || istate == i1 ||
			     istate == s4 || istate == d4 || istate == w1;

   wire ipl_below;

   ipl_below_func ibf(ipl, assert_int_ipl, ipl_below);
   
   always @(posedge clk)
     if (reset_n == 0)
       begin
	  interrupt <= 0;
       end
     else
       if (ok_to_assert_int)
	 begin
          if (assert_int & ipl_below)
	    begin
               interrupt <= 1;
               interrupt_vector <= assert_int_vec;
               $display("interrupt: asserts; vector %o", interrupt_vector);
            end
	  else
            interrupt <= 0;
       end
     else
       if (istate == t4)
	 interrupt <= 0;
   

   //
   // calculate next state
   //
   assign new_istate = istate == f1 ? ((trap || interrupt) ? t1 :
        			       c1) :

		       istate == c1 ? (is_illegal ? f1 :
        			       no_operand ? e1 :
        			       need_s1 ? s1 :
        			       need_d1 ? d1 :
				       e1):

		       istate == s1 ? (need_s2 ? s2 :
				       need_s4 ? s4 :
				       d1) :
		       istate == s2 ? (ss_ea_ind ? s3 :
        			       need_s4 ? s4 :
				       d1) :
		       istate == s3 ? s4 :
		       istate == s4 ? d1 :

		       istate == d1 ? (need_d2 ? d2 : 
        			       need_d4 ? d4 :
        			       need_push_state ? p1 :
        			       e1) :
		       istate == d2 ? (dd_ea_ind ? d3 :
        			       need_d4 ? d4 :
        			       need_push_state ? p1 :
        			       e1) :
		       istate == d3 ? (need_d4 ? d4 :
        			       need_push_state ? p1 :
        			       e1) :
		       istate == d4 ? (need_push_state ? p1 :
        			       e1) :

		       istate == e1 ? (need_pop_reg ? o3 :
				       need_pop_pc_psw ? o1 :
        			       w1) :

		       istate == w1 ? (halted ? h1 :
				       waited ? i1 :
				       f1) :

		       istate == o1 ? o2 :
		       istate == o2 ? f1 :
		       istate == o3 ? w1 :

		       istate == p1 ? e1 :

		       istate == t1 ? t2 :
		       istate == t2 ? t3 :
		       istate == t3 ? t4 :
		       istate == t4 ? f1 :

		       istate == i1 ? (interrupt ? f1 : i1) :

		       istate == h1 ? h1 :
		       istate;

  always @(posedge clk)
    if (reset_n == 0)
      istate <= f1;
   else
     istate <= bus_ack ? new_istate : istate;


   //
   // clock internal registes
   //
   always @(posedge clk)
     if (reset_n == 0)
       begin
	  ss_data <= 0;
	  dd_data <= 0;
	  ss_ea <= 0;
	  dd_ea <= 0;
	  e1_data <= 0;
       end
     else
       begin

	  ss_ea <= (istate == c1 ||
		    istate == s1 ||
		    istate == s2 ||
		    istate == s3 ||
		    istate == d1 ||
		    istate == d2 ||
		    istate == d3 ||
		    istate == t1 ||
		    istate == t3) ?
		   ss_ea_mux : ss_ea;

	  dd_ea <= (istate == c1 ||
		    istate == s1 ||
		    istate == s2 ||
		    istate == s3 ||
		    istate == d1 ||
		    istate == d2 ||
		    istate == d3) ?
		   dd_ea_mux : dd_ea;

	  if (istate == c1 ||
	      istate == s4 || istate == d4)
	    begin
	       ss_data <= ss_data_mux;
	       dd_data <= dd_data_mux;
	    end

	  e1_data <= (istate == e1 || istate == w1) ? e1_data_mux :
		     (istate == o3) ? bus_out :
		     e1_data;
       end

   //
   //debug
   //

`ifdef minimal_debug
   always @(posedge clk)
     #2 begin
   	case (istate)
	  f1:
	    begin
	       $display("f1: pc=%0o, sp=%0o, psw=%0o ipl%d n%d z%d v%d c%d",
			pc, sp, psw, ipl, cc_n, cc_z, cc_v, cc_c);
	       $display("    trap=%d, interrupt=%d", trap, interrupt);
	    end // case: f1
	endcase
     end
`endif
   
//`ifdef debug
   always @(posedge clk)
     #2 begin
   	case (istate)
	  f1:
	    begin
	       $display("f1: pc=%0o, sp=%0o, psw=%0o ipl%d n%d z%d v%d c%d",
			pc, sp, psw, ipl, cc_n, cc_z, cc_v, cc_c);
	       $display("    trap=%d, interrupt=%d", trap, interrupt);
	    end // case: f1

	  c1:
	    begin
	       $display("c1: isn %0o ss %d, dd %d, no_op %d, ill %d, push %d, pop %d",
			isn, need_srcspec_dd, need_destspec_dd,
			no_operand, is_illegal, need_push_state, need_pop_reg);

	       $display("    need_src_data %d, need_dest_data %d",
			need_src_data, need_dest_data);

	       $display(" ss: mode%d reg%d ind%d post %d pre %d",
			ss_mode, ss_reg, ss_ea_ind,
			ss_post_incr, ss_pre_dec);


	       $display(" dd: mode%d reg%d ea %0o ind%d post %d pre %d",
			dd_mode, dd_reg, dd_ea, dd_ea_ind,
			dd_post_incr, dd_pre_dec);

	       $display(" need: dest_data %d; s1 %d, s2 %d, s4 %d; d1 %d, d2 %d, d4 %d", 
			need_dest_data,
			need_s1, need_s2, need_s4, need_d1, need_d2, need_d4);


	    end

	  s1:
	    begin
	       $display("s1:");
	    end
	  
	  s2:
	    begin
	       $display("s2: ss_ea_mux %0o, [ea]=%0o", ss_ea_mux, bus_out);
	    end

	  s3:
	    begin
	       $display("s3: ss_ea_mux %0o", ss_ea_mux);
	    end

	  s4:
	    begin
	       $display("s4: ss_ea_mux %0o, ss_data_mux %o, bus_out %o",
			ss_ea_mux, ss_data_mux, bus_out);
	    end

	  d1:
	    begin
	       $display("d1: dd_ea %0o, dd_ea_mux %0o", dd_ea, dd_ea_mux);
	       $display("    ss_data %0o, ss_data_mux %0o",
			ss_data, ss_data_mux);
	       
               if (dd_post_incr)
		 begin
		    $display(" R%d <- %o (dd r++)", dd_reg, regs[dd_reg]);
		 end
               else
		 if (dd_pre_dec)
		   begin
		      $display(" R%d <- %o (dd r--)", dd_reg, regs[dd_reg]);
		   end
	    end // case: d1
	  
	  d2:
	    begin
	       $display("d2: dd_ea %0o, dd_ea_mux %0o", dd_ea, dd_ea_mux);
	    end

	  d3:
	    begin
	       $display("d3:");
	    end

	  d4:
	    begin
	       $display("d4:");
	    end

	  e1:
	    begin
	       $display("e1: isn %o, ss_data %o, dd_data %o",
			isn, ss_data, dd_data);
	       $display("    e1_data_mux %o, e1_data %o", e1_data_mux, e1_data);

	       $display("    ss_data %0o, dd_data %0o, e1_result %0o",
			ss_data, dd_data, e1_result);
	       $display("    latch_pc %d, latch_cc %d", latch_pc, latch_cc);
	       $display("    psw %o", psw);

	       $display("    e1_result %o, e1_data %o, e1_data_mux %o",
			e1_result, e1_data, e1_data_mux);

	    end

	  w1:
	    begin
	       $display("w1: dd%d %d, dd_data %o, ss%d %d, ss_data %o, e1_data %o",
		      dd_mode, dd_reg, dd_data, ss_mode, ss_reg, ss_data, e1_data);
	       $display("    store_result %d, store_ss_reg %d, store_32 %d",
			store_result, store_ss_reg, store_result32);
	       $display("    e1_data_mux %o, e1_data %o, e1_result %o",
			e1_data_mux, e1_data, e1_result);

	    end

	  o1:
	    begin
	       $display("o1:");
	    end

	  o2:
	    begin
	       $display("o2:");
	    end

	  o3:
	    begin
	       $display("o3:");
	    end


	  p1:
	    begin
	       $display("p1:");
	    end

	  t1:
	    begin
	       $display("t1: sp %o", sp);
	    end

	  t2:
	    begin
	       $display("t2:");
	    end

	  t3:
	    begin
	       $display("t3: ss_ea %o, ss_ea_mux %o", ss_ea, ss_ea_mux);
	    end

	  t4:
	    begin
	       $display("t4: ss_ea %o, ss_ea_mux %o", ss_ea, ss_ea_mux);
	    end

	  i1:
	    begin
	    end

	endcase // case(istate)

	$display("    bus_rd=%d, bus_wr=%d, bus_addr %o, bus_out %o",
		 bus_rd, bus_wr, bus_addr, bus_out);
	$display("    regs %0o %0o %0o %0o ",
		 regs[0], regs[1], regs[2], regs[3]);
	$display("         %0o %0o %0o %0o ",
		 regs[4], regs[5], regs[6], regs[7]);

	$display("    ss_ea_mux %0o, ss_ea %0o, dd_ea_mux %0o, dd_ea %0o",
		 ss_ea_mux, ss_ea, dd_ea_mux, dd_ea);
	$display("    ss_data %0o, dd_data %0o",
		 ss_data, dd_data);
	  
     end

//`endif
	    
endmodule



/*
* Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
