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
   reg [15:0] 	 data_out;
   output 	 decode;

   assign 	 decode = (iopage_addr >= 16'o13000) &&
			  (iopage_addr <= 16'o13776);

   wire [7:0] 	 offset;
   assign 	 offset = iopage_addr[7:0];
   
//   always @(posedge clk)
   always @(decode or offset or iopage_addr or iopage_rd or data_out)
     if (iopage_rd && decode)
       begin
       case (offset)
`define boot_rk
//`define boot_tt
`ifdef boot_rk
	 0: data_out = 16'o010000;	/* nop */
	 2: data_out = 16'o012706;
	 4: data_out = 16'o002000;	/* MOV #boot_start, SP */
	 6: data_out = 16'o012700;
	 8: data_out = 16'o000000;	/* MOV #unit, R0    ; unit number */
	 10: data_out = 16'o010003;	/* MOV R0, R3 */
	 12: data_out = 16'o000303;	/* SWAB R3 */
	 14: data_out = 16'o006303;	/* ASL R3 */
	 16: data_out = 16'o006303;	/* ASL R3 */
	 18: data_out = 16'o006303;	/* ASL R3 */
	 20: data_out = 16'o006303;	/* ASL R3 */
	 22: data_out = 16'o006303;	/* ASL R3 */
	 24: data_out = 16'o012701;
	 26: data_out = 16'o177412;	/* MOV #RKDA, R1    ; csr */
	 28: data_out = 16'o010311;	/* MOV R3, (R1)	    ; load da */
	 30: data_out = 16'o005041;	/* CLR -(R1)	    ; clear ba */
	 32: data_out = 16'o012741;
	 34: data_out = 16'o177000;	/* MOV #-256.*2, -(R1)  ; load wc */
	 36: data_out = 16'o012741; 
	 38: data_out = 16'o000005;	/* MOV #READ+GO, -(R1)  ; read & go */
	 40: data_out = 16'o005002;	/* CLR R2 */
	 42: data_out = 16'o005003;	/* CLR R3 */
	 44: data_out = 16'o012704;
	 46: data_out = 16'o002000+16'o020;	/* MOV #START+20, R4 */
	 48: data_out = 16'o005005;	/* CLR R5 */
	 50: data_out = 16'o105711;	/* TSTB (R1) */
	 52: data_out = 16'o100376;	/* BPL .-2 */
	 54: data_out = 16'o105011;	/* CLRB (R1) */
	 56: data_out = 16'o005007;	/* CLR PC */
	 default: data_out = 16'o0;
 `endif

 `ifdef boot_tt
	 0: data_out = 16'h1dc0; /*  	mov	$20 <msg>, r0 */
	 2: data_out = 16'h001c;
	 4: data_out = 16'h9401; /*         	movb	(r0)+, r1 */
	 6: data_out = 16'h0303; /*          	beq	e <done> */
	 8: data_out = 16'h09f7;
	 10: data_out = 16'h0004; /*      	jsr	pc, 10 <tpchr> */
	 12: data_out = 16'h01fb; /*          	br	4 <loop> */
	 14: data_out = 16'h/*0000*/01f8;
	 16: data_out = 16'h8bff; /*      	tstb	*$1e <tps> */
	 18: data_out = 16'h000a;
	 20: data_out = 16'h80fd; /*          	bpl	10 <tpchr> */
	 22: data_out = 16'h907f; /*      	movb	r1, *$1c <tpb> */
	 24: data_out = 16'h0002;
	 26: data_out = 16'h0087; /*          	rts	pc */
	 28: data_out = 16'hff76;
	 30: data_out = 16'hff74;
	 32: data_out = 16'h0a0d;
	 34: data_out = 16'h6548;
	 36: data_out = 16'h6c6c;
	 38: data_out = 16'h206f;
	 40: data_out = 16'h6f77;
	 42: data_out = 16'h6c72;
	 44: data_out = 16'h2164;
	 46: data_out = 16'h0a0d;
	 48: data_out = 16'h0000;
	 default: data_out = 16'o0;
 `endif
	 
       endcase // case(offset)
	  #2 $display("rom fetch %o %o", iopage_addr, data_out);
	  
       end // if (iopage_rd)
     else
       data_out = 16'b0;
   
endmodule

