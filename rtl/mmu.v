// mmu.v
// pdp-11 in verilog - mmu memory translation, KT-11 style
// copyright Brad Parker <brad@heeltoe.com> 2009

module mmu(clk, reset, cpu_va, cpu_cm, cpu_rd, cpu_wr, cpu_i_access, cpu_pa,
	   fetch_va, signal_abort, signal_trap,
	   pxr_wr, pxr_rd, pxr_addr, pxr_data_in, pxr_data_out);

   input clk;
   input reset;
   
   input [15:0] cpu_va;
   input [1:0] 	cpu_cm;
   input 	cpu_rd;
   input 	cpu_wr;
   input 	cpu_i_access;

   input 	fetch_va;
   output 	signal_abort;
   reg 		signal_abort;
   output 	signal_trap;
   reg 		signal_trap;
   
   input 	pxr_wr;
   input 	pxr_rd;
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
   
   reg [15:0] 	 pdr[63:0];
   reg [15:0] 	 par[63:0];

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

   wire        va_is_iopage;
   wire        mmu_on;
   wire        map_address;
   wire        enable_d_space;
   
   wire [21:0] cpu_pa_mapped;

   wire [5:0]  pxr_index;
   wire [15:0] pdr_add_bits;
   reg        pdr_update_a;
   reg 	      pdr_update_w;
   reg        update_pdr;
   
   wire       pg_len_err;

   
   // break down va into components
   assign cpu_apf = cpu_va[15:13];
   assign cpu_df = cpu_va[12:0];
   assign cpu_bn = cpu_df[12:6];

   assign pxr_index = {cpu_cm, ~cpu_i_access, cpu_apf};

   // p*r address = {mm,i,apf}
   assign      pdr_value = pdr[pxr_index];
   assign      par_value = par[pxr_index];
   
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

   assign enable_d_space = cpu_cm == 2'b00 ? mmr3[2] :
			   cpu_cm == 2'b01 ? mmr3[1] :
			   cpu_cm == 2'b11 ? mmr3[0] :
			   1'b0;
			   
   assign map_address = mmu_on &&
			(cpu_i_access || (~cpu_i_access && enable_d_space));
   
   // form 22 bit physical address
   assign cpu_pa_mapped = map_address ?
			  ({cpu_paf, 6'b0} + {9'b0, cpu_df}) :
			  {6'b0, cpu_va};

   // map io page accesses to end of address space
   assign cpu_pa = va_is_iopage ? {6'o77, cpu_va} : cpu_pa_mapped;
   
   assign 	pdr_plf = pdr_value[14:8];

   assign 	pdr_ed = pdr_value[3];
   assign 	pdr_w = pdr_value[6];
   assign 	pdr_acf = pdr_value[2:0];
   
   // check bn against page length
   assign 	pg_len_err = pdr_ed ?  cpu_bn < pdr_plf : cpu_bn > pdr_plf;

   // debug - clear registers
   integer 	i;
   initial
     begin
	for (i = 0; i < 64; i=i+1)
	  begin
	     pdr[i] = 16'b0;
	     par[i] = 16'b0;
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
   
   always @(pdr_acf or pg_len_err or mmr0)
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
	  
	if (cpu_wr)
	  case (pdr_acf)
	    3'd0, 3'd3, 3'd7:	// non-res, unused, unused
	      begin
		 update_pdr = 1;
		 update_mmr0_nonres = 1;
		 if (pg_len_err) update_mmr0_ple = 1;
		 signal_abort = 1;
	      end

	    3'd1, 3'd2:		// read-only
	      begin
		 update_pdr = 1;
		 update_mmr0_ro = 1;
		 if (pg_len_err) update_mmr0_ple = 1;
		 signal_abort = 1;
	      end

	    3'd4, 3'd5:		// unused, read/write
	      begin
		 update_pdr = 1;
		 pdr_update_a = 1;	// set a bit
		 if (mmr0[9]) 		// trap enable
		   begin
		      update_mmr0_page = 1;
		      update_mmr0_trap_flag = 1;
		      signal_trap = 1;
		   end
	      end
	    
	    3'd6:		// read/write (ok)
	      begin
		 update_pdr = 1;
		 pdr_update_w = 1;	// set w bit
		 if (pg_len_err)
		   begin
		      update_mmr0_ple = 1;
		      signal_trap = 1;
		   end
	      end
	  endcase
	else
	  if (cpu_rd)
	    case (pdr_acf)
	      3'd0, 3'd3, 3'd7:	// non-res, unused, unused
		begin
		   pdr_update_w = 1;	// set w bit
		   update_mmr0_nonres = 1;
		   signal_abort = 1;
		   if (pg_len_err)
		     begin
			update_mmr0_ple = 1;
			signal_trap = 1;
		     end
		end

	      3'd1, 3'd4:		// read-only, read/write
		begin
		   update_pdr = 1;
		   pdr_update_a = 1;	// set a bit
		   if (mmr0[9]) 		// trap enable
		     begin
			update_mmr0_page = 1;
			update_mmr0_trap_flag = 1;
			signal_trap = 1;
		     end
		end

	      3'd2, 3'd5, 3'd6:	// read-only, read/write, read/write
		begin
		   if (pg_len_err)
		     begin
			update_mmr0_ple = 1;
			signal_trap = 1;
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
   // 2 apf[2]		pa[15:12]
   // 1 apf[1]
   // 0 apf[0]
   //
   
   // read pxr table/reg
   always @(pxr_rd or pxr_addr or mmr0 or mmr1 or mmr2 or mmr3)
     begin
	pxr_data_out = 16'b0;
	if (pxr_rd)
	  /* verilator lint_off CASEX */
	  casex (pxr_addr)
	    8'b10xxxx00: pxr_data_out = mmr0;
	    8'b10xxxx01: pxr_data_out = mmr1;
	    8'b10xxxx10: pxr_data_out = mmr2;
	    8'b10xxxx11: pxr_data_out = mmr3;
	    8'b01xxxxxx: pxr_data_out = par[pxr_addr[5:0]];
	    8'b00xxxxxx: pxr_data_out = pdr[pxr_addr[5:0]];
	    default: pxr_data_out = 16'b0;
	  endcase
     end

   assign pdr_add_bits = { 8'b0, pdr_update_a, pdr_update_w, 6'b0 };

   // write pxr table/reg
   always @(posedge clk)
     if (reset)
       begin
	  mmr0 <= 0;
	  mmr3 <= 0;
       end
     else
       if (pxr_wr)
	 /* verilator lint_off CASEX */
	 casex (pxr_addr)
	   8'b10xxxx00: begin
	      mmr0 <= pxr_data_in;
`ifdef debug_mmu
	      $display("mmu: mmr0 <- %o", pxr_data_in);
`endif
	   end
	   8'b10xxxx11: begin
	      mmr3 <= pxr_data_in;
`ifdef debug_mmu
	      $display("mmu: mmr3 <- %o", pxr_data_in);
`endif
	     end
	   8'b01xxxxxx: begin
	      par[pxr_addr[5:0]] <= pxr_data_in;
`ifdef debug_mmu
	      $display("mmu: par[%o] <- %o", pxr_addr, pxr_data_in);
`endif
	     end
	   8'b00xxxxxx: begin
	      pdr[pxr_addr[5:0]] <= pxr_data_in;
`ifdef debug_mmu
	      $display("mmu: pdr[%o] <- %o", pxr_addr, pxr_data_in);
`endif
	     end
	 endcase
       else
	 begin
	    if (update_pdr)
	      pdr[pxr_index] <= pdr_value | pdr_add_bits;

	    if (update_mmr0_nonres ||
		update_mmr0_ple ||
		update_mmr0_ro ||
		update_mmr0_trap_flag ||
		update_mmr0_page)
	      mmr0 <= {
		       (update_mmr0_nonres ? 1'b1 : mmr0[15]),
		       (update_mmr0_ple ? 1'b1 : mmr0[14]),
		       (update_mmr0_ro ? 1'b1 : mmr0[13]),
		       (update_mmr0_trap_flag ? 1'b1 : mmr0[12]),
		       mmr0[11:4],
		       (update_mmr0_page ? cpu_apf : mmr0[3:1]),
		       mmr0[0]
		      };
	 end

   always @(posedge clk)
     if (reset)
       mmr2 <= 0;
     else
       if (fetch_va)
	 mmr2 <= cpu_va;

   always @(posedge clk)
     if (reset)
       mmr1 <= 0;
//     else
//       if (fetch_incdec)
//	 mmr1 <= incdec;
   
   
endmodule
