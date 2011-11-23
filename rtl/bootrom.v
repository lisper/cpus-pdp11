// bootrom.v
// basic rk11 bootrom, residing at 1730000

//`define boot_rk
//`define boot_tt
`define boot_diag

module bootrom(clk, reset, iopage_addr, data_in, data_out, decode,
	       iopage_rd, iopage_wr, iopage_byte_op);
   
   input clk;
   input reset;
   input [12:0] iopage_addr;
   input [15:0] data_in;
   input 	iopage_rd, iopage_wr, iopage_byte_op;
   output [15:0] data_out;
   output 	 decode;

   assign 	 decode = (iopage_addr >= 13'o13000) &&
			  (iopage_addr <= 13'o13776);

   wire [9:0] 	 offset;
   assign 	 offset = {iopage_addr[9:1], 1'b0};

   reg [15:0] 	 fetch;

   assign 	 data_out = iopage_byte_op ?
			    {8'b0, iopage_addr[0] ? fetch[15:8] : fetch[7:0]} :
			    fetch;
   
   always @(decode or offset or iopage_addr or iopage_rd or data_out or fetch)
     if (iopage_rd && decode)
       begin
       case (offset)
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
 `endif

 `ifdef boot_diag
0: fetch = 16'o012706;
2: fetch = 16'o007000;
4: fetch = 16'o004737;
6: fetch = 16'o131076;
8: fetch = 16'o004737;
10: fetch = 16'o130706;
12: fetch = 16'o004737;
14: fetch = 16'o130744;
16: fetch = 16'o012705;
18: fetch = 16'o006000;
20: fetch = 16'o122715;
22: fetch = 16'o000162;
24: fetch = 16'o001521;
26: fetch = 16'o122715;
28: fetch = 16'o000150;
30: fetch = 16'o001421;
32: fetch = 16'o122715;
34: fetch = 16'o000144;
36: fetch = 16'o001417;
38: fetch = 16'o122715;
40: fetch = 16'o000145;
42: fetch = 16'o001447;
44: fetch = 16'o122715;
46: fetch = 16'o000147;
48: fetch = 16'o001474;
50: fetch = 16'o122715;
52: fetch = 16'o000151;
54: fetch = 16'o001532;
56: fetch = 16'o122715;
58: fetch = 16'o000170;
60: fetch = 16'o001563;
62: fetch = 16'o000137;
64: fetch = 16'o130010;
66: fetch = 16'o000000;
68: fetch = 16'o004737;
70: fetch = 16'o130720;
72: fetch = 16'o062705;
74: fetch = 16'o000002;
76: fetch = 16'o010501;
78: fetch = 16'o004737;
80: fetch = 16'o130472;
82: fetch = 16'o010004;
84: fetch = 16'o010401;
86: fetch = 16'o004737;
88: fetch = 16'o130542;
90: fetch = 16'o112701;
92: fetch = 16'o000072;
94: fetch = 16'o004737;
96: fetch = 16'o131126;
98: fetch = 16'o004737;
100: fetch = 16'o130732;
102: fetch = 16'o012702;
104: fetch = 16'o000010;
106: fetch = 16'o012401;
108: fetch = 16'o004737;
110: fetch = 16'o130530;
112: fetch = 16'o077204;
114: fetch = 16'o004737;
116: fetch = 16'o130720;
118: fetch = 16'o000137;
120: fetch = 16'o130010;
122: fetch = 16'o004737;
124: fetch = 16'o130720;
126: fetch = 16'o062705;
128: fetch = 16'o000002;
130: fetch = 16'o010501;
132: fetch = 16'o004737;
134: fetch = 16'o130472;
136: fetch = 16'o010004;
138: fetch = 16'o010401;
140: fetch = 16'o004737;
142: fetch = 16'o130542;
144: fetch = 16'o112701;
146: fetch = 16'o000072;
148: fetch = 16'o004737;
150: fetch = 16'o131126;
152: fetch = 16'o004737;
154: fetch = 16'o130732;
156: fetch = 16'o012401;
158: fetch = 16'o004737;
160: fetch = 16'o130530;
162: fetch = 16'o004737;
164: fetch = 16'o130720;
166: fetch = 16'o000137;
168: fetch = 16'o130010;
170: fetch = 16'o004737;
172: fetch = 16'o130720;
174: fetch = 16'o062705;
176: fetch = 16'o000002;
178: fetch = 16'o010501;
180: fetch = 16'o004737;
182: fetch = 16'o130472;
184: fetch = 16'o010004;
186: fetch = 16'o000104;
188: fetch = 16'o012700;
190: fetch = 16'o000000;
192: fetch = 16'o010003;
194: fetch = 16'o000303;
196: fetch = 16'o006303;
198: fetch = 16'o006303;
200: fetch = 16'o006303;
202: fetch = 16'o006303;
204: fetch = 16'o006303;
206: fetch = 16'o012701;
208: fetch = 16'o177412;
210: fetch = 16'o010311;
212: fetch = 16'o005041;
214: fetch = 16'o012741;
216: fetch = 16'o177000;
218: fetch = 16'o012741;
220: fetch = 16'o000005;
222: fetch = 16'o105711;
224: fetch = 16'o100376;
226: fetch = 16'o105011;
228: fetch = 16'o004737;
230: fetch = 16'o130720;
232: fetch = 16'o000137;
234: fetch = 16'o130010;
236: fetch = 16'o005002;
238: fetch = 16'o012705;
240: fetch = 16'o000010;
242: fetch = 16'o004737;
244: fetch = 16'o130720;
246: fetch = 16'o010201;
248: fetch = 16'o004737;
250: fetch = 16'o130542;
252: fetch = 16'o004737;
254: fetch = 16'o130732;
256: fetch = 16'o012704;
258: fetch = 16'o177414;
260: fetch = 16'o010224;
262: fetch = 16'o012703;
264: fetch = 16'o177404;
266: fetch = 16'o012713;
268: fetch = 16'o000013;
270: fetch = 16'o105713;
272: fetch = 16'o100376;
274: fetch = 16'o105013;
276: fetch = 16'o011401;
278: fetch = 16'o004737;
280: fetch = 16'o130542;
282: fetch = 16'o004737;
284: fetch = 16'o130720;
286: fetch = 16'o077525;
288: fetch = 16'o000137;
290: fetch = 16'o130010;
292: fetch = 16'o012703;
294: fetch = 16'o177404;
296: fetch = 16'o012713;
298: fetch = 16'o000001;
300: fetch = 16'o000240;
302: fetch = 16'o000240;
304: fetch = 16'o105013;
306: fetch = 16'o004737;
308: fetch = 16'o130720;
310: fetch = 16'o000137;
312: fetch = 16'o130010;
314: fetch = 16'o010246;
316: fetch = 16'o005002;
318: fetch = 16'o112501;
320: fetch = 16'o001410;
322: fetch = 16'o042701;
324: fetch = 16'o177770;
326: fetch = 16'o006302;
328: fetch = 16'o006302;
330: fetch = 16'o006302;
332: fetch = 16'o050102;
334: fetch = 16'o000137;
336: fetch = 16'o130476;
338: fetch = 16'o010200;
340: fetch = 16'o012602;
342: fetch = 16'o000207;
344: fetch = 16'o004737;
346: fetch = 16'o130542;
348: fetch = 16'o004737;
350: fetch = 16'o130732;
352: fetch = 16'o000207;
354: fetch = 16'o010246;
356: fetch = 16'o010346;
358: fetch = 16'o012703;
360: fetch = 16'o131207;
362: fetch = 16'o004737;
364: fetch = 16'o130654;
366: fetch = 16'o004737;
368: fetch = 16'o130654;
370: fetch = 16'o004737;
372: fetch = 16'o130654;
374: fetch = 16'o004737;
376: fetch = 16'o130654;
378: fetch = 16'o004737;
380: fetch = 16'o130654;
382: fetch = 16'o004737;
384: fetch = 16'o130654;
386: fetch = 16'o114301;
388: fetch = 16'o004737;
390: fetch = 16'o131126;
392: fetch = 16'o114301;
394: fetch = 16'o004737;
396: fetch = 16'o131126;
398: fetch = 16'o114301;
400: fetch = 16'o004737;
402: fetch = 16'o131126;
404: fetch = 16'o114301;
406: fetch = 16'o004737;
408: fetch = 16'o131126;
410: fetch = 16'o114301;
412: fetch = 16'o004737;
414: fetch = 16'o131126;
416: fetch = 16'o114301;
418: fetch = 16'o004737;
420: fetch = 16'o131126;
422: fetch = 16'o012603;
424: fetch = 16'o012602;
426: fetch = 16'o000207;
428: fetch = 16'o010102;
430: fetch = 16'o042702;
432: fetch = 16'o177770;
434: fetch = 16'o062702;
436: fetch = 16'o000060;
438: fetch = 16'o110223;
440: fetch = 16'o042701;
442: fetch = 16'o000007;
444: fetch = 16'o000241;
446: fetch = 16'o006001;
448: fetch = 16'o006001;
450: fetch = 16'o006001;
452: fetch = 16'o000207;
454: fetch = 16'o012700;
456: fetch = 16'o131177;
458: fetch = 16'o004737;
460: fetch = 16'o131112;
462: fetch = 16'o000207;
464: fetch = 16'o012700;
466: fetch = 16'o131174;
468: fetch = 16'o004737;
470: fetch = 16'o131112;
472: fetch = 16'o000207;
474: fetch = 16'o112701;
476: fetch = 16'o000040;
478: fetch = 16'o004737;
480: fetch = 16'o131126;
482: fetch = 16'o000207;
484: fetch = 16'o012705;
486: fetch = 16'o006000;
488: fetch = 16'o004737;
490: fetch = 16'o131142;
492: fetch = 16'o022701;
494: fetch = 16'o000015;
496: fetch = 16'o001410;
498: fetch = 16'o022701;
500: fetch = 16'o000177;
502: fetch = 16'o001412;
504: fetch = 16'o004737;
506: fetch = 16'o131126;
508: fetch = 16'o110125;
510: fetch = 16'o000137;
512: fetch = 16'o130750;
514: fetch = 16'o112725;
516: fetch = 16'o000000;
518: fetch = 16'o112725;
520: fetch = 16'o000000;
522: fetch = 16'o000207;
524: fetch = 16'o022705;
526: fetch = 16'o006000;
528: fetch = 16'o001420;
530: fetch = 16'o162705;
532: fetch = 16'o000001;
534: fetch = 16'o012701;
536: fetch = 16'o000010;
538: fetch = 16'o004737;
540: fetch = 16'o131126;
542: fetch = 16'o012701;
544: fetch = 16'o000040;
546: fetch = 16'o004737;
548: fetch = 16'o131126;
550: fetch = 16'o012701;
552: fetch = 16'o000010;
554: fetch = 16'o004737;
556: fetch = 16'o131126;
558: fetch = 16'o000137;
560: fetch = 16'o130750;
562: fetch = 16'o012701;
564: fetch = 16'o000007;
566: fetch = 16'o004737;
568: fetch = 16'o131126;
570: fetch = 16'o000137;
572: fetch = 16'o130750;
574: fetch = 16'o000240;
576: fetch = 16'o012700;
578: fetch = 16'o131156;
580: fetch = 16'o004737;
582: fetch = 16'o131112;
584: fetch = 16'o000207;
586: fetch = 16'o112001;
588: fetch = 16'o001403;
590: fetch = 16'o004737;
592: fetch = 16'o131126;
594: fetch = 16'o000773;
596: fetch = 16'o000207;
598: fetch = 16'o105737;
600: fetch = 16'o177564;
602: fetch = 16'o100375;
604: fetch = 16'o110137;
606: fetch = 16'o177566;
608: fetch = 16'o000207;
610: fetch = 16'o105737;
612: fetch = 16'o177560;
614: fetch = 16'o100375;
616: fetch = 16'o113701;
618: fetch = 16'o177562;
620: fetch = 16'o000207;
622: fetch = 16'o005015;
624: fetch = 16'o062510;
626: fetch = 16'o066154;
628: fetch = 16'o020157;
630: fetch = 16'o067567;
632: fetch = 16'o066162;
634: fetch = 16'o020544;
636: fetch = 16'o005015;
638: fetch = 16'o006400;
640: fetch = 16'o071012;
642: fetch = 16'o066557;
644: fetch = 16'o020076;
646: fetch = 16'o000000;
648: fetch = 16'o000000;
650: fetch = 16'o000000;
652: fetch = 16'o001400;
 `endif
	 
	 default: fetch = 16'o0;
       endcase // case(offset)

	  //#2 $display("rom fetch %o %o", iopage_addr, fetch);
	  
       end // if (iopage_rd)
     else
       fetch = 16'b0;
   
endmodule

