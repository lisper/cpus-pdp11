// mmu.v
// PDP-11 in verilog - mmu memory translation, KT-11 style
// copyright Brad Parker <brad@heeltoe.com> 2009-2010

`define mmu_1134
//`define mmu_1170

`ifdef mmu_1134
 `define no_super
`endif

module mmu(clk, reset, soft_reset,
	   cpu_va, cpu_cm, cpu_rd, cpu_wr, cpu_i_access, cpu_d_access,
	   cpu_trap, cpu_pa, fetch_va, trap_odd, signal_abort, signal_trap,
	   pxr_wr, pxr_rd, pxr_be, pxr_addr, pxr_data_in, pxr_data_out);

   input clk;
   input reset;
   input soft_reset;
   
   input [15:0] cpu_va;
   input [1:0] 	cpu_cm;
   input 	cpu_rd;
   input 	cpu_wr;
   input 	cpu_i_access;
   input 	cpu_d_access;
   input 	cpu_trap;

   input 	fetch_va;
   input 	trap_odd;
   output 	signal_abort;
   reg 		signal_abort;
   output 	signal_trap;
   reg 		signal_trap;

 	
   input 	pxr_wr;
   input 	pxr_rd;
   input [1:0] 	pxr_be;
   input [7:0] 	pxr_addr;
   input [15:0] pxr_data_in;

   output [21:0] cpu_pa;
   output [15:0] pxr_data_out;
   reg [15:0] pxr_data_out;

   // translation registers
   reg [15:0] 	 mmr0;
   reg [15:0] 	 mmr1;
   reg [15:0] 	 mmr2;
   reg [15:0] 	 mmr3;
   
   reg [7:0] 	 pdr_h[63:0];
   reg [7:0] 	 pdr_l[63:0];
   reg [7:0] 	 par_h[63:0];
   reg [7:0] 	 par_l[63:0];

   wire [2:0] 	 cpu_apf;
   wire [12:0] 	 cpu_df;
   wire [15:0] 	 cpu_paf;
   wire [6:0] 	 cpu_bn;
   
   wire [15:0] pdr_value;
   wire [2:0]  pdr_acf;
   wire [6:0]  pdr_plf;
   wire        pdr_ed;
   wire        pdr_w;

   wire [15:0] par_value;

   wire        mmu_on;		// mmr0[0]
   wire        maint_mode;	// mmr0[8]
   wire        traps_enabled;	// mmr0[9]

   wire        va_is_iopage;
   wire        pa_is_iopage;
   wire        map_address;
   wire        enable_d_space;

   wire [21:0] map_adder_22;
   wire [21:0] map_adder;
   wire [21:0] cpu_pa_mapped;

   wire [5:0]  pxr_index;
   wire [15:0] pdr_add_bits;
   wire [15:0] pdr_update_value;
   reg        pdr_update_a;
   reg 	      pdr_update_w;
   reg        update_pdr;
   
   wire       pg_len_err;
   wire       map22;
   
   // break down va into components
   assign cpu_apf = cpu_va[15:13];
   assign cpu_df = cpu_va[12:0];
   assign cpu_bn = cpu_df[12:6];

   // form index into p*r register file
   assign pxr_index = {cpu_trap ? 2'b00 : cpu_cm,
		       enable_d_space ? ~cpu_i_access : 1'b0,
		       cpu_apf};

`ifdef mmu_1134
   // 11/34a values
   parameter PDR_MASK = 16'o077716; /* acf low bit gone */
   parameter PDR_W_MASK_HI = 8'o177;
   parameter PDR_W_MASK_LO = 8'o016;
   parameter PAR_MASK = 16'o007777; /* 12 bits */
`endif

   assign map22 = 1'b0;
   
   // p*r address = {mm,i,apf}
   assign      pdr_value = {pdr_h[pxr_index], pdr_l[pxr_index]} & PDR_MASK;
   assign      par_value = {par_h[pxr_index], par_l[pxr_index]} & PAR_MASK;
   
   assign cpu_paf = par_value;

   assign va_is_iopage = cpu_va[15:13] == 3'b111;	/* 8k io page */
   
`ifdef debug_mmu
   always @(posedge clk)
     if (cpu_va != 0 && mmr0[0])
       $display("ZZZ: va %o, pa %o, cm %o, i %o, pxr_index %o",
		cpu_va, cpu_pa, cpu_cm, cpu_i_access, pxr_index);
`endif

   // MMR0_MME bit
   assign mmu_on = mmr0[0];
   assign maint_mode = mmr0[8];
`ifdef mmu_1170
   assign traps_enabled = mmr0[9];
`else   
   assign traps_enabled = 1'b1;
`endif
   
   // allow for split i & d
   assign enable_d_space = cpu_cm == 2'b00 ? mmr3[2] :
			   cpu_cm == 2'b01 ? mmr3[1] :
			   cpu_cm == 2'b11 ? mmr3[0] :
			   1'b0;

   // enable mapping if mmu on or using maint-mode w/destination access
   assign map_address = mmu_on || (maint_mode & cpu_d_access);

   // form 22 bit physical address
   assign map_adder_22 = {cpu_paf, 6'b0} + {9'b0, cpu_df};

   // compensate if doing 18 bit mapping
   assign map_adder = map22 ? map_adder_22 : {4'b0000, map_adder_22[17:0]};
   
   // pick va or mapped address
   assign cpu_pa_mapped = map_address ? map_adder : {6'b0, cpu_va};

   // map 18 bit iopage to 22 bit iopage if only doing 18 bit mapping
   // (iopage.v expects full 22 bit mapping - see bus.v)
   assign pa_is_iopage = ~map22 & cpu_pa_mapped[17:14] == 4'b1111;

   assign cpu_pa = va_is_iopage ? {6'o77, cpu_va} :
		   pa_is_iopage ? {6'o77, cpu_pa_mapped[15:0]} : cpu_pa_mapped;
   
   assign 	pdr_plf = pdr_value[14:8];

   assign 	pdr_ed = pdr_value[3];
   assign 	pdr_w = pdr_value[6];
   assign 	pdr_acf = pdr_value[2:0];
   
   // check bn against page length
   assign 	pg_len_err = pdr_ed ? cpu_bn < pdr_plf : cpu_bn > pdr_plf;

   wire [5:0] 	pxr_addr_5_0;
   assign 	pxr_addr_5_0 = pxr_addr[5:0];

 	
   // debug - clear registers
   integer 	i;
   initial
     begin
	for (i = 0; i < 64; i=i+1)
	  begin
	     pdr_h[i] = 8'b0;
	     pdr_l[i] = 8'b0;
	     par_h[i] = 8'b0;
	     par_l[i] = 8'b0;
	  end
     end
   
   //
   // decode logic for pdr bits
   //
   // check pdr_acf
   // update pdr_w
   // update pdr_a
   //
   
   reg 		update_mmr0_nonres;
   reg 		update_mmr0_ple;
   reg 		update_mmr0_ro;
   reg 		update_mmr0_trap_flag;
   reg 		update_mmr0_page;
   
   always @(pdr_acf or pg_len_err or mmr0 or cpu_wr or cpu_rd or
	    cpu_cm or trap_odd or mmu_on)
     begin

	update_pdr = 0;
	pdr_update_a = 0;
	pdr_update_w = 0;

	update_mmr0_nonres = 0;
	update_mmr0_ple = 0;
	update_mmr0_ro = 0;
	update_mmr0_trap_flag = 0;
	update_mmr0_page = 0;

	signal_abort = 0;
	signal_trap = 0;

`ifdef no_super
	if (cpu_cm == 2'b01 && (cpu_rd || cpu_wr) && mmu_on)
	  begin
	     update_mmr0_nonres = 1;
	     update_mmr0_page = 1;
	     signal_abort = 1;
 `ifdef debug
	     $display("zzz: no-super, signal abort, rd non-res");
	     $display("zzz: cpu_apf=%b, cpu_va=%o", cpu_apf, cpu_va);
 `endif
	  end
	else
`endif
	if (cpu_wr && mmu_on)
	  case (pdr_acf)
	    3'd0, 3'd3, 3'd7:	// non-res, unused, unused
	      begin
		 update_pdr = 1;
		 update_mmr0_page = 1;
		 update_mmr0_nonres = 1;
		 if (pg_len_err)
		   update_mmr0_ple = 1;
		 signal_abort = 1;
 `ifdef debug
		 $display("zzz: acf=%o, signal abort, wr non-res", pdr_acf);
`endif
	      end

	    3'd1, 3'd2:		// read-only
	      begin
		 update_pdr = 1;
		 update_mmr0_page = 1;
		 update_mmr0_ro = 1;
		 if (pg_len_err)
		   update_mmr0_ple = 1;
		 signal_abort = 1;
 `ifdef debug
		 $display("zzz: acf=%o, signal abort, wr r-o", pdr_acf);
 `endif
	      end

	    3'd4:		// read/write
	      begin
		 update_pdr = 1;
		 pdr_update_a = 1;	// set a bit
`ifdef mmu_1134
		 //on 11/34, abort non-res
		 update_mmr0_nonres = 1;
		 update_mmr0_page = 1;
		 signal_trap = 1;
 `ifdef debug
		 $display("zzz: acf=%o, signal trap, wr unused", pdr_acf);
 `endif
`endif		 
`ifdef mmu_1170
		 if (traps_enabled)	// trap enable
		   begin
		      update_mmr0_page = 1;
		      update_mmr0_trap_flag = 1;
		      signal_trap = 1;
 `ifdef debug
		      $display("zzz: acf=%o, signal trap, wr unused", pdr_acf);
 `endif
		   end
`endif
	      end

`ifdef mmu_1170
	    3'd5:		// read/write
	      begin
		 update_pdr = 1;
		 pdr_update_a = 1;	// set a bit
		 if (traps_enabled)	// trap enable
		   begin
		      update_mmr0_page = 1;
		      update_mmr0_trap_flag = 1;
		      signal_trap = 1;
 `ifdef debug
		      $display("zzz: acf=%o, signal trap, wr r/w", pdr_acf);
 `endif
		   end
	      end
`endif
	    
	    3'd6:		// read/write (ok)
	      begin
 `ifdef debug
		 $display("zzz: acf=6, set w; index %o, trap %b, cm %b",
			  pxr_index, cpu_trap, cpu_cm);
 `endif
		 update_pdr = 1;
		 update_mmr0_page = 1;
		 if (~trap_odd)
		   pdr_update_w = 1;	// set w bit
		 if (pg_len_err)
		   begin
		      update_mmr0_ple = 1;
		      signal_trap = 1;
 `ifdef debug
		      $display("zzz: signal trap, wr len");
 `endif
		   end
	      end
	  endcase
	else
	  if (cpu_rd && mmu_on)
	    case (pdr_acf)
	      3'd0, 3'd3, 3'd7:	// non-res, unused, unused
		begin
//		   if (~trap_odd)
//		     pdr_update_w = 1;	// set w bit
		   update_mmr0_nonres = 1;
		   update_mmr0_page = 1;
		   if (pg_len_err)
		     begin
			update_mmr0_ple = 1;
			signal_abort = 1;
 `ifdef debug
			$display("zzz: acf=%o, signal abort, rd non-res",
				 pdr_acf);
 `endif
		     end
		   else
		     begin
			signal_abort = 1;
 `ifdef debug
			$display("zzz: acf=%o, signal abort, rd non-res",
				 pdr_acf);
 `endif
		     end
		end

`ifdef mmu_1170
	      3'd1:		// read-only
		begin
		   update_pdr = 1;
		   pdr_update_a = 1;	// set a bit
		   if (traps_enabled) 	// trap enable
		     begin
			update_mmr0_page = 1;
			update_mmr0_trap_flag = 1;
			signal_trap = 1;
 `ifdef debug
			$display("zzz: acf=%o, signal trap, rd r-o", pdr_acf);
 `endif
		     end
		end
`endif

	      3'd4:		// read/write
		begin
		   update_pdr = 1;
		   pdr_update_a = 1;	// set a bit
`ifdef mmu_1134
		   update_mmr0_page = 1;
		   update_mmr0_nonres = 1;
		   signal_trap = 1;
 `ifdef debug
		   $display("zzz: acf=%o, signal trap, rd r-w", pdr_acf);
 `endif
`endif
`ifdef mmu_1170
		   if (traps_enabled) 	// trap enable
		     begin
			update_mmr0_page = 1;
			update_mmr0_trap_flag = 1;
			signal_trap = 1;
 `ifdef debug
			$display("zzz: acf=%o, signal trap, rd r-w", pdr_acf);
 `endif
		     end
`endif
		end

	      3'd2, 3'd5, 3'd6:	// read-only, read/write, read/write
		begin
		   update_mmr0_page = 1;
		   
		   if (pg_len_err)
		     begin
			update_mmr0_ple = 1;
			signal_abort = 1;
 `ifdef debug
			$display("zzz: acf=%o, signal abort, rd len", pdr_acf);
 `endif
		     end
		end
	    endcase
     end
   
   //
   // all par's & pdr's are stored in one of two 64x16 register files
   // the address is used to break into k/s/u & i/d spaces
   //
   // pxr_addr bits:
   //
   // 7 1=mmr,0=pxr
   // 6 1=par,0=pdr
   // 5 mode[1]		cpu mode (00=kernel, 01=super, 11=user)
   // 4 mode[0]
   // 3 0=I,1=D
   // 2 apf[2]		pa[15:13]
   // 1 apf[1]
   // 0 apf[0]
   //
   
   // read pxr table/reg
   always @(pxr_rd or pxr_addr or pxr_addr_5_0 or
	    mmr0 or mmr1 or mmr2 or mmr3 or
	    mmu_on or cpu_cm or cpu_apf
	    /*or par_h or par_l or pdr_h or pdr_l*/)
     begin
	pxr_data_out = 16'b0;
	if (pxr_rd)
	  /* verilator lint_off CASEX */
	  casex (pxr_addr)
	    8'b10xxxx00:
	      begin
		 pxr_data_out =
			// fault; return latched value
			(mmr0[15] | mmr0[14] | mmr0[13] | ~mmu_on) ? mmr0 :
			// no fault; return "live" value for current access
			{ mmr0[15:7], cpu_cm, mmr0[4], cpu_apf, mmr0[0] };
`ifdef debug_mmu
		 $display("mmu: read mmr0 -> %o", mmr0);
`endif
	      end
	    8'b10xxxx01: pxr_data_out = mmr1;
	    8'b10xxxx10:
	      begin
		 pxr_data_out = mmr2;
`ifdef debug_mmu
		 $display("mmu: read mmr2 -> %o", mmr2);
`endif
	      end
	    8'b10xxxx11: pxr_data_out = mmr3;
	    8'b01xxxxxx:
	      begin
		 pxr_data_out =
			{par_h[pxr_addr_5_0], par_l[pxr_addr_5_0]} & PAR_MASK;
`ifdef debug_mmu
		 $display("mmu: read par[%o] -> %o; %t",
			  pxr_addr_5_0,
			  {par_h[pxr_addr_5_0], par_l[pxr_addr_5_0]} & PAR_MASK,
			  $time);
`endif
	      end
	    8'b00xxxxxx:
	      begin
		 pxr_data_out =
			{pdr_h[pxr_addr_5_0], pdr_l[pxr_addr_5_0]} & PDR_MASK;
`ifdef debug_mmu
		 $display("mmu: read pdr[%o] -> %o; %t",
			  pxr_addr_5_0,
			  {pdr_h[pxr_addr_5_0], pdr_l[pxr_addr_5_0]} & PDR_MASK,
			  $time);
`endif
	      end
	    default: pxr_data_out = 16'b0;
	  endcase
     end

   assign pdr_add_bits = { 8'b0, pdr_update_a, pdr_update_w, 6'b0 };

   assign pdr_update_value = pdr_value | pdr_add_bits;

   wire local_reset;
   assign local_reset = reset || soft_reset;
   
   // write pxr table/reg
   always @(posedge clk or posedge local_reset)
     if (local_reset)
       begin
 `ifdef debug
	  if (soft_reset)
	    $display("mmu: soft reset; %t", $time);
 `endif
	  mmr0 <= 0;
	  mmr3 <= 0;
       end
     else
       if (pxr_wr)
	 /* verilator lint_off CASEX */
	 casex (pxr_addr)
	   8'b10xxxx00: begin
	      case (pxr_be)
		2'b10: mmr0 <= {pxr_data_in[15:8], mmr0[7:0]};
		2'b01: mmr0 <= {mmr0[15:8], pxr_data_in[7:0]};
		default: mmr0 <= pxr_data_in;
	      endcase

`ifdef debug_mmu
	      $display("mmu: write mmr0 <- %o", pxr_data_in);
`endif
	   end

	   8'b10xxxx11: begin
	      mmr3 <= pxr_data_in;
`ifdef debug_mmu
	      $display("mmu: write mmr3 <- %o", pxr_data_in);
`endif
	     end

	   8'b01xxxxxx: begin
	      if (pxr_be[1])
		  par_h[pxr_addr_5_0] <= pxr_data_in[15:8];
	      if (pxr_be[0])
		  par_l[pxr_addr_5_0] <= pxr_data_in[7:0];
`ifdef debug_mmu
	      $display("mmu: write par[%o] <- %o; pxr_be %b; %t",
		       pxr_addr_5_0, pxr_data_in, pxr_be, $time);
`endif
	     end

	   8'b00xxxxxx: begin
	      if (pxr_be[1])
		pdr_h[pxr_addr_5_0] <= pxr_data_in[15:8] & PDR_W_MASK_HI;
	      if (pxr_be[0])
		pdr_l[pxr_addr_5_0] <= pxr_data_in[7:0] & PDR_W_MASK_LO;
`ifdef debug_mmu
      	      $display("mmu: write pdr[%o] <- %o; pxr_addr %o, pxr_be %b; %t",
		       pxr_addr_5_0, pxr_data_in, pxr_addr, pxr_be, $time);
`endif
	     end
	 endcase
       else
	 if (mmu_on)
	   begin
	      if (update_pdr)
		begin
		   pdr_h[pxr_index] <= pdr_update_value[15:8];
		   pdr_l[pxr_index] <= pdr_update_value[7:0];
		   
`ifdef debug_mmu
		   $display("mmu: update pdr[%o] <- %o; %t",
			    pxr_index, pdr_update_value, $time);
`endif
		end

	      // update mmr0 if requested,
	      //  but only if there are no error bits set
	      if ((update_mmr0_nonres ||
		   update_mmr0_ple ||
		   update_mmr0_ro ||
		   update_mmr0_trap_flag ||
		   update_mmr0_page) &&
		  ~(mmr0[15] | mmr0[14] | mmr0[13]))
		begin
		   mmr0 <= {
			    (update_mmr0_nonres ? 1'b1 : mmr0[15]),
			    (update_mmr0_ple ? 1'b1 : mmr0[14]),
			    (update_mmr0_ro ? 1'b1 : mmr0[13]),
			    (update_mmr0_trap_flag ? 1'b1 : mmr0[12]),
			    mmr0[11:7],
			    (update_mmr0_page ? cpu_cm : mmr0[6:5]),
			    mmr0[4],
			    (update_mmr0_page ? cpu_apf : mmr0[3:1]),
			    mmr0[0]
			    };
`ifdef debug_mmu
		   #2 $display("mmu: update mmr0 <- %o", mmr0);
`endif
		end
		
	   end // if (mmu_on)
   
   always @(posedge clk or posedge local_reset)
     if (local_reset)
       mmr2 <= 0;
     else
       // mmr2 should "track" va during fetch_va until fault
`ifdef no_super
       if (fetch_va && ~(mmr0[15] | mmr0[14] | mmr0[13] || cpu_cm == 2'b01))
	 mmr2 <= cpu_va;
`else
       if (fetch_va && ~(mmr0[15] | mmr0[14] | mmr0[13]))
	 mmr2 <= cpu_va;
`endif
   
   always @(posedge clk)
     if (reset)
       mmr1 <= 0;
//     else
//       if (fetch_incdec)
//	 mmr1 <= incdec;
   
   
endmodule
