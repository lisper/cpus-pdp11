// mmu_regs.v
//
// decode bus address for KT-11 registers
// generate correct signals and addresses for mmu.v
// copyright Brad Parker <brad@heeltoe.com> 2009

module mmu_regs (clk, reset, iopage_addr, data_in, data_out, decode,
		 iopage_rd, iopage_wr, iopage_byte_op,
		 trap, vector,
		 pxr_wr, pxr_rd, pxr_be, pxr_addr, pxr_data_in, pxr_data_out,
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
   output [1:0]  pxr_be;
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
   assign pxr_data_out = data_in;

   // handle byte accesses on read
   assign data_out = iopage_byte_op ?
		     { 8'b0, iopage_addr[0] ? 
		       pxr_data_in[15:8] : pxr_data_in[7:0] } :
		     pxr_data_in;

   // use byte enables for writes
   assign pxr_be = iopage_byte_op ? { iopage_addr[0], ~iopage_addr[0] } :
		   2'b11;
   
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
   // 7600-7616 uid   0011xxxx (0011 0000 - 0011 0111) 060 - 067
   // 7620-7636 udd   0011xxxx (0011 1000 - 0011 1111) 070 - 077
   // 7640-7656 uia   0111xxxx (0111 0000 - 0111 0111) 160 - 167
   // 7660-7676 uda   0111xxxx (0111 1000 - 0111 1111) 170 - 177
   // 
   // 2200-2216 sid   0001xxxx (0001 0000 - 0001 0111) 020 - 027
   // 2220-2236 sdd   0001xxxx (0001 1000 - 0001 1111) 030 -
   // 2240-2256 sia   0101xxxx (0101 0000 - 0001 0111) 120 -
   // 2260-2276 sda   0101xxxx (0101 1000 - 0001 0111) 130 - 
   // 
   // 2300-2316 kid   0000xxxx (0000 0000 - 0000 0111) 000 - 007
   // 2320-2336 kdd   0000xxxx (0000 1000 - 0000 1111) 010 - 017
   // 2340-2356 kia   0100xxxx (0100 0000 - 0100 0111) 100 - 107
   // 2360-2376 kda   0100xxxx (0100 1000 - 0100 1111) 110 - 117

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
   // this is pretty, but it's not used; trap_abort is hardwired in pdp11.v
   // someday we will generalize external aborts w/external vectors
   // like interrupts.
   //
   assign trap = pxr_trap;
   assign vector = 8'o250;
   
endmodule

