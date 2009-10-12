// mmu_regs.v
//
// decode bus address for KT-11 registers
// generate correct signals and addresses for mmu.v
// copyright Brad Parker <brad@heeltoe.com> 2009

module mmu_regs (clk, reset, iopage_addr, data_in, data_out, decode,
		 iopage_rd, iopage_wr, iopage_byte_op,
		 trap, vector,
		 pxr_wr, pxr_rd, pxr_addr, pxr_data_in, pxr_data_out,
		 pxr_trap);
   
   input clk;
   input reset;
   input [12:0] iopage_addr;
   input [15:0] data_in;
   input 	iopage_rd, iopage_wr, iopage_byte_op;
   output [15:0] data_out;
   output 	 decode;

   output 	 trap;
   output [7:0]  vector;

   output 	 pxr_wr;
   output 	 pxr_rd;
   output [7:0]  pxr_addr;
   reg [7:0] 	 pxr_addr;
   input [15:0]  pxr_data_in;
   output [15:0] pxr_data_out;
   input 	 pxr_trap;
 	 
   // internal
   wire 	 user_par_pdr_decode;
   wire 	 supr_par_pdr_decode;
   wire 	 kern_par_pdr_decode;
   wire 	 par_pdr_decode;
   wire 	 decode_reg;
   
   assign 	 user_par_pdr_decode = iopage_addr >= 13'o17600 &&
				       iopage_addr <= 13'o17676;
   assign 	 supr_par_pdr_decode = iopage_addr >= 13'o12200 &&
				       iopage_addr <= 13'o12276;
   assign 	 kern_par_pdr_decode = iopage_addr >= 13'o12300 &&
				       iopage_addr <= 13'o12376;

   assign 	 par_pdr_decode = user_par_pdr_decode |
				  supr_par_pdr_decode |
				  kern_par_pdr_decode;

   assign 	 decode_reg = (iopage_addr == 13'o17516) |
			      (iopage_addr == 13'o17572) |
			      (iopage_addr == 13'o17574) |
			      (iopage_addr == 13'o17576);
   
   assign 	 decode = decode_reg | par_pdr_decode;

   // pass data through
   assign data_out = pxr_data_in;
   assign pxr_data_out = data_in;

   // register/table read/write
   assign pxr_rd = iopage_rd && decode;
   assign pxr_wr = iopage_wr && decode;

   // par/pdr addressing from pdp11:
   //
   // a[4] = i/d; o=i,1=d
   // a[5] = par/pdr; 0=pdr,1=par
   // a[4:1] = offset
   //
   // 7600-7616 uid   000x-000x xxx
   // 7620-7636 udd   001x-001x xxx
   // 7640-7656 uia   010x-010x xxx
   // 7660-7676 uda   011x-011x xxx
   // 
   // 2200-2216 sid
   // 2220-2236 sdd
   // 2240-2256 sia
   // 2260-2276 sda
   // 
   // 2300-2316 kid   
   // 2320-2336 kdd
   // 2340-2356 kia
   // 2360-2376 kda

   // generate appropriate pxr address
   always @(iopage_addr or iopage_rd or iopage_wr or decode)
     begin
	if (decode && (iopage_rd || iopage_wr))
	  casex (iopage_addr)
	    // mmr0-3
	    13'o17572: pxr_addr = {8'b10000000};
	    13'o17574: pxr_addr = {8'b10000001};
	    13'o17576: pxr_addr = {8'b10000010};
	    13'o17516: pxr_addr = {8'b10000011};

	    // kernel, supervisor and user par & pdr registers
	    13'o176xx: pxr_addr = {1'b0,iopage_addr[5],2'b11,iopage_addr[4:1]};
	    13'o122xx: pxr_addr = {1'b0,iopage_addr[5],2'b01,iopage_addr[4:1]};
	    13'o123xx: pxr_addr = {1'b0,iopage_addr[5],2'b00,iopage_addr[4:1]};
	    
	    default: pxr_addr = 8'b0;
	  endcase
	else
	  pxr_addr = 8'b0;
     end

   //
   assign trap = pxr_trap;
   assign vector = 8'o250;
   
endmodule

