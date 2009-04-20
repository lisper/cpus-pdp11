// bootrom.v
// basic rk11 bootrom, residing at 1730000

module bootrom(clk, reset, iopage_addr, data_in, data_out, decode,
	       iopage_rd, iopage_wr, iopage_byte_op);
   
   input clk;
   input reset;
   input [12:0] iopage_addr;
   input [15:0] data_in;
   input 	iopage_rd, iopage_wr, iopage_byte_op;
   output [15:0] data_out;
   output 	 decode;

   assign 	 decode = (iopage_addr >= 16'o13000) &&
			  (iopage_addr <= 16'o13776);

   wire [7:0] 	 offset;
   assign 	 offset = {iopage_addr[7:1], 1'b0};

   reg [15:0] 	 fetch;

   assign 	 data_out = iopage_byte_op ?
			    {8'b0, iopage_addr[0] ? fetch[15:8] : fetch[7:0]} :
			    fetch;
   
   always @(decode or offset or iopage_addr or iopage_rd or data_out)
     if (iopage_rd && decode)
       begin
       case (offset)
`define boot_rk
//`define boot_tt
`ifdef boot_rk
	 0: fetch = 16'o010000;	/* nop */
	 2: fetch = 16'o012706;
	 4: fetch = 16'o002000;	/* MOV #boot_start, SP */
	 6: fetch = 16'o012700;
	 8: fetch = 16'o000000;	/* MOV #unit, R0    ; unit number */
	 10: fetch = 16'o010003;	/* MOV R0, R3 */
	 12: fetch = 16'o000303;	/* SWAB R3 */
	 14: fetch = 16'o006303;	/* ASL R3 */
	 16: fetch = 16'o006303;	/* ASL R3 */
	 18: fetch = 16'o006303;	/* ASL R3 */
	 20: fetch = 16'o006303;	/* ASL R3 */
	 22: fetch = 16'o006303;	/* ASL R3 */
	 24: fetch = 16'o012701;
	 26: fetch = 16'o177412;	/* MOV #RKDA, R1    ; csr */
	 28: fetch = 16'o010311;	/* MOV R3, (R1)	    ; load da */
	 30: fetch = 16'o005041;	/* CLR -(R1)	    ; clear ba */
	 32: fetch = 16'o012741;
	 34: fetch = 16'o177000;	/* MOV #-256.*2, -(R1)  ; load wc */
	 36: fetch = 16'o012741; 
	 38: fetch = 16'o000005;	/* MOV #READ+GO, -(R1)  ; read & go */
	 40: fetch = 16'o005002;	/* CLR R2 */
	 42: fetch = 16'o005003;	/* CLR R3 */
	 44: fetch = 16'o012704;
	 46: fetch = 16'o002000+16'o020;	/* MOV #START+20, R4 */
	 48: fetch = 16'o005005;	/* CLR R5 */
	 50: fetch = 16'o105711;	/* TSTB (R1) */
	 52: fetch = 16'o100376;	/* BPL .-2 */
	 54: fetch = 16'o105011;	/* CLRB (R1) */
	 56: fetch = 16'o005007;	/* CLR PC */
	 default: fetch = 16'o0;
 `endif

 `ifdef boot_tt
	 0: fetch = 16'o12706; /* 000 */
	 2: fetch = 16'o500;   /* 002 */
	 4: fetch = 16'o12700; /* 004 	mov	#msg, r0 */
	 6: fetch = 16'o173044;/* 006 */
	 8: fetch = 16'h9401;  /* 010       	movb	(r0)+, r1 */
	 10: fetch = 16'h0303; /* 012       	beq	e <done> */
	 12: fetch = 16'h09f7; /* 014      	jsr	pc, 024 <tpchr> */
	 14: fetch = 16'h0004; /* 016 */
	 16: fetch = 16'h01fb; /* 020       	br	10 <loop> */
	 18: fetch = 16'h01f8; /* 022        br      4 */
	 20: fetch = 16'h8bff; /* 024      	tstb	*$1e <tps> */
	 22: fetch = 16'h000a; /* 026 */
	 24: fetch = 16'h80fd; /* 030       	bpl	024 <tpchr> */
	 26: fetch = 16'h907f; /* 032     	movb	r1, *$1c <tpb> */
	 28: fetch = 16'h0002; /* 034 */
	 30: fetch = 16'h0087; /* 036       	rts	pc */
	 32: fetch = 16'hff76; /* 040*/
	 34: fetch = 16'hff74; /* 042 */
	 36: fetch = 16'h0a0d; /* 044 */
	 38: fetch = 16'h6548;
	 40: fetch = 16'h6c6c;
	 42: fetch = 16'h206f;
	 44: fetch = 16'h6f77;
	 46: fetch = 16'h6c72;
	 48: fetch = 16'h2164;
	 50: fetch = 16'h0a0d;
	 52: fetch = 16'h0000;
	 default: fetch = 16'o0;
 `endif
	 
       endcase // case(offset)
	  #2 $display("rom fetch %o %o", iopage_addr, fetch);
	  
       end // if (iopage_rd)
     else
       fetch = 16'b0;
   
endmodule

