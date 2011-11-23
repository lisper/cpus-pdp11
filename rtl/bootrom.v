// bootrom.v
// basic rk11 bootrom, residing at 1773000 (173000)

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
			  (iopage_addr <= 13'o14776);

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
512: fetch = 16'o012706;
514: fetch = 16'o007000;
516: fetch = 16'o004737;
518: fetch = 16'o174076;
520: fetch = 16'o004737;
522: fetch = 16'o173706;
524: fetch = 16'o004737;
526: fetch = 16'o173744;
528: fetch = 16'o012705;
530: fetch = 16'o006000;
532: fetch = 16'o122715;
534: fetch = 16'o000162;
536: fetch = 16'o001521;
538: fetch = 16'o122715;
540: fetch = 16'o000150;
542: fetch = 16'o001421;
544: fetch = 16'o122715;
546: fetch = 16'o000144;
548: fetch = 16'o001417;
550: fetch = 16'o122715;
552: fetch = 16'o000145;
554: fetch = 16'o001447;
556: fetch = 16'o122715;
558: fetch = 16'o000147;
560: fetch = 16'o001474;
562: fetch = 16'o122715;
564: fetch = 16'o000151;
566: fetch = 16'o001532;
568: fetch = 16'o122715;
570: fetch = 16'o000170;
572: fetch = 16'o001563;
574: fetch = 16'o000137;
576: fetch = 16'o173010;
578: fetch = 16'o000000;
580: fetch = 16'o004737;
582: fetch = 16'o173720;
584: fetch = 16'o062705;
586: fetch = 16'o000002;
588: fetch = 16'o010501;
590: fetch = 16'o004737;
592: fetch = 16'o173472;
594: fetch = 16'o010004;
596: fetch = 16'o010401;
598: fetch = 16'o004737;
600: fetch = 16'o173542;
602: fetch = 16'o112701;
604: fetch = 16'o000072;
606: fetch = 16'o004737;
608: fetch = 16'o174126;
610: fetch = 16'o004737;
612: fetch = 16'o173732;
614: fetch = 16'o012702;
616: fetch = 16'o000010;
618: fetch = 16'o012401;
620: fetch = 16'o004737;
622: fetch = 16'o173530;
624: fetch = 16'o077204;
626: fetch = 16'o004737;
628: fetch = 16'o173720;
630: fetch = 16'o000137;
632: fetch = 16'o173010;
634: fetch = 16'o004737;
636: fetch = 16'o173720;
638: fetch = 16'o062705;
640: fetch = 16'o000002;
642: fetch = 16'o010501;
644: fetch = 16'o004737;
646: fetch = 16'o173472;
648: fetch = 16'o010004;
650: fetch = 16'o010401;
652: fetch = 16'o004737;
654: fetch = 16'o173542;
656: fetch = 16'o112701;
658: fetch = 16'o000072;
660: fetch = 16'o004737;
662: fetch = 16'o174126;
664: fetch = 16'o004737;
666: fetch = 16'o173732;
668: fetch = 16'o012401;
670: fetch = 16'o004737;
672: fetch = 16'o173530;
674: fetch = 16'o004737;
676: fetch = 16'o173720;
678: fetch = 16'o000137;
680: fetch = 16'o173010;
682: fetch = 16'o004737;
684: fetch = 16'o173720;
686: fetch = 16'o062705;
688: fetch = 16'o000002;
690: fetch = 16'o010501;
692: fetch = 16'o004737;
694: fetch = 16'o173472;
696: fetch = 16'o010004;
698: fetch = 16'o000104;
700: fetch = 16'o012700;
702: fetch = 16'o000000;
704: fetch = 16'o010003;
706: fetch = 16'o000303;
708: fetch = 16'o006303;
710: fetch = 16'o006303;
712: fetch = 16'o006303;
714: fetch = 16'o006303;
716: fetch = 16'o006303;
718: fetch = 16'o012701;
720: fetch = 16'o177412;
722: fetch = 16'o010311;
724: fetch = 16'o005041;
726: fetch = 16'o012741;
728: fetch = 16'o177000;
730: fetch = 16'o012741;
732: fetch = 16'o000005;
734: fetch = 16'o105711;
736: fetch = 16'o100376;
738: fetch = 16'o105011;
740: fetch = 16'o004737;
742: fetch = 16'o173720;
744: fetch = 16'o000137;
746: fetch = 16'o173010;
748: fetch = 16'o005002;
750: fetch = 16'o012705;
752: fetch = 16'o000010;
754: fetch = 16'o004737;
756: fetch = 16'o173720;
758: fetch = 16'o010201;
760: fetch = 16'o004737;
762: fetch = 16'o173542;
764: fetch = 16'o004737;
766: fetch = 16'o173732;
768: fetch = 16'o012704;
770: fetch = 16'o177414;
772: fetch = 16'o010224;
774: fetch = 16'o012703;
776: fetch = 16'o177404;
778: fetch = 16'o012713;
780: fetch = 16'o000013;
782: fetch = 16'o105713;
784: fetch = 16'o100376;
786: fetch = 16'o105013;
788: fetch = 16'o011401;
790: fetch = 16'o004737;
792: fetch = 16'o173542;
794: fetch = 16'o004737;
796: fetch = 16'o173720;
798: fetch = 16'o077525;
800: fetch = 16'o000137;
802: fetch = 16'o173010;
804: fetch = 16'o012703;
806: fetch = 16'o177404;
808: fetch = 16'o012713;
810: fetch = 16'o000001;
812: fetch = 16'o000240;
814: fetch = 16'o000240;
816: fetch = 16'o105013;
818: fetch = 16'o004737;
820: fetch = 16'o173720;
822: fetch = 16'o000137;
824: fetch = 16'o173010;
826: fetch = 16'o010246;
828: fetch = 16'o005002;
830: fetch = 16'o112501;
832: fetch = 16'o001410;
834: fetch = 16'o042701;
836: fetch = 16'o177770;
838: fetch = 16'o006302;
840: fetch = 16'o006302;
842: fetch = 16'o006302;
844: fetch = 16'o050102;
846: fetch = 16'o000137;
848: fetch = 16'o173476;
850: fetch = 16'o010200;
852: fetch = 16'o012602;
854: fetch = 16'o000207;
856: fetch = 16'o004737;
858: fetch = 16'o173542;
860: fetch = 16'o004737;
862: fetch = 16'o173732;
864: fetch = 16'o000207;
866: fetch = 16'o010246;
868: fetch = 16'o010346;
870: fetch = 16'o012703;
872: fetch = 16'o174207;
874: fetch = 16'o004737;
876: fetch = 16'o173654;
878: fetch = 16'o004737;
880: fetch = 16'o173654;
882: fetch = 16'o004737;
884: fetch = 16'o173654;
886: fetch = 16'o004737;
888: fetch = 16'o173654;
890: fetch = 16'o004737;
892: fetch = 16'o173654;
894: fetch = 16'o004737;
896: fetch = 16'o173654;
898: fetch = 16'o114301;
900: fetch = 16'o004737;
902: fetch = 16'o174126;
904: fetch = 16'o114301;
906: fetch = 16'o004737;
908: fetch = 16'o174126;
910: fetch = 16'o114301;
912: fetch = 16'o004737;
914: fetch = 16'o174126;
916: fetch = 16'o114301;
918: fetch = 16'o004737;
920: fetch = 16'o174126;
922: fetch = 16'o114301;
924: fetch = 16'o004737;
926: fetch = 16'o174126;
928: fetch = 16'o114301;
930: fetch = 16'o004737;
932: fetch = 16'o174126;
934: fetch = 16'o012603;
936: fetch = 16'o012602;
938: fetch = 16'o000207;
940: fetch = 16'o010102;
942: fetch = 16'o042702;
944: fetch = 16'o177770;
946: fetch = 16'o062702;
948: fetch = 16'o000060;
950: fetch = 16'o110223;
952: fetch = 16'o042701;
954: fetch = 16'o000007;
956: fetch = 16'o000241;
958: fetch = 16'o006001;
960: fetch = 16'o006001;
962: fetch = 16'o006001;
964: fetch = 16'o000207;
966: fetch = 16'o012700;
968: fetch = 16'o174177;
970: fetch = 16'o004737;
972: fetch = 16'o174112;
974: fetch = 16'o000207;
976: fetch = 16'o012700;
978: fetch = 16'o174174;
980: fetch = 16'o004737;
982: fetch = 16'o174112;
984: fetch = 16'o000207;
986: fetch = 16'o112701;
988: fetch = 16'o000040;
990: fetch = 16'o004737;
992: fetch = 16'o174126;
994: fetch = 16'o000207;
996: fetch = 16'o012705;
998: fetch = 16'o006000;
1000: fetch = 16'o004737;
1002: fetch = 16'o174142;
1004: fetch = 16'o022701;
1006: fetch = 16'o000015;
1008: fetch = 16'o001410;
1010: fetch = 16'o022701;
1012: fetch = 16'o000177;
1014: fetch = 16'o001412;
1016: fetch = 16'o004737;
1018: fetch = 16'o174126;
1020: fetch = 16'o110125;
1022: fetch = 16'o000137;
0: fetch = 16'o173750;
2: fetch = 16'o112725;
4: fetch = 16'o000000;
6: fetch = 16'o112725;
8: fetch = 16'o000000;
10: fetch = 16'o000207;
12: fetch = 16'o022705;
14: fetch = 16'o006000;
16: fetch = 16'o001420;
18: fetch = 16'o162705;
20: fetch = 16'o000001;
22: fetch = 16'o012701;
24: fetch = 16'o000010;
26: fetch = 16'o004737;
28: fetch = 16'o174126;
30: fetch = 16'o012701;
32: fetch = 16'o000040;
34: fetch = 16'o004737;
36: fetch = 16'o174126;
38: fetch = 16'o012701;
40: fetch = 16'o000010;
42: fetch = 16'o004737;
44: fetch = 16'o174126;
46: fetch = 16'o000137;
48: fetch = 16'o173750;
50: fetch = 16'o012701;
52: fetch = 16'o000007;
54: fetch = 16'o004737;
56: fetch = 16'o174126;
58: fetch = 16'o000137;
60: fetch = 16'o173750;
62: fetch = 16'o000240;
64: fetch = 16'o012700;
66: fetch = 16'o174156;
68: fetch = 16'o004737;
70: fetch = 16'o174112;
72: fetch = 16'o000207;
74: fetch = 16'o112001;
76: fetch = 16'o001403;
78: fetch = 16'o004737;
80: fetch = 16'o174126;
82: fetch = 16'o000773;
84: fetch = 16'o000207;
86: fetch = 16'o105737;
88: fetch = 16'o177564;
90: fetch = 16'o100375;
92: fetch = 16'o110137;
94: fetch = 16'o177566;
96: fetch = 16'o000207;
98: fetch = 16'o105737;
100: fetch = 16'o177560;
102: fetch = 16'o100375;
104: fetch = 16'o113701;
106: fetch = 16'o177562;
108: fetch = 16'o000207;
110: fetch = 16'o005015;
112: fetch = 16'o062510;
114: fetch = 16'o066154;
116: fetch = 16'o020157;
118: fetch = 16'o067567;
120: fetch = 16'o066162;
122: fetch = 16'o020544;
124: fetch = 16'o005015;
126: fetch = 16'o006400;
128: fetch = 16'o071012;
130: fetch = 16'o066557;
132: fetch = 16'o020076;
134: fetch = 16'o000000;
136: fetch = 16'o000000;
138: fetch = 16'o000000;
140: fetch = 16'o001400;
 `endif
	 
	 default: fetch = 16'o0;
       endcase // case(offset)

	  //#2 $display("rom fetch %o %o", iopage_addr, fetch);
	  
       end // if (iopage_rd)
     else
       fetch = 16'b0;
   
endmodule

