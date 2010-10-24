// execute.v
// PDP-11 in verilog - combinatorial logic for execute unit (e1 state)
// copyright Brad Parker <brad@heeltoe.com> 2009-2010

module execute(clk, reset, enable,
	       pc, psw,
	       ss_data, dd_data,
	       cc_n, cc_z, cc_v, cc_c,
	       current_mode,

	       dd_ea, ss_reg, ss_reg_value, ss_rego1_value,

	       isn, r5,

	       assert_halt, assert_wait, assert_trap_priv, assert_trap_emt, 
	       assert_trap_trap, assert_bpt, assert_iot, assert_reset,
	       assert_trace_inhibit,

	       e1_result, e32_result, e1_advance,

	       new_pc, latch_pc, latch_sp,
	       new_cc_n, new_cc_z, new_cc_v, new_cc_c,
	       latch_cc, latch_psw_prio, new_psw_prio);

   input clk;
   input reset;
   input enable;
   input [15:0] pc, psw;
   input [15:0] ss_data, dd_data;
   input 	cc_n, cc_z, cc_v, cc_c;
   input [1:0] 	current_mode;
   input [15:0] dd_ea;
   input [15:0] isn;
   input [15:0] r5;
   input [2:0] 	ss_reg;
   input [15:0]	ss_reg_value, ss_rego1_value;

   output 	assert_halt, assert_wait, assert_trap_priv, assert_trap_emt, 
		assert_trap_trap, assert_bpt, assert_iot, assert_reset;
   output 	assert_trace_inhibit;
 	
   output [15:0] e1_result /* verilator isolate_assignments*/;
   output [15:0] new_pc;
   output [15:0] e32_result /* verilator isolate_assignments*/;
   output 	 e1_advance;
 	 
   output 	 latch_pc;
   output 	 latch_sp;
   output 	 new_cc_n, new_cc_z, new_cc_v, new_cc_c;
   output 	 latch_cc, latch_psw_prio;
   output [2:0]  new_psw_prio;

   //
   wire [15:0] 	 div_result;
   wire [15:0] 	 div_remainder;
   wire [31:0] 	 mul_result;
   wire [31:0] 	 shift_result;

   reg 		assert_halt, assert_wait, assert_trap_priv, assert_trap_emt, 
		assert_trap_trap, assert_bpt, assert_iot, assert_reset;
   reg 		assert_trace_inhibit;

   reg [15:0] e1_result, new_pc;
   reg [15:0] e32_result;

   reg 	      latch_pc;
   reg 	      latch_sp;
   reg 	      new_cc_n, new_cc_z, new_cc_v, new_cc_c;
   reg 	      latch_cc, latch_psw_prio;
   reg [2:0]  new_psw_prio;

   reg [5:0]  shift;

   //
   wire       e32_result_sign, e32_result_zero;
   wire       e1_result_sign, e1_result_byte_sign;
   wire       e1_result_zero, e1_result_byte_zero;
   wire       dd_data_sign, dd_data_zero, ss_data_sign;
   
   assign e32_result_sign = e32_result[15];
   assign e32_result_zero = e1_result == 16'b0 && e32_result == 16'b0;

   assign e1_result_sign = e1_result[15];
   assign e1_result_byte_sign = e1_result[7];

   assign e1_result_zero = e1_result == 16'b0;
   assign e1_result_byte_zero = e1_result[7:0] == 8'b0;

   assign dd_data_sign = dd_data[15];
   assign dd_data_zero = dd_data == 16'b0;

   assign ss_data_sign = ss_data[15];

   //
   wire [7:0] pc_offset;
   wire [15:0] new_pc_w, new_pc_b, new_pc_sob;

   add8 add8_pc(pc_offset, isn[7:0], isn[7:0]);
   
   assign new_pc_w = pc + { 8'b0, pc_offset};
   assign new_pc_b = pc + { 8'hff, pc_offset };
   assign new_pc_sob = pc - {9'b0, isn[5:0], 1'b0};
   
   // cpu modes
   parameter 	mode_kernel = 2'b00;
   parameter 	mode_super  = 2'b01;
   parameter 	mode_undef  = 2'b10;
   parameter 	mode_user   = 2'b11;

   //
   wire   mul_done;
   wire   div_done;
   wire   shift_done;
   reg 	  mul_ready;
   reg 	  div_ready;
   reg 	  shift_ready;

   reg   div_abort;

   wire   shift_out;
   wire   sign_change16;
   wire   sign_change32;

   wire   div_overflow;
   wire   mul_overflow;
   
   wire   e1_advance;
   wire   is_isn_muldiv;

   mul1616 mul1616_box(.clk(clk), .reset(reset),
		       .ready(mul_ready),
		       .done(mul_done),
		       .multiplier(ss_data),
		       .multiplicand(dd_data),
		       .product(mul_result),
		       .overflow(mul_overflow));
   
   div3216 div3216_box(.clk(clk), .reset(reset),
		       .ready(div_ready),
		       .done(div_done),
		       .dividend({ss_reg_value, ss_rego1_value}),
		       .divider(dd_data),
		       .quotient(div_result),
   		       .remainder(div_remainder),
		       .overflow(div_overflow));

   wire [31:0] shift_in;
   wire shift_sign_change16, shift_sign_change32;

   shift32 shift32_box(.clk(clk), .reset(reset),
		       .ready(shift_ready),
		       .done(shift_done),
		       .in(shift_in),
		       .out(shift_result),
		       .shift(shift),
		       .last_bit(shift_out),
		       .sign_change16(shift_sign_change16),
		       .sign_change32(shift_sign_change32));

   wire   is_ash, is_ashc, is_ashx;
   
   assign is_ash = (isn[15:9] == 7'o072) ? 1'b1 : 1'b0;		/* ash */
   assign is_ashc = (isn[15:9] == 7'o073) ? 1'b1 : 1'b0;	/* ashc */

   assign is_ashx = is_ash | is_ashc;
   
   assign shift_in = is_ash ?
		     {ss_data[15] ? 16'hffff : 16'b0,ss_data} :	/* ash */
		     {ss_reg_value, ss_rego1_value};		/* ashc */
   
   assign is_isn_muldiv =
			 (isn[15:9] == 7'o070) ||		/* mul */
			 (isn[15:9] == 7'o071);			/* div */

   assign e1_advance = is_isn_muldiv ? (mul_done || div_done || div_abort) :
		       is_ashx ? (shift_done) : 1'b1;
   
   //
   always @(/*clk or*/enable or isn or pc or psw or ss_data or dd_data or
	    ss_reg or ss_reg_value or ss_rego1_value or
	    dd_ea or
	    cc_n or cc_z or cc_v or cc_c or isn or
	    e32_result or e32_result_sign or e32_result_zero or
	    e1_result or e1_result_sign or e1_result_zero or
	    e1_result_byte_sign or e1_result_byte_zero or
	    mul_result or div_result or div_remainder or
	    div_overflow or mul_overflow or
	    dd_data_sign or dd_data_zero or ss_data_sign or
	    pc_offset or new_pc_w or new_pc_b or new_pc_sob or
	    current_mode or r5 or
	    shift_result or shift_out or
	    shift_sign_change16 or shift_sign_change32
           )
     if (~enable) begin
	assert_halt = 0;
	assert_wait = 0;
	assert_trap_priv = 0;
	assert_trap_emt = 0;
	assert_trap_trap = 0;
	assert_bpt = 0;
	assert_iot = 0;
	assert_reset = 0;
	assert_trace_inhibit = 0;

	e1_result = 0;
	e32_result = 0;
	new_pc = 0;
	latch_pc = 0;
	latch_sp = 0;
	new_cc_n = 0;
	new_cc_z = 0;
	new_cc_v = 0;
	new_cc_c = 0;
	latch_cc = 0;
	latch_psw_prio = 0;
	new_psw_prio = 0;
	mul_ready = 0;
	div_ready = 0;
	div_abort = 0;
	shift_ready = 0;
	shift = 0;
     end
     else
       begin
	assert_halt = 0;
	assert_wait = 0;
	assert_trap_priv = 0;
	assert_trap_emt = 0;
	assert_trap_trap = 0;
	assert_bpt = 0;
	assert_iot = 0;
	assert_reset = 0;
	assert_trace_inhibit = 0;

	e1_result = 0;
	e32_result = 0;
	  
	new_pc = 0;
	latch_pc = 0;
	latch_sp = 0;

	new_cc_n = cc_n;
	new_cc_z = cc_z;
	new_cc_v = cc_v;
	new_cc_c = cc_c;
	latch_cc = 0;
	latch_psw_prio = 0;
	new_psw_prio = 0;

	mul_ready = 0;
	div_ready = 0;
	div_abort = 0;
	shift_ready = 0;

	shift = 0;
	
	if (isn[15:12] == 0)
	  begin
	     if (isn[11:6] == 6'o00 && isn[5:0] < 6'o10)
	       begin
`ifdef debug
		  if (0) $display("e: MS");
`endif
		  case (isn[2:0])
		    0:					    /* halt */
		      begin
`ifdef debug
			 $display("e: HALT");
`endif
//xxx fix me
//			 if (current_mode == mode_kernel)
			   assert_halt = 1;
//			 else
//			   assert_trap_priv = 1;
		      end
		    
		    1:					    /* wait */
		      begin
`ifdef debug
			 $display("e: WAIT");
`endif
//			 assert_wait = 1;
		      end

		    3:					    /* bpt */
		      begin
			 assert_bpt = 1;
		      end

		    4:					    /* iot */
		      begin
			 assert_iot = 1;
			 assert_trace_inhibit = 1;
		      end

		    5:					    /* reset */
		      begin
			 if (current_mode == mode_kernel)
			   assert_reset = 1;

			 // apparently reset does not change psw
			 //new_cc_n = 0;
			 //new_cc_v = 0;
			 //new_cc_c = 0;
			 //new_cc_z = 0;
			 //latch_cc = 1;
		      end

		    2:					    /* rti */
		      begin
			 if (0) $display("e: RTI");
		      end

		    6:					    /* rtt */
		      begin
			 //if (0) $display("e: RTT");
			 assert_trace_inhibit = 1;
		      end

		    7:					    /* mfpt */
		      e1_result = 16'h1234;
		  endcase // case(isn[2:0])
	       end // if (isn[11:6] == 000 && isn[5:0] < 010)
	     else
	       begin
		  if (0) $display("e: pc & cc %o", isn[11:6]);

		  case (isn[11:6])

		    6'o01:					    /* jmp */
		      begin
			 if (0) $display("e: JMP; dest_ea %o", dd_ea);
			 new_pc = dd_ea;
			 // don't latch if illegal jmp rx
			 latch_pc = isn[5:3] != 3'b000;
		      end

		    6'o02:					    /* rts */
		      case (isn[5:0])
			6'o00, 6'o01, 6'o02, 6'o03,
			  6'o04, 6'o05, 6'o06, 6'o07:
			    begin
			       if (0) $display("e: RTS");
			       new_pc = dd_data;
			       latch_pc = 1;
			    end

			6'o30:				    	    /* spl */
			  begin
			     new_psw_prio = isn[2:0];
			     latch_psw_prio = 1;
			  end

			// ccc	cc	-	000257
			// cln	cc	-	000250
			// clz	cc	-	000244
			// clv	cc	-	000242
			// clc	cc	-	000241
			// c	cc	-	000240
			6'o40, 6'o41, 6'o42, 6'o43,
			  6'o44, 6'o45, 6'o46, 6'o47,
			  6'o50, 6'o51, 6'o52, 6'o53,
			  6'o54, 6'o55, 6'o56, 6'o57:
			    begin
			       if (isn[3]) new_cc_n = 0;
			       if (isn[2]) new_cc_z = 0;
			       if (isn[1]) new_cc_v = 0;
			       if (isn[0]) new_cc_c = 0;
			       latch_cc = 1;
			    end

			// sen	cc	-	000270
			// sez	cc	-	000264
			// sev	cc	-	000262
			// sec	cc	-	000261
			// s	cc	-	000260
			6'o60, 6'o61, 6'o62, 6'o63,
			  6'o64, 6'o65, 6'o66, 6'o67,
			  6'o70, 6'o71, 6'o72, 6'o73,
			  6'o74, 6'o75, 6'o76, 6'o77:
			    begin
			       if (isn[3]) new_cc_n = 1;
			       if (isn[2]) new_cc_z = 1;
			       if (isn[1]) new_cc_v = 1;
			       if (isn[0]) new_cc_c = 1;
			       latch_cc = 1;
			    end

			default:
			  begin
			  end
			
		      endcase // case(isn[5:0])

		    6'o03:					    /* swab */
		      begin
			 if (0) $display("e: SWAB");
			 e1_result = {dd_data[7:0],dd_data[15:8]};
			 new_cc_n = e1_result_byte_sign;
			 new_cc_z = e1_result_byte_zero;
			 new_cc_v = 0;
			 new_cc_c = 0;
			 latch_cc = 1;
		      end
		    
		    6'o04, 6'o05:				    /* br */
		      begin
			 new_pc = new_pc_w;
			 latch_pc = 1;
			 if (0) $display("e: br; isn %o, pc %o, new_pc %o",
					 isn, pc, new_pc);
		      end

		    6'o06, 6'o07:				    /* br */
		      begin
			 new_pc = new_pc_b;
			 latch_pc = 1;
		      end

		    6'o10, 6'o11:				    /* bne */
		      begin
			 new_pc = new_pc_w;
			 latch_pc = ~cc_z;
		      end

		    6'o12, 6'o13:				    /* bne */
		      begin
			 new_pc = new_pc_b;
			 latch_pc = ~cc_z;
		      end

		    6'o14, 6'o15:				    /* beq */
		      begin
			 new_pc = new_pc_w;
			 latch_pc = cc_z;
		      end

		    6'o16, 6'o17:				    /* beq */
		      begin
			 new_pc = new_pc_b;
			 latch_pc = cc_z;
		      end

		    6'o20, 6'o21:				    /* bge */
		      begin
			 new_pc = new_pc_w;
			 latch_pc = (cc_n ^ cc_v) ? 1'b0 : 1'b1;
`ifdef debug
			 if (0) $display("e: bge; latch_pc %o pc %o new_pc %o",
					 latch_pc, pc, new_pc);
`endif
		      end

		    6'o22, 6'o23:				    /* bge */
		      begin
			 new_pc = new_pc_b;
			 latch_pc = (cc_n ^ cc_v) ? 1'b0 : 1'b1;
`ifdef debug
			 if (0) $display("e: bge; isn %o, latch_pc %o pc %o new_pc %o",
					 isn, latch_pc, pc, new_pc);
`endif
		      end

		    6'o24, 6'o25:				    /* blt */
		      begin
			 new_pc = new_pc_w;
			 latch_pc = (cc_n ^ cc_v);
		      end

		    6'o26, 6'o27:				    /* blt */
		      begin
			 new_pc = new_pc_b;
			 latch_pc = (cc_n ^ cc_v);
		      end

		    6'o30, 6'o31:				    /* bgt */
		      begin
			 new_pc = new_pc_w;
			 latch_pc = (cc_z || cc_n ^ cc_v) ? 1'b0 : 1'b1;
		      end

		    6'o32, 6'o33:				    /* bgt */
		      begin
			 new_pc = new_pc_b;
			 latch_pc = (cc_z || cc_n ^ cc_v) ? 1'b0 : 1'b1;
		      end

		    6'o34, 6'o35:				    /* ble */
		      begin
			 new_pc = new_pc_w;
			 latch_pc = (cc_z || cc_n ^ cc_v);
		      end

		    6'o36, 6'o37:				    /* ble */
		      begin
			 new_pc = new_pc_b;
			 latch_pc = (cc_z || cc_n ^ cc_v);
		      end

		    6'o40, 6'o41, 6'o42, 6'o43,	    /* jsr */
		      6'o44, 6'o45, 6'o46, 6'o47:
			begin
			   if (0) $display("e: JSR r%d; dd_data %o, dd_ea %o",
					   ss_reg, dd_data, dd_ea);
			   e1_result = pc;
			   new_pc = dd_ea;
			   // don't latch if illegal jsr rx
			   latch_pc = isn[5:3] != 3'b000;
			end

		    6'o50:					    /* clr */
		      begin
			 new_cc_n = 0;
			 new_cc_v = 0;
			 new_cc_c = 0;
			 new_cc_z = 1;
			 latch_cc = 1;
			 e1_result = 0;
		      end

		    6'o51:					    /* com */
		      begin
			 e1_result = ~dd_data;
			 new_cc_n = e1_result_sign;
			 new_cc_z = e1_result_zero;
			 new_cc_v = 0;
			 new_cc_c = 1;
			 latch_cc = 1;
		      end

		    6'o52:					    /* inc */
		      begin
			 e1_result = dd_data + 16'd1;
			 new_cc_n = e1_result_sign;
			 new_cc_z = e1_result_zero;
			 new_cc_v = (e1_result == 16'o100000);
			 latch_cc = 1;
		      end

		    6'o53:					    /* dec */
		      begin
			 e1_result = dd_data - 16'd1;
			 new_cc_n = e1_result_sign;
			 new_cc_z = e1_result_zero;
			 new_cc_v = (e1_result == 16'o77777);
			 latch_cc = 1;
		      end

		    6'o54:					    /* neg */
		      begin
			 e1_result = -dd_data;
			 new_cc_n = e1_result_sign;
			 new_cc_z = e1_result_zero;
			 new_cc_v = (e1_result == 16'o100000);
			 new_cc_c = new_cc_z ^ 1'b1;
			 latch_cc = 1;
		      end

		    6'o55:					    /* adc */
		      begin
			 e1_result = dd_data + {15'b0, cc_c};
			 new_cc_n = e1_result_sign;
			 new_cc_z = e1_result_zero;
			 new_cc_v = (cc_c && (e1_result == 16'o100000));
			 new_cc_c = cc_c & new_cc_z;
			 latch_cc = 1;
		      end

		    6'o56:					    /* sbc */
		      begin
			 e1_result = dd_data - {15'b0, cc_c};
			 new_cc_n = e1_result_sign;
			 new_cc_z = e1_result_zero;
			 new_cc_v = (cc_c && (e1_result == 16'o77777));
			 new_cc_c = (cc_c && (e1_result == 16'o177777));
			 latch_cc = 1;
		      end

		    6'o57:					    /* tst */
		      begin
			 e1_result = dd_data;
			 new_cc_n = e1_result_sign;
			 new_cc_z = e1_result_zero;
			 new_cc_v = 0;
			 new_cc_c = 0;
			 latch_cc = 1;
		      end

		    6'o60:					    /* ror */
		      begin
			 e1_result = {cc_c, dd_data[15:1]};
			 new_cc_n = e1_result_sign;
			 new_cc_z = e1_result_zero;
			 new_cc_c = dd_data[0];
			 new_cc_v = new_cc_n ^ new_cc_c;
			 latch_cc = 1;
		      end

		    6'o61:					    /* rol */
		      begin
			 e1_result = {dd_data[14:0], cc_c};
			 new_cc_n = e1_result_sign;
			 new_cc_z = e1_result_zero;
			 new_cc_c = dd_data_sign;
			 new_cc_v = new_cc_n ^ new_cc_c;
			 latch_cc = 1;
		      end

		    6'o62:					    /* asr */
		      begin
			 e1_result = {dd_data[15], dd_data[15:1]};
			 new_cc_n = e1_result_sign;
			 new_cc_z = e1_result_zero;
			 new_cc_c = dd_data[0];
			 new_cc_v = new_cc_n ^ new_cc_c;
			 latch_cc = 1;
		      end

		    6'o63:					    /* asl */
		      begin
			 e1_result = {dd_data[14:0], 1'b0};
			 new_cc_n = e1_result_sign;
			 new_cc_z = e1_result_zero;
			 new_cc_c = dd_data[15];
			 new_cc_v = new_cc_n ^ new_cc_c;
			 latch_cc = 1;
		      end

		    6'o64:					    /* mark */
		      begin
			 e1_result = new_pc_w;
			 latch_sp = 1;
			 
			 new_pc = r5;
			 latch_pc = 1;
		      end
		    
		    6'o65:					    /* mfpi */
		      begin
`ifdef debug
			 $display("e: MFPI %o", dd_data);
`endif
			 new_cc_n = dd_data_sign;
			 new_cc_z = dd_data_zero;
			 new_cc_v = 0;
			 latch_cc = 1;
		      end
		    
		    6'o66:					    /* mtpi */
		      begin
`ifdef debug
			 $display("e: MTPI %o", dd_data);
`endif
			 new_cc_n = dd_data_sign;
			 new_cc_z = dd_data_zero;
			 new_cc_v = 0;
			 latch_cc = 1;
		      end
		    
		    6'o67:					    /* sxt */
		      begin
			 e1_result = cc_n ? 16'hffff : 16'b0;
			 new_cc_z = cc_n ^ 1'b1;
			 new_cc_v = 0;
			 latch_cc = 1;
		      end
		    
		    6'o70,					    /* csm */
		      6'o72,					    /* tstset */
		      6'o73:					    /* wrtlck */
			begin
			end

		    default:
		      begin
		      end

		  endcase // case(isn[11:6])
	       end // else: !if(isn[11:6] == 6'o00 && isn[5:0] < 010)
	  end // if (isn[15:12] == 0)
	else
	  begin
	     if (0) $display("e: isn[15:12] != 0 (%o)", isn[15:12]);
	     case (isn[15:12])
	       4'o00:
		 begin
		 end

	       4'o01:					    /* mov */
		 begin
		    e1_result = ss_data;
		    new_cc_n = e1_result_sign;
		    new_cc_z = e1_result_zero;
		    new_cc_v = 0;
		    latch_cc = 1;
		    if (0) $display("e: mov ss_data %o, e1_result %o",
				    ss_data, e1_result);
		 end

	       4'o02:					    /* cmp */
		 begin
		    //$display("e: CMP %o %o", ss_data, dd_data);
		    e1_result = ss_data - dd_data;
		    new_cc_n = e1_result_sign;
		    new_cc_z = e1_result_zero;
		    new_cc_v = (ss_data[15] ^ dd_data[15]) &
			       (~dd_data[15] ^ e1_result[15]);
		    new_cc_c = ss_data < dd_data;
		    latch_cc = 1;
		 end

	       4'o03:					    /* bit */
		 begin
		    e1_result = ss_data & dd_data;
		    new_cc_n = e1_result_sign;
		    new_cc_z = e1_result_zero;
		    new_cc_v = 0;
		    latch_cc = 1;
		 end

	       4'o04:					    /* bic */
		 begin
		    e1_result = ~ss_data & dd_data;
		    new_cc_n = e1_result_sign;
		    new_cc_z = e1_result_zero;
		    new_cc_v = 0;
		    latch_cc = 1;
		 end

	       4'o05:					    /* bis */
		 begin
		    e1_result = ss_data | dd_data;
		    new_cc_n = e1_result_sign;
		    new_cc_z = e1_result_zero;
		    new_cc_v = 0;
		    latch_cc = 1;
		 end

	       4'o06:					    /* add */
		 begin
		    e1_result = ss_data + dd_data;
		    new_cc_n = e1_result_sign;
		    new_cc_z = e1_result_zero;
		    new_cc_v = (~ss_data[15] ^ dd_data[15]) &
			       (ss_data[15] ^ e1_result[15]);
		    new_cc_c = (e1_result < ss_data);
		    latch_cc = 1;
		    if (0) $display("e: add, new_cc_z %o", new_cc_z);
		 end

	       04'o7:
		 case (isn[11:9]) 

		   0:					    /* mul */
		     begin
			if (0) $display("e: MUL %o %o %o",
					ss_data, dd_data, mul_result);

			mul_ready = 1;
			e32_result = mul_result[31:16];
			e1_result = mul_result[15:0];

			new_cc_n = e32_result_sign;
			new_cc_z = e32_result_zero;
			new_cc_v = 0;
			new_cc_c = mul_overflow;
			latch_cc = 1;
		     end

		   1:					    /* div */
		     begin
			if (dd_data == 0)
			  begin
			     e32_result = ss_reg_value;
			     e1_result = ss_rego1_value;
			     new_cc_n = 0;
			     new_cc_z = 1;
			     new_cc_v = 1;
			     new_cc_c = 1;
			     latch_cc = 1;
			     div_abort = 1;
			  end
			else if ((ss_reg_value == 16'h8000 &&
				  ss_rego1_value == 0) &&
				 (dd_data == 16'o177777))
			  begin
			     e32_result = ss_reg_value;
			     e1_result = ss_rego1_value;
			     new_cc_v = 1;
			     new_cc_z = 1;
			     new_cc_n = 1;
			     new_cc_c = 1;
			     latch_cc = 1;
			     div_abort = 1;
			  end
			else begin
			   div_ready = 1;
			   e32_result = div_result;
			   e1_result = div_remainder;

			   if (div_overflow)
			     begin
				e32_result = ss_reg_value;
				e1_result = ss_rego1_value;
				new_cc_n = e32_result_sign;
				new_cc_z = 0;
				new_cc_v = 1;
				new_cc_c = 0;
			     end else begin
				new_cc_n = e32_result_sign;
				new_cc_z = e32_result == 16'b0;
				new_cc_v = 0;
				new_cc_c = 0;
			     end

			   latch_cc = 1;
			end
		     end // case: 1
		   

		   2:					    /* ash */
		     begin
			shift = dd_data[5:0];
`ifdef debug_xx
			$display("e: ASH %o; done %b", shift, shift32_box.done);
`endif
			
			shift_ready = 1;
			e1_result = shift_result[15:0];

			new_cc_n = e1_result_sign;
			new_cc_z = e1_result_zero;

			new_cc_v = shift == 0 ? 1'b0 : shift_sign_change16;

			new_cc_c = shift[5] ? shift_out : shift_result[16];
			latch_cc = 1;
		     end // case: 2
		   
		   
		   3:					    /* ashc */
		     begin
			shift = dd_data[5:0];

			shift_ready = 1;
			e32_result = shift_result[31:16];
			e1_result = shift_result[15:0];

			new_cc_n = e32_result_sign;
			new_cc_z = e32_result_zero;

			new_cc_v = shift == 0 ? 1'b0 : shift_sign_change32;

			new_cc_c = shift_out;
			latch_cc = 1;
		     end

		   4:					    /* xor */
		     begin
			e1_result = ss_data ^ dd_data;
			new_cc_n = e1_result_sign;
			new_cc_z = e1_result_zero;
			new_cc_v = 0;
			latch_cc = 1;
		     end
		   
		   5:					    /* fis */
		     begin
		     end
		   
		   6:					    /* cis */
		     begin
		     end

		   7:					    /* sob */
		     begin
			e1_result = ss_data - 16'd1;
			new_pc = new_pc_sob;
			latch_pc = ~e1_result_zero;
		     end
		 endcase // case(isn[11:9])

	       4'o10:
		 begin
		    if (0) $display("e: 010 isn[11:6] %o", isn[11:6]);
		 case (isn[11:6])
		   6'o00, 6'o01:				/* bpl */
		     begin
			//if (0) $display("e: BPL"); 
			new_pc = new_pc_w;
			latch_pc = ~cc_n;
		     end

		   6'o02, 6'o03:				/* bpl */
		     begin
			//if (0) $display("e: BPLB");
			new_pc = new_pc_b;
			latch_pc = ~cc_n;
		     end

		   6'o04, 6'o05:				/* bmi */
		     begin
			new_pc = new_pc_w;
			latch_pc = cc_n;
		     end

		   6'o06, 6'o07:				/* bmi */
		     begin
			new_pc = new_pc_b;
			latch_pc = cc_n;
		     end

		   6'o10, 6'o11:				/* bhi */
		     begin
			new_pc = new_pc_w;
			latch_pc = ~(cc_c | cc_z);
		     end

		   6'o12, 6'o13:				/* bhi */
		     begin
			new_pc = new_pc_b;
			latch_pc = ~(cc_c | cc_z);
		     end

		   6'o14, 6'o15:				/* blos */
		     begin
			new_pc = new_pc_w;
			latch_pc = (cc_c | cc_z);
		     end

		   6'o16, 6'o17:				/* blos */
		     begin
			new_pc = new_pc_b;
			latch_pc = (cc_c | cc_z);
		     end

		   6'o20, 6'o21:				/* bvc */
		     begin
			new_pc = new_pc_w;
			latch_pc = ~cc_v;
		     end

		   6'o22, 6'o23:				/* bvc */
		     begin
			new_pc = new_pc_b;
			latch_pc = ~cc_v;
		     end

		   6'o24, 6'o25:				/* bvs */
		     begin
			new_pc = new_pc_w;
			latch_pc = cc_v;
		     end

		   6'o26, 6'o27:				/* bvs */
		     begin
			new_pc = new_pc_b;
			latch_pc = cc_v;
		     end

		   6'o30, 6'o31:				/* bcc */
		     begin
			new_pc = new_pc_w;
			latch_pc = ~cc_c;
		     end

		   6'o32, 6'o33:				/* bcc */
		     begin
			new_pc = new_pc_b;
			latch_pc = ~cc_c;
		     end

		   6'o34, 6'o35:				/* bcs */
		     begin
			new_pc = new_pc_w;
			latch_pc = cc_c;
		     end

		   6'o36, 6'o37:				/* bcs */
		     begin
			new_pc = new_pc_b;
			latch_pc = cc_c;
		     end

		   6'o40, 6'o41, 6'o42, 6'o43:	/* emt */
		     begin
`ifdef debug
			$display("e: EMT");
`endif
			assert_trap_emt = 1;
		     end

		   6'o44, 6'o45, 6'o46, 6'o47:	/* trap */
		     begin
`ifdef debug
			$display("e: TRAP");
`endif
			assert_trap_trap = 1;
		     end

		   6'o50:				/* clrb */
		     begin
			e1_result = {dd_data[15:8], 8'b0};
			new_cc_n = 0;
			new_cc_v = 0;
			new_cc_c = 0;
			new_cc_z = 1;
			latch_cc = 1;
		     end

		   6'o51:				/* comb */
		     begin
			e1_result = {8'b0, ~dd_data[7:0]};
			new_cc_n = e1_result_byte_sign;
			new_cc_z = e1_result_byte_zero;
			new_cc_v = 0;
			new_cc_c = 1;
			latch_cc = 1;
			//note: byte write of src - rmw to memory word
		     end

		   6'o52:				/* incb */
		     begin
			e1_result = {dd_data[15:8], (dd_data[7:0] + 8'd1)};
			new_cc_n = e1_result_byte_sign;
			new_cc_z = e1_result_byte_zero;
			new_cc_v = 0;
			latch_cc = 1;
			//note: byte write of src - rmw to memory word
		     end

		   6'o53:				/* decb */
		     begin
			e1_result = {dd_data[15:8], (dd_data[7:0] - 8'd1)};
			new_cc_n = e1_result_byte_sign;
			new_cc_z = e1_result_byte_zero;
			new_cc_v = e1_result[7:0] == 8'o177;
			latch_cc = 1;
			//note: byte write of src - rmw to memory word
		     end

		   6'o54:				/* negb */
		     begin
			e1_result = {dd_data[15:8], -dd_data[7:0]};
			new_cc_n = e1_result_byte_sign;
			new_cc_z = e1_result_byte_zero;
			new_cc_v = e1_result[7:0] == 8'o200;
			new_cc_c = new_cc_z ^ 1'b1;
			latch_cc = 1;
			//note: byte write of src - rmw to memory word
		     end

		   6'o55:				/* adcb */
		     begin
			e1_result = {
				     dd_data[15:8],
				     dd_data[7:0] + {7'b0, cc_c}
				     };
			new_cc_n = e1_result_byte_sign;
			new_cc_z = e1_result_byte_zero;
			new_cc_v = cc_c && (e1_result[7:0] == 8'o200);
			new_cc_c = cc_c & new_cc_z;
			latch_cc = 1;
			//note: byte write of src - rmw to memory word
		     end

		   6'o56:				/* sbcb */
		     begin
			e1_result = {
				     dd_data[15:8],
				     dd_data[7:0] - {7'b0, cc_c}
				     };
			new_cc_n = e1_result_byte_sign;
			new_cc_z = e1_result_byte_zero;
			new_cc_v = cc_c && (e1_result[7:0] == 8'o177);
			new_cc_c = cc_c && (e1_result[7:0] == 8'o377);
			latch_cc = 1;
			//note: byte write of src - rmw to memory word
		     end

		   6'o57:				/* tstb */
		     begin
			e1_result = {8'b0, dd_data[7:0]};
			new_cc_n = e1_result_byte_sign;
			new_cc_z = e1_result_byte_zero;
			new_cc_v = 0;
			new_cc_c = 0;
			latch_cc = 1;
		     end

		   6'o60:				/* rorb */
		     begin
			e1_result = {dd_data[15:8], cc_c, dd_data[7:1]};
			new_cc_n = e1_result_byte_sign;
			new_cc_z = e1_result_byte_zero;
			new_cc_c = dd_data[0];
			new_cc_v = new_cc_n ^ new_cc_c;
			latch_cc = 1;
			//note: byte write of src - rmw to memory word
		     end

		   6'o61:				/* rolb */
		     begin
`ifdef debug
			if (0) $display("e: ROLB %o", dd_data);
`endif
			e1_result = {dd_data[15:8], dd_data[6:0], cc_c};
			new_cc_n = e1_result_byte_sign;
			new_cc_z = e1_result_byte_zero;
			new_cc_c = dd_data[7];
			new_cc_v = new_cc_n ^ new_cc_c;
			latch_cc = 1;
			//note: byte write of src - rmw to memory word
		     end

		   6'o62:				/* asrb */
		     begin
			e1_result = {dd_data[15:8], dd_data[7], dd_data[7:1]};
			new_cc_n = e1_result_byte_sign;
			new_cc_z = e1_result_byte_zero;
			new_cc_c = dd_data[0];
			new_cc_v = new_cc_n ^ new_cc_c;
			latch_cc = 1;
			//note: byte write of src - rmw to memory word
		     end

		   6'o63:				/* aslb */
		     begin
			e1_result = {dd_data[15:8], dd_data[6:0], 1'b0};
			new_cc_n = e1_result_byte_sign;
			new_cc_z = e1_result_byte_zero;
			new_cc_c = dd_data[7];
			new_cc_v = new_cc_n ^ new_cc_c;
			latch_cc = 1;
			//note: byte write of src - rmw to memory word
		     end

		   6'o64:				/* mtps */
		     begin
//			if (current_mode == mode_kernel)
//			  begin
//			  end

			new_psw_prio = dd_data[7:5];
			latch_psw_prio = 1;

			new_cc_n = dd_data[3];
			new_cc_z = dd_data[2];
			new_cc_v = dd_data[1];
			new_cc_c = dd_data[0];
			latch_cc = 1;
		     end

		   6'o65:				/* mfpd */
		     begin
`ifdef debug
			$display("e: MFPD %o", dd_data);
`endif
			new_cc_n = dd_data_sign;
			new_cc_z = dd_data_zero;
			new_cc_v = 0;
			latch_cc = 1;
		     end

		   6'o66:				/* mtpd */
		     begin
`ifdef debug
			$display("e: MTPD %o", dd_data);
`endif
			new_cc_n = dd_data_sign;
			new_cc_z = dd_data_zero;
			new_cc_v = 0;
			latch_cc = 1;
			// pop data
		     end

		   6'o67:				/* mfps */
		     begin
			new_cc_n = e1_result_sign;
			new_cc_z = e1_result_zero;
			new_cc_v = 0;
			latch_cc = 1;
			e1_result = psw[7] ? (16'o177400 | psw) : psw;
		     end

		   default:
		     begin
		     end
		 endcase // case(isn[11:6])
		 end

	       4'o11:					    /* movb */
		 begin
		    e1_result = {ss_data[7] ? 8'hff : 8'b0, ss_data[7:0]};
		    new_cc_n = e1_result_byte_sign;
		    new_cc_z = e1_result_byte_zero;
		    new_cc_v = 0;
		    latch_cc = 1;
		 end

	       4'o12:					    /* cmpb */
		 begin
		    e1_result = {8'b0, ss_data[7:0] - dd_data [7:0]};
		    new_cc_n = e1_result_byte_sign;
		    new_cc_z = e1_result_byte_zero;
		    new_cc_v = (ss_data[7] ^ dd_data[7]) &
			       (~dd_data[7] ^ e1_result[7]);
		    new_cc_c = ss_data[7:0] < dd_data[7:0];
		    latch_cc = 1;
		 end

	       4'o13:					    /* bitb */
		 begin
		    e1_result = {dd_data[15:8], ss_data[7:0] & dd_data[7:0]};
		    new_cc_n = e1_result_byte_sign;
		    new_cc_z = e1_result_zero;
		    new_cc_v = 0;
		    latch_cc = 1;
		 end

	       4'o14:					    /* bicb */
		 begin
		    e1_result = {dd_data[15:8], ~ss_data[7:0] & dd_data[7:0]};
		    new_cc_n = e1_result_byte_sign;
		    new_cc_z = e1_result_zero;
		    new_cc_v = 0;
		    latch_cc = 1;
		 end

	       4'o15:					    /* bisb */
		 begin
		    e1_result = {dd_data[15:8], ss_data[7:0] | dd_data[7:0]};
		    new_cc_n = e1_result_byte_sign;
		    new_cc_z = e1_result_zero;
		    new_cc_v = 0;
		    latch_cc = 1;
		 end

	       4'o16:					    /* sub */
		 begin
		    if (0) $display("e: SUB %o %o", ss_data, dd_data);
		    e1_result = dd_data - ss_data;
		    new_cc_n = e1_result_sign;
		    new_cc_z = e1_result_zero;
		    new_cc_v = ((ss_data[15] ^ dd_data[15]) &
				(~ss_data[15] ^ e1_result[15]));
		    new_cc_c = dd_data < ss_data;
		    latch_cc = 1;
		 end // case: 016

	       4'o17:
		 begin
		 end
	     endcase // case(isn[15:12])
	  end // else: !if(isn[15:12] == 0)

     end // always @*

endmodule // execute
