//
// execute.v
//
// execute state for pdp-11
//

module execute(pc, psw,
	       ss_data, dd_data,
	       cc_n, cc_z, cc_v, cc_c,
	       current_mode,

	       dd_ea, ss_reg, ss_reg_value, ss_rego1_value,

	       isn,
	       isn_15_12, isn_11_9, isn_11_6, isn_5_0, isn_3_0,

	       assert_halt, assert_wait, assert_trap_priv, assert_trap_emt, 
	       assert_trap_trap, assert_bpt, assert_iot, assert_reset, 

	       e1_result, e32_result,

	       new_pc, latch_pc, 
	       new_cc_n, new_cc_z, new_cc_v, new_cc_c,
	       latch_cc, latch_psw_prio, new_psw_prio);

   input [15:0] pc, psw;
   input [15:0] ss_data, dd_data;
   input 	cc_n, cc_z, cc_v, cc_c;
   input [2:0] 	current_mode;
   input [15:0] dd_ea;
   input [15:0] isn;
   input [3:0] 	isn_15_12;
   input [5:0] 	isn_11_6, isn_5_0;
   input [2:0] 	isn_11_9;
   input [3:0]  isn_3_0;
   input [2:0] 	ss_reg;
   input [15:0]	ss_reg_value, ss_rego1_value;

   output 	assert_halt, assert_wait, assert_trap_priv, assert_trap_emt, 
		assert_trap_trap, assert_bpt, assert_iot, assert_reset;

   output [15:0] e1_result, new_pc;
   output [31:0] e32_result;

   output 	 latch_pc;
   output 	 new_cc_n, new_cc_z, new_cc_v, new_cc_c;
   output 	 latch_cc, latch_psw_prio;
   output [2:0]  new_psw_prio;

   reg 		assert_halt, assert_wait, assert_trap_priv, assert_trap_emt, 
		assert_trap_trap, assert_bpt, assert_iot, assert_reset;

   reg [15:0] e1_result, new_pc;
   reg [31:0] e32_result;

   reg 	      latch_pc;
   reg 	      new_cc_n, new_cc_z, new_cc_v, new_cc_c;
   reg 	      latch_cc, latch_psw_prio;
   reg [2:0]  new_psw_prio;

   
   reg [15:0] temp, sign, shift;

   always @(ss_data, dd_data)
     begin
	assert_halt = 0;
	assert_wait = 0;
	assert_trap_priv = 0;
	assert_trap_emt = 0;
	assert_trap_trap = 0;
	assert_bpt = 0;
	assert_iot = 0;
	assert_reset = 0;

	e1_result = 0;

	new_pc = 0;
	latch_pc = 0;

	new_cc_n = cc_n;
	new_cc_z = cc_z;
	new_cc_v = cc_v;
	new_cc_c = cc_c;
	latch_cc = 0;
	latch_psw_prio = 0;

	
	if (isn_15_12 == 0)
	  begin
	     if (isn_11_6 == 000 && isn_5_0 < 010)
	       begin
		  $display("e: MS");
		  case (isn & 7)
		    0:					    /* halt */
		      begin
			 $display("e: HALT");
			 //		    if (current_mode == mode_kernel)
			 if (1)
			   assert_halt = 1;
			 else
			   assert_trap_priv = 1;
		      end
		    
		    1:					    /* wait */
		      begin
			 $display("e: WAIT");
			 assert_wait = 1;
		      end

		    3:					    /* bpt */
		      assert_bpt = 1;

		    4:					    /* iot */
		      assert_iot = 1;

		    5:					    /* reset */
		      //		 if (current_mode == mode_kernel)
		      assert_reset = 1;

		    2:					    /* rti */
		      $display("e: RTI");

		    6:					    /* rtt */
		      $display("e: RTT");

		    7:					    /* mfpt */
		      e1_result = 16'h1234;
		  endcase // case(isn & 7)
	       end // if (isn_11_6 == 000 && isn_5_0 < 010)
	     else
	       begin
		  $display("e: pc & cc %o", isn_11_6);

		  case (isn_11_6)

		    6'o01:					    /* jmp */
		      begin
			 $display("e: JMP; dest_ea %6o", dd_ea);
			 new_pc = dd_ea;
			 latch_pc = 1;
		      end

		    6'o02:					    /* rts */
		      case (isn_5_0)
			6'o00, 6'o01, 6'o02, 6'o03,
			  6'o04, 6'o05, 6'o06, 6'o07:
			    begin
			       $display("e: RTS");
			       new_pc = dd_data;
			       latch_pc = 1;
			    end

			6'o30:				    /* spl */
			  begin
			     new_psw_prio = isn & 7;
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
			       if (isn_3_0 & 4'b1000) new_cc_n = 0;
			       if (isn_3_0 & 4'b0100) new_cc_z = 0;
			       if (isn_3_0 & 4'b0010) new_cc_v = 0;
			       if (isn_3_0 & 4'b0001) new_cc_c = 0;
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
			  6'o074, 6'o75, 6'o76, 6'o77:
			    begin
			       if (isn_3_0 & 4'b1000) new_cc_n = 1;
			       if (isn_3_0 & 4'b0100) new_cc_z = 1;
			       if (isn_3_0 & 4'b0010) new_cc_v = 1;
			       if (isn_3_0 & 4'b0001) new_cc_c = 1;
			       latch_cc = 1;
			    end
		      endcase // case(isn_5_0)

		    6'o03:					    // swab
		      begin
			 $display("e: SWAB");
			 e1_result = {dd_data[7:0],dd_data[15:8]};
			 new_cc_n = sign_b(e1_result);
			 new_cc_z = zero_b(e1_result);
			 new_cc_v = 0;
			 new_cc_c = 0;
			 latch_cc = 1;
		      end
		    
		    6'o04, 6'o05:				    /* br */
		      begin
			 new_pc = new_pc_w(pc, isn);
			 latch_pc = 1;
			 $display("e: br; isn %o, pc %o, new_pc %o",
				  isn, pc, new_pc);
		      end

		    6'o06, 6'o07:				    /* br */
		      begin
			 new_pc = new_pc_b(pc, isn);
			 latch_pc = 1;
		      end

		    6'o10, 6'o11:				    /* bne */
		      begin
			 new_pc = new_pc_w(pc, isn);
			 latch_pc = ~cc_z;
		      end

		    6'o12, 6'o13:				    /* bne */
		      begin
			 new_pc = new_pc_b(pc, isn);
			 latch_pc = ~cc_z;
		      end

		    6'o14, 6'o15:				    /* beq */
		      begin
			 new_pc = new_pc_w(pc, isn);
			 latch_pc = cc_z;
		      end

		    6'o16, 6'o17:				    /* beq */
		      begin
			 new_pc = new_pc_b(pc, isn);
			 latch_pc = cc_z;
		      end

		    6'o20, 6'o21:				    /* bge */
		      begin
			 new_pc = new_pc_w(pc, isn);
			 latch_pc = (cc_n ^ cc_v) ? 0 : 1;
		      end

		    6'o22, 6'o23:				    /* bge */
		      begin
			 new_pc = new_pc_b(pc, isn);
			 latch_pc = (cc_n ^ cc_v) ? 0 : 1;
		      end

		    6'o24, 6'o25:				    /* blt */
		      begin
			 new_pc = new_pc_w(pc, isn);
			 latch_pc = (cc_n ^ cc_v);
		      end

		    6'o26, 6'o27:				    /* blt */
		      begin
			 new_pc = new_pc_b(pc, isn);
			 latch_pc = (cc_n ^ cc_v);
		      end

		    6'o30, 6'o31:				    /* bgt */
		      begin
			 new_pc = new_pc_w(pc, isn);
			 latch_pc = (cc_z || cc_n ^ cc_v) ? 0 : 1;
		      end

		    6'o32, 033:				    /* bgt */
		      begin
			 new_pc = new_pc_b(pc, isn);
			 latch_pc = (cc_z || cc_n ^ cc_v) ? 0 : 1;
		      end

		    6'o34, 6'o35:				    /* ble */
		      begin
			 new_pc = new_pc_w(pc, isn);
			 latch_pc = (cc_z || cc_n ^ cc_v);
		      end

		    6'o36, 6'o37:				    /* ble */
		      begin
			 new_pc = new_pc_b(pc, isn);
			 latch_pc = (cc_z || cc_n ^ cc_v);
		      end

		    6'o40, 6'o41, 6'o42, 6'o43,	    /* jsr */
		      6'o44, 6'o45, 6'o46, 6'o47:
			begin
			   $display(" JSR r%d; dd_data %6o, dd_ea %6o",
				    ss_reg, dd_data, dd_ea);
			   e1_result = pc;
			   new_pc = dd_ea;
			   latch_pc = 1;
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
			 e1_result = dd_data ^ 16'o177777;
			 new_cc_n = sign_w(e1_result);
			 new_cc_z = zero_w(e1_result);
			 new_cc_v = 0;
			 new_cc_c = 1;
			 latch_cc = 1;
		      end

		    6'o52:					    /* inc */
		      begin
			 e1_result = dd_data + 1;
			 new_cc_n = sign_w(e1_result);
			 new_cc_z = zero_w(e1_result);
			 new_cc_v = (e1_result == 16'o100000);
			 latch_cc = 1;
		      end

		    6'o53:					    /* dec */
		      begin
			 e1_result = dd_data - 1;
			 new_cc_n = sign_w(e1_result);
			 new_cc_z = zero_w(e1_result);
			 new_cc_v = (e1_result == 16'o77777);
			 latch_cc = 1;
		      end

		    6'o54:					    /* neg */
		      begin
			 e1_result = -dd_data;
			 new_cc_n = sign_w(e1_result);
			 new_cc_z = zero_w(e1_result);
			 new_cc_v = (e1_result == 16'o100000);
			 new_cc_c = new_cc_z ^ 1;
			 latch_cc = 1;
		      end

		    6'o55:					    /* adc */
		      begin
			 e1_result = dd_data + cc_c;
			 new_cc_n = sign_w(e1_result);
			 new_cc_z = zero_w(e1_result);
			 new_cc_v = (cc_c && (e1_result == 16'o100000));
			 new_cc_c = cc_c & new_cc_z;
			 latch_cc = 1;
		      end

		    6'o56:					    /* sbc */
		      begin
			 e1_result = dd_data - (cc_c);
			 new_cc_n = sign_w(e1_result);
			 new_cc_z = zero_w(e1_result);
			 new_cc_v = (cc_c && (e1_result == 16'o77777));
			 new_cc_c = (cc_c && (e1_result == 16'o177777));
			 latch_cc = 1;
		      end

		    6'o57:					    /* tst */
		      begin
			 e1_result = dd_data;
			 new_cc_n = sign_w(e1_result);
			 new_cc_z = zero_w(e1_result);
			 new_cc_v = 0;
			 new_cc_c = 0;
			 latch_cc = 1;
		      end

		    16'o60:					    /* ror */
		      begin
			 e1_result = (dd_data >> 1) | (cc_c << 15);
			 new_cc_n = sign_w(e1_result);
			 new_cc_z = zero_w(e1_result);
			 new_cc_c = dd_data & 1;
			 new_cc_v = new_cc_n ^ new_cc_c;
			 latch_cc = 1;
		      end

		    16'o61:					    /* rol */
		      begin
			 e1_result = {dd_data[14:0], cc_c};
			 new_cc_n = sign_w(e1_result);
			 new_cc_z = zero_w(e1_result);
			 new_cc_c = sign_w(dd_data);
			 new_cc_v = (new_cc_n ^ new_cc_c);
			 latch_cc = 1;
		      end

		    16'o62:					    /* asr */
		      begin
			 e1_result = (dd_data >> 1) | (dd_data & 16'o100000);
			 new_cc_n = sign_w(e1_result);
			 new_cc_z = zero_w(e1_result);
			 new_cc_c = dd_data & 1;
			 new_cc_v = (new_cc_n ^ new_cc_c);
			 latch_cc = 1;
		      end

		    16'o63:					    /* asl */
		      begin
			 e1_result = {dd_data[14:0], 1'b0};
			 new_cc_n = sign_w(e1_result);
			 new_cc_z = zero_w(e1_result);
			 new_cc_c = sign_w(dd_data);
			 new_cc_v = (new_cc_n ^ new_cc_c);
			 latch_cc = 1;
		      end

		    16'o64,					    /* mark */
		      16'o65,					    /* mfpi */
		      16'o66,					    /* mtpi */
		      16'o67,					    /* sxt */
		      16'o70,					    /* csm */
		      16'o72,					    /* tstset */
		      16'o73:					    /* wrtlck */
			begin
			end

		  endcase // case(isn_11_6)
	       end // else: !if(isn_11_6 == 16'o00 && isn_5_0 < 010)
	  end // if (isn_15_12 == 0)
	else
	  begin
	     if (0) $display("e: isn_15_12 != 0 (%o)", isn_15_12);
	     case (isn_15_12)
	       4'o01:					    /* mov */
		 begin
		    e1_result = ss_data;
		    new_cc_n = sign_w(e1_result);
		    new_cc_z = zero_w(e1_result);
		    new_cc_v = 0;
		    latch_cc = 1;
		 end

	       4'o02:					    /* cmp */
		 begin
		    //$display(" CMP %6o %6o", ss_data, dd_data);
		    e1_result = ss_data - dd_data;
		    new_cc_n = sign_w(e1_result);
		    new_cc_z = zero_w(e1_result);
		    new_cc_v = sign_w((ss_data ^ dd_data) &
				      (~dd_data ^ e1_result));
		    new_cc_c = ss_data < dd_data;
		    latch_cc = 1;
		 end

	       4'o03:					    /* bit */
		 begin
		    e1_result = ss_data & dd_data;
		    new_cc_n = sign_w(e1_result);
		    new_cc_z = zero_w(e1_result);
		    new_cc_v = 0;
		    latch_cc = 1;
		 end

	       4'o04:					    /* bic */
		 begin
		    e1_result = ~ss_data & dd_data;
		    new_cc_n = sign_w(e1_result);
		    new_cc_z = zero_w(e1_result);
		    new_cc_v = 0;
		    latch_cc = 1;
		 end

	       4'o05:					    /* bis */
		 begin
		    e1_result = ss_data | dd_data;
		    new_cc_n = sign_w(e1_result);
		    new_cc_z = zero_w(e1_result);
		    new_cc_v = 0;
		    latch_cc = 1;
		 end

	       4'o06:					    /* add */
		 begin
		    e1_result = ss_data + dd_data;
		    new_cc_n = sign_w(e1_result);
		    new_cc_z = zero_w(e1_result);
		    new_cc_v = sign_w((~ss_data ^ dd_data) &
				      (ss_data ^ e1_result));
		    new_cc_c = (e1_result < ss_data);
		    latch_cc = 1;
		 end

	       04'o7:
		 case (isn_11_9) 

		   0:					    /* mul */
		     begin
			$display(" MUL %o %o", ss_data, dd_data);
			e32_result = ss_data * dd_data;
			new_cc_n = e32_result[31];
			new_cc_z = e32_result == 0;
			new_cc_v = 0;
			new_cc_c = ((e32_result > 16'o77777) ||
				    (e32_result < -16'o100000));
			latch_cc = 1;
		     end

		   1:					    /* div */
		     begin
			if (dd_data == 0)
			  begin
			     new_cc_n = 0;
			     new_cc_z = 1;
			     new_cc_v = 1;
			     new_cc_c = 1;
			     latch_cc = 1;
			  end
			else if ((ss_reg_value == 16'h8000 &&
				  ss_rego1_value == 0) &&
				 (dd_data == 16'o177777))
			  begin
			     new_cc_v = 1;
			     new_cc_z = 1;
			     new_cc_n = 1;
			     new_cc_c = 1;
			     latch_cc = 1;
			  end
			else begin
			   e32_result = {ss_reg_value, ss_rego1_value} / dd_data;
			   if ((e32_result > 16'o77777) ||
			       (e32_result < -16'o100000))
			     begin
				new_cc_n = sign_l(e32_result);
				new_cc_v = 1;
				new_cc_z = 0;
				new_cc_c = 0;
				latch_cc = 1;
			     end else begin
				new_cc_n = sign_l(e32_result);
				new_cc_z = zero_l(e32_result);
				new_cc_v = 0;
				new_cc_c = 0;
				latch_cc = 1;
			     end
			end
		     end // case: 1
		   

		   2:					    /* ash */
		     begin
			sign = sign_w(ss_data);
			shift = dd_data & 077;

			if (shift == 0) begin			/* [0] */
			   e1_result = ss_data;
			   new_cc_v = 0;
			   new_cc_c = 0;
			   latch_cc = 1;
			end
			else if (shift <= 15) begin			/* [1,15] */
			   e1_result = ss_data << shift;
			   temp = ss_data >> (16 - shift);
			   new_cc_v = (temp != ((e1_result & 16'o100000)? 16'o177777: 0));
			   new_cc_c = (temp & 1);
			end
			else if (shift <= 31) begin			/* [16,31] */
			   e1_result = 0;
			   new_cc_v = (ss_data != 0);
			   new_cc_c = (ss_data << (shift - 16)) & 1;
			end
			else if (shift == 32) begin			/* [32] = -32 */
			   e1_result = -sign;
			   new_cc_v = 0;
			   new_cc_c = 0;
			end
			else begin					/* [33,63] = -31,-1 */
			   e1_result =
				      (ss_data >> (64 - shift)) |
				      (-sign << (shift - 32));
			   new_cc_v = 0;
			   new_cc_c = ((ss_data >> (63 - shift)) & 1);
			end

			new_cc_n = sign_w(e1_result);
			new_cc_z = zero_w(e1_result);
		     end // case: 2
		   
		   
		   3:					    /* ashc */
		     begin
			sign = sign_w(ss_data);
			shift = dd_data & 077;

			if (dd_data == 0) begin			/* [0] */
			   e32_result = ss_data;
			   new_cc_v = 0;
			   new_cc_c = 0;
			end
			else if (shift <= 31) begin		/* [1,31] */
			   e32_result = ss_data << shift;
			   temp = (ss_data >> (32 - shift)) | (-sign << shift);
			   new_cc_v = (temp != (e32_result[31] ? -1 : 0));
			   new_cc_c = (temp & 1);
			end
			else if (shift == 32) begin		/* [32] = -32 */
			   e32_result = -sign;
			   new_cc_v = 0;
			   new_cc_c = (ss_data >> 31) & 1;
			end
			else begin				/* [33,63] = -31,-1 */
			   e32_result = (ss_data >> (64 - shift)) | (-sign << (shift - 32));
			   new_cc_v = 0;
			   new_cc_c = ((ss_data >> (63 - shift)) & 1);
			end
			
			new_cc_n = sign_l(e32_result);
			new_cc_z = sign_l(e32_result);
		     end // case: 3

		   4:					    /* xor */
		     begin
			e1_result = ss_data ^ dd_data;
			new_cc_n = sign_w(e1_result);
			new_cc_z = zero_w(e1_result);
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
			e1_result = ss_data - 1;
			new_pc = pc - dd_data - dd_data;
			latch_pc = e1_result == 0 ? 0 : 1;
		     end
		 endcase // case(isn_11_9)

	       4'o10:
		 //if (1) $display(" e: 010 isn_11_6 %o", isn_11_6);
		 case (isn_11_6)
		   6'o00, 6'o01:				/* bpl */
		     begin
			$display("e: BPL"); 
			new_pc = new_pc_w(pc, isn);
			latch_pc = cc_n == 0;
		     end

		   6'o02, 6'o03:				/* bpl */
		     begin
			$display("e: BPLB");
			new_pc = new_pc_b(pc, isn);
			latch_pc = cc_n == 0;
		     end

		   6'o04, 6'o05:				/* bmi */
		     begin
			new_pc = new_pc_w(pc, isn);
			latch_pc = cc_n;
		     end

		   6'o06, 6'o07:				/* bmi */
		     begin
			new_pc = new_pc_b(pc, isn);
			latch_pc = cc_n;
		     end

		   6'o10, 6'o11:				/* bhi */
		     begin
			new_pc = new_pc_w(pc, isn);
			latch_pc = (cc_c | cc_z) == 0;
		     end

		   6'o12, 6'o13:				/* bhi */
		     begin
			new_pc = new_pc_b(pc, isn);
			latch_pc = (cc_c | cc_z) == 0;
		     end

		   6'o14, 6'o15:				/* blos */
		     begin
			new_pc = new_pc_w(pc, isn);
			latch_pc = (cc_c | cc_z);
		     end

		   6'o16, 6'o17:				/* blos */
		     begin
			new_pc = new_pc_b(pc, isn);
			latch_pc = (cc_c | cc_z);
		     end

		   6'o20, 6'o21:				/* bvc */
		     begin
			new_pc = new_pc_w(pc, isn);
			latch_pc = cc_v == 0;
		     end

		   6'o22, 6'o23:				/* bvc */
		     begin
			new_pc = new_pc_b(pc, isn);
			latch_pc = cc_v == 0;
		     end

		   6'o24, 6'o25:				/* bvs */
		     begin
			new_pc = new_pc_w(pc, isn);
			latch_pc = cc_v;
		     end

		   6'o26, 6'o27:				/* bvs */
		     begin
			new_pc = new_pc_b(pc, isn);
			latch_pc = cc_v;
		     end

		   6'o30, 6'o31:				/* bcc */
		     begin
			new_pc = new_pc_w(pc, isn);
			latch_pc = cc_c == 0;
		     end

		   6'o32, 6'o33:				/* bcc */
		     begin
			new_pc = new_pc_b(pc, isn);
			latch_pc = cc_c == 0;
		     end

		   6'o34, 6'o35:				/* bcs */
		     begin
			new_pc = new_pc_w(pc, isn);
			latch_pc = cc_c;
		     end

		   6'o36, 6'o37:				/* bcs */
		     begin
			new_pc = new_pc_b(pc, isn);
			latch_pc = cc_c;
		     end

		   6'o40, 6'o41, 6'o42, 6'o43:	/* emt */
		     begin
			$display(" EMT");
			assert_trap_emt = 1;
		     end

		   6'o44, 6'o45, 6'o46, 6'o47:	/* trap */
		     begin
			$display(" TRAP");
			assert_trap_trap = 1;
		     end

		   6'o50:				/* clrb */
		     begin
			new_cc_n = 0;
			new_cc_v = 0;
			new_cc_c = 0;
			new_cc_z = 1;
			latch_cc = 1;
			e1_result = {dd_data[15:8], 8'b0};
		     end

		   6'o51:				/* comb */
		     begin
			e1_result = (dd_data ^ 8'o377) & 8'o377;
			new_cc_n = sign_b(e1_result);
			new_cc_z = zero_b(e1_result);
			new_cc_v = 0;
			new_cc_c = 1;
			latch_cc = 1;
			//note: byte write of src - rmw to memory word
		     end

		   6'o52:				/* incb */
		     begin
			e1_result = {dd_data[15:8], (dd_data[7:0] + 1)};
			new_cc_n = sign_b(e1_result);
			new_cc_z = zero_b(e1_result);
			new_cc_v = 0;
			latch_cc = 1;
			//note: byte write of src - rmw to memory word
		     end

		   6'o53:				/* decb */
		     begin
			e1_result = {dd_data[15:8], (dd_data[7:0] - 1)};
			new_cc_n = sign_b(e1_result);
			new_cc_z = zero_b(e1_result);
			new_cc_v = e1_result[7:0] == 8'o177;
			latch_cc = 1;
			//note: byte write of src - rmw to memory word
		     end

		   6'o54:				/* negb */
		     begin
			e1_result = {dd_data[15:8], -dd_data[7:0]};
			new_cc_n = sign_b(e1_result);
			new_cc_z = zero_b(e1_result);
			new_cc_v = e1_result[7:0] == 16'o200;
			new_cc_c = new_cc_z ^ 1;
			latch_cc = 1;
			//note: byte write of src - rmw to memory word
		     end

		   6'o55:				/* adcb */
		     begin
			e1_result = {dd_data[15:8], (dd_data[7:0] + cc_c)};
			new_cc_n = sign_b(e1_result);
			new_cc_z = zero_b(e1_result);
			new_cc_v = cc_c && (e1_result[7:0] == 0200);
			new_cc_c = cc_c & new_cc_z;
			latch_cc = 1;
			//note: byte write of src - rmw to memory word
		     end

		   6'o56:				/* sbcb */
		     begin
			e1_result = {dd_data[15:8], (dd_data[7:0] - cc_c)};
			new_cc_n = sign_b(e1_result);
			new_cc_z = zero_b(e1_result);
			new_cc_v = cc_c && (e1_result[7:0] == 0177);
			new_cc_c = cc_c && (e1_result[7:0] == 0377);
			latch_cc = 1;
			//note: byte write of src - rmw to memory word
		     end

		   6'o57:				/* tstb */
		     begin
			$display(" TSTB %o", dd_data[7:0]);
			e1_result = {8'b0, dd_data[7:0]};
			$display(" TSTB %o, e1_result 0x%x", dd_data[7:0], e1_result);
			new_cc_n = sign_b(e1_result);
			new_cc_z = zero_b(e1_result);
			new_cc_v = 0;
			new_cc_c = 0;
			latch_cc = 1;
		     end

		   6'o60:				/* rorb */
		     begin
			e1_result = {dd_data[15:8], cc_c, dd_data[7:1]};
			new_cc_n = sign_b(e1_result);
			new_cc_z = zero_b(e1_result);
			new_cc_c = dd_data[0];
			new_cc_v = new_cc_n ^ new_cc_c;
			latch_cc = 1;
			//note: byte write of src - rmw to memory word
		     end

		   061:				/* rolb */
		     begin
			e1_result = {dd_data[15:8], dd_data[6:0], cc_c};
			new_cc_n = sign_b(e1_result);
			new_cc_z = zero_b(e1_result);
			new_cc_c = dd_data[7];
			new_cc_v = new_cc_n ^ new_cc_c;
			latch_cc = 1;
			//note: byte write of src - rmw to memory word
		     end

		   6'o62:				/* asrb */
		     begin
			e1_result = {dd_data[15:8], dd_data[7], dd_data[7:1]};
			new_cc_n = sign_b(e1_result);
			new_cc_z = zero_b(e1_result);
			new_cc_c = ss_data[0];
			new_cc_v = new_cc_n ^ new_cc_c;
			latch_cc = 1;
			//note: byte write of src - rmw to memory word
		     end

		   6'o63:				/* aslb */
		     begin
			e1_result = {dd_data[15:8], dd_data[6:0], 1'b0};
			new_cc_n = sign_b(e1_result);
			new_cc_z = zero_b(e1_result);
			new_cc_c = ss_data[7];
			new_cc_v = new_cc_n ^ new_cc_c;
			latch_cc = 1;
			//note: byte write of src - rmw to memory word
		     end

		   6'o64:				/* mtps */
		     begin
			//		   if (current_mode == mode_kernel)
			begin
			   // ipl = (dd_data >> psw_v_ipl) & 07;
			   // trap_req = calc_ints (ipl, trap_req);
			end

			new_cc_n = dd_data[3];
			new_cc_z = dd_data[2];
			new_cc_v = dd_data[1];
			new_cc_c = dd_data[0];
			latch_cc = 1;
		     end

		   6'o65:				/* mfpd */
		     begin

			new_cc_n = sign_w(dd_data);
			new_cc_z = zero_w(dd_data);
			new_cc_v = 0;
			latch_cc = 1;
		     end

		   6'o66:				/* mtpd */
		     begin

			new_cc_n = sign_w(dd_data);
			new_cc_z = zero_w(dd_data);
			new_cc_v = 0;
			// pop data
		     end

		   6'o67:				/* mfps */
		     begin

			new_cc_n = sign_w(dd_data);
			new_cc_z = zero_w(dd_data);
			new_cc_v = 0;
			e1_result = dd_data[7] ? (0177400 | dd_data) : dd_data;
		     end
		   
		 endcase // case(isn_11_6)
	       

	       4'o11:					    /* movb */
		 begin
		    e1_result = {ss_data[7] ? 8'hff : 8'b0, ss_data[7:0]};
		    new_cc_n = sign_b(e1_result);
		    new_cc_z = zero_b(e1_result);
		    new_cc_v = 0;
		    latch_cc = 1;
		 end

	       4'o12:					    /* cmpb */
		 begin
		    e1_result = ss_data[7:0] - dd_data [7:0];
		    new_cc_n = sign_b(e1_result);
		    new_cc_z = zero_b(e1_result);
		    new_cc_v = sign_b( ((ss_data[7:0]) ^ (dd_data[7:0])) &
				       (~(dd_data[7:0]) ^ (e1_result[7:0])) );
		    new_cc_c = ss_data[7:0] < dd_data[7:0];
		    latch_cc = 1;
		 end

	       4'o13:					    /* bitb */
		 begin
		    e1_result = ss_data[7:0] & dd_data[7:0];
		    new_cc_n = sign_b(e1_result);
		    new_cc_z = zero_w(e1_result);
		    new_cc_v = 0;
		    latch_cc = 1;
		 end

	       4'o14:					    /* bicb */
		 begin
		    e1_result = ~ss_data[7:0] & dd_data[7:0];
		    new_cc_n = sign_b(e1_result);
		    new_cc_z = zero_w(e1_result);
		    new_cc_v = 0;
		    latch_cc = 1;
		 end

	       4'o15:					    /* bisb */
		 begin
		    e1_result = ss_data[7:0] | dd_data[7:0];
		    new_cc_n = sign_b(e1_result);
		    new_cc_z = zero_w(e1_result);
		    new_cc_v = 0;
		    latch_cc = 1;
		 end

	       4'o16:					    /* sub */
		 begin
		    if (0) $display(" SUB %6o %6o", ss_data, dd_data);
		    e1_result = dd_data - ss_data;
		    new_cc_n = sign_w(e1_result);
		    new_cc_z = zero_w(e1_result);
		    new_cc_v = sign_w((ss_data ^ dd_data) &
				      (~ss_data ^ e1_result));
		    new_cc_c = dd_data < ss_data;
		    latch_cc = 1;
		 end // case: 016
	     endcase // case(isn_15_12)
	  end // else: !if(isn_15_12 == 0)

	$display(" ss_data %6o, dd_data %6o, e1_result %6o",
		 ss_data, dd_data, e1_result);
	$display(" latch_pc %d, latch_cc %d", latch_pc, latch_cc);
	$display(" psw %o", psw);
     end // always @ (ss_data, dd_data)

   function sign_l;
      input [31:0] v;
      sign_l = v[31];
   endfunction

   function sign_w;
      input [15:0] v;
      sign_w = v[15];
   endfunction

   function sign_b;
      input [7:0]  v;
      sign_b = v[7];
   endfunction

   function zero_l;
      input [31:0] v;
      zero_l = v == 32'h0;
   endfunction

   function zero_w;
      input [15:0] v;
      zero_w = v == 16'h0;
   endfunction

   function zero_b;
      input [7:0]  v;
      zero_b = v == 8'h0;
   endfunction

   function [7:0] add8;
      input [7:0]  arg1;
      input [7:0]  arg2;
      add8 = arg1 + arg2;
   endfunction // add8
   
   function [15:0] new_pc_w;
      input [15:0] pc;
      input [15:0] isn;
      new_pc_w = pc + add8(isn[7:0], isn[7:0]);
   endfunction // new_pc_w

   function [15:0] new_pc_b;
      input [15:0] pc;
      input [15:0] isn;
      new_pc_b = pc + { 8'hff, add8(isn[7:0], isn[7:0])};
   endfunction // new_pc_w
   
endmodule // execute
