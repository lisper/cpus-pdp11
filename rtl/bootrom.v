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
512: fetch = 16'o000240;
514: fetch = 16'o012706;
516: fetch = 16'o001000;
518: fetch = 16'o012700;
520: fetch = 16'o173042;
522: fetch = 16'o112001;
524: fetch = 16'o001403;
526: fetch = 16'o004737;
528: fetch = 16'o173026;
530: fetch = 16'o000773;
532: fetch = 16'o000000;
534: fetch = 16'o105737;
536: fetch = 16'o177564;
538: fetch = 16'o100375;
540: fetch = 16'o110137;
542: fetch = 16'o177566;
544: fetch = 16'o000207;
546: fetch = 16'o005015;
548: fetch = 16'o062510;
550: fetch = 16'o066154;
552: fetch = 16'o020157;
554: fetch = 16'o067567;
556: fetch = 16'o066162;
558: fetch = 16'o020544;
560: fetch = 16'o005015;
562: fetch = 16'o000000;
 `endif

 `ifdef boot_diag
512: fetch = 16'o000240;
514: fetch = 16'o012706;
516: fetch = 16'o007000;
518: fetch = 16'o004737;
520: fetch = 16'o174100;
522: fetch = 16'o004737;
524: fetch = 16'o173710;
526: fetch = 16'o004737;
528: fetch = 16'o173746;
530: fetch = 16'o012705;
532: fetch = 16'o006000;
534: fetch = 16'o122715;
536: fetch = 16'o000162;
538: fetch = 16'o001521;
540: fetch = 16'o122715;
542: fetch = 16'o000150;
544: fetch = 16'o001421;
546: fetch = 16'o122715;
548: fetch = 16'o000144;
550: fetch = 16'o001417;
552: fetch = 16'o122715;
554: fetch = 16'o000145;
556: fetch = 16'o001447;
558: fetch = 16'o122715;
560: fetch = 16'o000147;
562: fetch = 16'o001474;
564: fetch = 16'o122715;
566: fetch = 16'o000151;
568: fetch = 16'o001532;
570: fetch = 16'o122715;
572: fetch = 16'o000170;
574: fetch = 16'o001563;
576: fetch = 16'o000137;
578: fetch = 16'o173012;
580: fetch = 16'o000000;
582: fetch = 16'o004737;
584: fetch = 16'o173722;
586: fetch = 16'o062705;
588: fetch = 16'o000002;
590: fetch = 16'o010501;
592: fetch = 16'o004737;
594: fetch = 16'o173474;
596: fetch = 16'o010004;
598: fetch = 16'o010401;
600: fetch = 16'o004737;
602: fetch = 16'o173544;
604: fetch = 16'o112701;
606: fetch = 16'o000072;
608: fetch = 16'o004737;
610: fetch = 16'o174130;
612: fetch = 16'o004737;
614: fetch = 16'o173734;
616: fetch = 16'o012702;
618: fetch = 16'o000010;
620: fetch = 16'o012401;
622: fetch = 16'o004737;
624: fetch = 16'o173532;
626: fetch = 16'o077204;
628: fetch = 16'o004737;
630: fetch = 16'o173722;
632: fetch = 16'o000137;
634: fetch = 16'o173012;
636: fetch = 16'o004737;
638: fetch = 16'o173722;
640: fetch = 16'o062705;
642: fetch = 16'o000002;
644: fetch = 16'o010501;
646: fetch = 16'o004737;
648: fetch = 16'o173474;
650: fetch = 16'o010004;
652: fetch = 16'o010401;
654: fetch = 16'o004737;
656: fetch = 16'o173544;
658: fetch = 16'o112701;
660: fetch = 16'o000072;
662: fetch = 16'o004737;
664: fetch = 16'o174130;
666: fetch = 16'o004737;
668: fetch = 16'o173734;
670: fetch = 16'o012401;
672: fetch = 16'o004737;
674: fetch = 16'o173532;
676: fetch = 16'o004737;
678: fetch = 16'o173722;
680: fetch = 16'o000137;
682: fetch = 16'o173012;
684: fetch = 16'o004737;
686: fetch = 16'o173722;
688: fetch = 16'o062705;
690: fetch = 16'o000002;
692: fetch = 16'o010501;
694: fetch = 16'o004737;
696: fetch = 16'o173474;
698: fetch = 16'o010004;
700: fetch = 16'o000104;
702: fetch = 16'o012700;
704: fetch = 16'o000000;
706: fetch = 16'o010003;
708: fetch = 16'o000303;
710: fetch = 16'o006303;
712: fetch = 16'o006303;
714: fetch = 16'o006303;
716: fetch = 16'o006303;
718: fetch = 16'o006303;
720: fetch = 16'o012701;
722: fetch = 16'o177412;
724: fetch = 16'o010311;
726: fetch = 16'o005041;
728: fetch = 16'o012741;
730: fetch = 16'o177000;
732: fetch = 16'o012741;
734: fetch = 16'o000005;
736: fetch = 16'o105711;
738: fetch = 16'o100376;
740: fetch = 16'o105011;
742: fetch = 16'o004737;
744: fetch = 16'o173722;
746: fetch = 16'o000137;
748: fetch = 16'o173012;
750: fetch = 16'o005002;
752: fetch = 16'o012705;
754: fetch = 16'o000010;
756: fetch = 16'o004737;
758: fetch = 16'o173722;
760: fetch = 16'o010201;
762: fetch = 16'o004737;
764: fetch = 16'o173544;
766: fetch = 16'o004737;
768: fetch = 16'o173734;
770: fetch = 16'o012704;
772: fetch = 16'o177414;
774: fetch = 16'o010224;
776: fetch = 16'o012703;
778: fetch = 16'o177404;
780: fetch = 16'o012713;
782: fetch = 16'o000013;
784: fetch = 16'o105713;
786: fetch = 16'o100376;
788: fetch = 16'o105013;
790: fetch = 16'o011401;
792: fetch = 16'o004737;
794: fetch = 16'o173544;
796: fetch = 16'o004737;
798: fetch = 16'o173722;
800: fetch = 16'o077525;
802: fetch = 16'o000137;
804: fetch = 16'o173012;
806: fetch = 16'o012703;
808: fetch = 16'o177404;
810: fetch = 16'o012713;
812: fetch = 16'o000001;
814: fetch = 16'o000240;
816: fetch = 16'o000240;
818: fetch = 16'o105013;
820: fetch = 16'o004737;
822: fetch = 16'o173722;
824: fetch = 16'o000137;
826: fetch = 16'o173012;
828: fetch = 16'o010246;
830: fetch = 16'o005002;
832: fetch = 16'o112501;
834: fetch = 16'o001410;
836: fetch = 16'o042701;
838: fetch = 16'o177770;
840: fetch = 16'o006302;
842: fetch = 16'o006302;
844: fetch = 16'o006302;
846: fetch = 16'o050102;
848: fetch = 16'o000137;
850: fetch = 16'o173500;
852: fetch = 16'o010200;
854: fetch = 16'o012602;
856: fetch = 16'o000207;
858: fetch = 16'o004737;
860: fetch = 16'o173544;
862: fetch = 16'o004737;
864: fetch = 16'o173734;
866: fetch = 16'o000207;
868: fetch = 16'o010246;
870: fetch = 16'o010346;
872: fetch = 16'o012703;
874: fetch = 16'o174211;
876: fetch = 16'o004737;
878: fetch = 16'o173656;
880: fetch = 16'o004737;
882: fetch = 16'o173656;
884: fetch = 16'o004737;
886: fetch = 16'o173656;
888: fetch = 16'o004737;
890: fetch = 16'o173656;
892: fetch = 16'o004737;
894: fetch = 16'o173656;
896: fetch = 16'o004737;
898: fetch = 16'o173656;
900: fetch = 16'o114301;
902: fetch = 16'o004737;
904: fetch = 16'o174130;
906: fetch = 16'o114301;
908: fetch = 16'o004737;
910: fetch = 16'o174130;
912: fetch = 16'o114301;
914: fetch = 16'o004737;
916: fetch = 16'o174130;
918: fetch = 16'o114301;
920: fetch = 16'o004737;
922: fetch = 16'o174130;
924: fetch = 16'o114301;
926: fetch = 16'o004737;
928: fetch = 16'o174130;
930: fetch = 16'o114301;
932: fetch = 16'o004737;
934: fetch = 16'o174130;
936: fetch = 16'o012603;
938: fetch = 16'o012602;
940: fetch = 16'o000207;
942: fetch = 16'o010102;
944: fetch = 16'o042702;
946: fetch = 16'o177770;
948: fetch = 16'o062702;
950: fetch = 16'o000060;
952: fetch = 16'o110223;
954: fetch = 16'o042701;
956: fetch = 16'o000007;
958: fetch = 16'o000241;
960: fetch = 16'o006001;
962: fetch = 16'o006001;
964: fetch = 16'o006001;
966: fetch = 16'o000207;
968: fetch = 16'o012700;
970: fetch = 16'o174201;
972: fetch = 16'o004737;
974: fetch = 16'o174114;
976: fetch = 16'o000207;
978: fetch = 16'o012700;
980: fetch = 16'o174176;
982: fetch = 16'o004737;
984: fetch = 16'o174114;
986: fetch = 16'o000207;
988: fetch = 16'o112701;
990: fetch = 16'o000040;
992: fetch = 16'o004737;
994: fetch = 16'o174130;
996: fetch = 16'o000207;
998: fetch = 16'o012705;
1000: fetch = 16'o006000;
1002: fetch = 16'o004737;
1004: fetch = 16'o174144;
1006: fetch = 16'o022701;
1008: fetch = 16'o000015;
1010: fetch = 16'o001410;
1012: fetch = 16'o022701;
1014: fetch = 16'o000177;
1016: fetch = 16'o001412;
1018: fetch = 16'o004737;
1020: fetch = 16'o174130;
1022: fetch = 16'o110125;
0: fetch = 16'o000137;
2: fetch = 16'o173752;
4: fetch = 16'o112725;
6: fetch = 16'o000000;
8: fetch = 16'o112725;
10: fetch = 16'o000000;
12: fetch = 16'o000207;
14: fetch = 16'o022705;
16: fetch = 16'o006000;
18: fetch = 16'o001420;
20: fetch = 16'o162705;
22: fetch = 16'o000001;
24: fetch = 16'o012701;
26: fetch = 16'o000010;
28: fetch = 16'o004737;
30: fetch = 16'o174130;
32: fetch = 16'o012701;
34: fetch = 16'o000040;
36: fetch = 16'o004737;
38: fetch = 16'o174130;
40: fetch = 16'o012701;
42: fetch = 16'o000010;
44: fetch = 16'o004737;
46: fetch = 16'o174130;
48: fetch = 16'o000137;
50: fetch = 16'o173752;
52: fetch = 16'o012701;
54: fetch = 16'o000007;
56: fetch = 16'o004737;
58: fetch = 16'o174130;
60: fetch = 16'o000137;
62: fetch = 16'o173752;
64: fetch = 16'o000240;
66: fetch = 16'o012700;
68: fetch = 16'o174160;
70: fetch = 16'o004737;
72: fetch = 16'o174114;
74: fetch = 16'o000207;
76: fetch = 16'o112001;
78: fetch = 16'o001403;
80: fetch = 16'o004737;
82: fetch = 16'o174130;
84: fetch = 16'o000773;
86: fetch = 16'o000207;
88: fetch = 16'o105737;
90: fetch = 16'o177564;
92: fetch = 16'o100375;
94: fetch = 16'o110137;
96: fetch = 16'o177566;
98: fetch = 16'o000207;
100: fetch = 16'o105737;
102: fetch = 16'o177560;
104: fetch = 16'o100375;
106: fetch = 16'o113701;
108: fetch = 16'o177562;
110: fetch = 16'o000207;
112: fetch = 16'o005015;
114: fetch = 16'o062510;
116: fetch = 16'o066154;
118: fetch = 16'o020157;
120: fetch = 16'o067567;
122: fetch = 16'o066162;
124: fetch = 16'o020544;
126: fetch = 16'o005015;
128: fetch = 16'o006400;
130: fetch = 16'o071012;
132: fetch = 16'o066557;
134: fetch = 16'o020076;
136: fetch = 16'o000000;
138: fetch = 16'o000000;
140: fetch = 16'o000000;
142: fetch = 16'o001400;
 `endif
	 
	 default: fetch = 16'o0;
       endcase // case(offset)

	  //#2 $display("rom fetch %o %o", iopage_addr, fetch);
	  
       end // if (iopage_rd)
     else
       fetch = 16'b0;
   
endmodule

