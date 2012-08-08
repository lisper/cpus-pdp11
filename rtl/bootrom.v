// bootrom.v
// basic rk11 bootrom, residing at 1773000 (173000)

//`define boot_rk
//`define boot_tt
`define boot_kb
//`define boot_diag
//`define debug_bootrom

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
   
`ifdef debug_bootrom
   always @(posedge clk)
     if (iopage_rd)
       $display("bootrom: (clk) iopage_rd iopage_addr=%o, decode=%b", iopage_addr, decode);
   always @(decode or offset or iopage_addr or iopage_rd or data_out or fetch)
     if (iopage_rd)
       $display("bootrom: (comb) iopage_rd iopage_addr=%o, decode=%b", iopage_addr, decode);
`endif

   always @(decode or offset or iopage_addr or iopage_rd or data_out or fetch)
     if (iopage_rd && decode)
       begin
       case (offset)
`ifdef boot_rk
	 512+0: fetch = 16'o010000;	/* nop */
	 512+2: fetch = 16'o012706;	/* MOV #boot_start, SP */
	 512+4: fetch = 16'o002000;
	 512+6: fetch = 16'o012700;	/* MOV #unit, R0    ; unit number */
	 512+8: fetch = 16'o000000;
	 512+10: fetch = 16'o010003;	/* MOV R0, R3 */
	 512+12: fetch = 16'o000303;	/* SWAB R3 */
	 512+14: fetch = 16'o006303;	/* ASL R3 */
	 512+16: fetch = 16'o006303;	/* ASL R3 */
	 512+18: fetch = 16'o006303;	/* ASL R3 */
	 512+20: fetch = 16'o006303;	/* ASL R3 */
	 512+22: fetch = 16'o006303;	/* ASL R3 */
	 512+24: fetch = 16'o012701;	/* MOV #RKDA, R1    ; csr */
	 512+26: fetch = 16'o177412;
	 512+28: fetch = 16'o010311;	/* MOV R3, (R1)	 512+   ; load da */
	 512+30: fetch = 16'o005041;	/* CLR -(R1)	 512+   ; clear ba */
	 512+32: fetch = 16'o012741;	/* MOV #-256.*2, -(R1)  ; load wc */
	 512+34: fetch = 16'o177000;
	 512+36: fetch = 16'o012741;	/* MOV #READ+GO, -(R1)  ; read & go */ 
	 512+38: fetch = 16'o000005;
	 512+40: fetch = 16'o005002;	/* CLR R2 */
	 512+42: fetch = 16'o005003;	/* CLR R3 */
	 512+44: fetch = 16'o012704;	/* MOV #START+20, R4 */
	 512+46: fetch = 16'o002000+16'o020;
	 512+48: fetch = 16'o005005;	/* CLR R5 */
	 512+50: fetch = 16'o105711;	/* TSTB (R1) */
	 512+52: fetch = 16'o100376;	/* BPL .-2 */
	 512+54: fetch = 16'o105011;	/* CLRB (R1) */
	 512+56: fetch = 16'o005007;	/* CLR PC */
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

 `ifdef boot_kb
512: fetch = 16'o000240;
514: fetch = 16'o012706;
516: fetch = 16'o001000;
518: fetch = 16'o012700;
520: fetch = 16'o173100;
522: fetch = 16'o112001;
524: fetch = 16'o001403;
526: fetch = 16'o004737;
528: fetch = 16'o173050;
530: fetch = 16'o000773;
532: fetch = 16'o004737;
534: fetch = 16'o173064;
536: fetch = 16'o020127;
538: fetch = 16'o000003;
540: fetch = 16'o001404;
542: fetch = 16'o004737;
544: fetch = 16'o173050;
546: fetch = 16'o000137;
548: fetch = 16'o173024;
550: fetch = 16'o000000;
552: fetch = 16'o105737;
554: fetch = 16'o177564;
556: fetch = 16'o100375;
558: fetch = 16'o110137;
560: fetch = 16'o177566;
562: fetch = 16'o000207;
564: fetch = 16'o105737;
566: fetch = 16'o177560;
568: fetch = 16'o100375;
570: fetch = 16'o113701;
572: fetch = 16'o177562;
574: fetch = 16'o000207;
576: fetch = 16'o005015;
578: fetch = 16'o062510;
580: fetch = 16'o066154;
582: fetch = 16'o020157;
584: fetch = 16'o067567;
586: fetch = 16'o066162;
588: fetch = 16'o020544;
590: fetch = 16'o005015;
592: fetch = 16'o000000;
 `endif
	 
 `ifdef boot_diag
	 512: fetch = 16'o000240;
	 514: fetch = 16'o012706;
	 516: fetch = 16'o007000;
	 518: fetch = 16'o004737;
	 520: fetch = 16'o174114;
	 522: fetch = 16'o000137;
	 524: fetch = 16'o173022;
	 526: fetch = 16'o000000;
	 528: fetch = 16'o000000;
	 530: fetch = 16'o004737;
	 532: fetch = 16'o173724;
	 534: fetch = 16'o004737;
	 536: fetch = 16'o173762;
	 538: fetch = 16'o012705;
	 540: fetch = 16'o006000;
	 542: fetch = 16'o122715;
	 544: fetch = 16'o000162;
	 546: fetch = 16'o001521;
	 548: fetch = 16'o122715;
	 550: fetch = 16'o000150;
	 552: fetch = 16'o001421;
	 554: fetch = 16'o122715;
	 556: fetch = 16'o000144;
	 558: fetch = 16'o001417;
	 560: fetch = 16'o122715;
	 562: fetch = 16'o000145;
	 564: fetch = 16'o001447;
	 566: fetch = 16'o122715;
	 568: fetch = 16'o000147;
	 570: fetch = 16'o001474;
	 572: fetch = 16'o122715;
	 574: fetch = 16'o000151;
	 576: fetch = 16'o001532;
	 578: fetch = 16'o122715;
	 580: fetch = 16'o000170;
	 582: fetch = 16'o001565;
	 584: fetch = 16'o000137;
	 586: fetch = 16'o173022;
	 588: fetch = 16'o000000;
	 590: fetch = 16'o004737;
	 592: fetch = 16'o173736;
	 594: fetch = 16'o062705;
	 596: fetch = 16'o000002;
	 598: fetch = 16'o010501;
	 600: fetch = 16'o004737;
	 602: fetch = 16'o173510;
	 604: fetch = 16'o010004;
	 606: fetch = 16'o010401;
	 608: fetch = 16'o004737;
	 610: fetch = 16'o173560;
	 612: fetch = 16'o112701;
	 614: fetch = 16'o000072;
	 616: fetch = 16'o004737;
	 618: fetch = 16'o174234;
	 620: fetch = 16'o004737;
	 622: fetch = 16'o173750;
	 624: fetch = 16'o012702;
	 626: fetch = 16'o000010;
	 628: fetch = 16'o012401;
	 630: fetch = 16'o004737;
	 632: fetch = 16'o173546;
	 634: fetch = 16'o077204;
	 636: fetch = 16'o004737;
	 638: fetch = 16'o173736;
	 640: fetch = 16'o000137;
	 642: fetch = 16'o173022;
	 644: fetch = 16'o004737;
	 646: fetch = 16'o173736;
	 648: fetch = 16'o062705;
	 650: fetch = 16'o000002;
	 652: fetch = 16'o010501;
	 654: fetch = 16'o004737;
	 656: fetch = 16'o173510;
	 658: fetch = 16'o010004;
	 660: fetch = 16'o010401;
	 662: fetch = 16'o004737;
	 664: fetch = 16'o173560;
	 666: fetch = 16'o112701;
	 668: fetch = 16'o000072;
	 670: fetch = 16'o004737;
	 672: fetch = 16'o174234;
	 674: fetch = 16'o004737;
	 676: fetch = 16'o173750;
	 678: fetch = 16'o012401;
	 680: fetch = 16'o004737;
	 682: fetch = 16'o173546;
	 684: fetch = 16'o004737;
	 686: fetch = 16'o173736;
	 688: fetch = 16'o000137;
	 690: fetch = 16'o173022;
	 692: fetch = 16'o004737;
	 694: fetch = 16'o173736;
	 696: fetch = 16'o062705;
	 698: fetch = 16'o000002;
	 700: fetch = 16'o010501;
	 702: fetch = 16'o004737;
	 704: fetch = 16'o173510;
	 706: fetch = 16'o010004;
	 708: fetch = 16'o000104;
	 710: fetch = 16'o012700;
	 712: fetch = 16'o000000;
	 714: fetch = 16'o010003;
	 716: fetch = 16'o000303;
	 718: fetch = 16'o006303;
	 720: fetch = 16'o006303;
	 722: fetch = 16'o006303;
	 724: fetch = 16'o006303;
	 726: fetch = 16'o006303;
	 728: fetch = 16'o012701;
	 730: fetch = 16'o177412;
	 732: fetch = 16'o010311;
	 734: fetch = 16'o005041;
	 736: fetch = 16'o012741;
	 738: fetch = 16'o177000;
	 740: fetch = 16'o012741;
	 742: fetch = 16'o000005;
	 744: fetch = 16'o105711;
	 746: fetch = 16'o100376;
	 748: fetch = 16'o105011;
	 750: fetch = 16'o004737;
	 752: fetch = 16'o173736;
	 754: fetch = 16'o000137;
	 756: fetch = 16'o173022;
	 758: fetch = 16'o012702;
	 760: fetch = 16'o000020;
	 762: fetch = 16'o012705;
	 764: fetch = 16'o000010;
	 766: fetch = 16'o004737;
	 768: fetch = 16'o173736;
	 770: fetch = 16'o010201;
	 772: fetch = 16'o004737;
	 774: fetch = 16'o173560;
	 776: fetch = 16'o004737;
	 778: fetch = 16'o173750;
	 780: fetch = 16'o012704;
	 782: fetch = 16'o177414;
	 784: fetch = 16'o010224;
	 786: fetch = 16'o012703;
	 788: fetch = 16'o177404;
	 790: fetch = 16'o012713;
	 792: fetch = 16'o000013;
	 794: fetch = 16'o105713;
	 796: fetch = 16'o100376;
	 798: fetch = 16'o105013;
	 800: fetch = 16'o011401;
	 802: fetch = 16'o004737;
	 804: fetch = 16'o173560;
	 806: fetch = 16'o004737;
	 808: fetch = 16'o173736;
	 810: fetch = 16'o005202;
	 812: fetch = 16'o077526;
	 814: fetch = 16'o000137;
	 816: fetch = 16'o173022;
	 818: fetch = 16'o012703;
	 820: fetch = 16'o177404;
	 822: fetch = 16'o012713;
	 824: fetch = 16'o000001;
	 826: fetch = 16'o000240;
	 828: fetch = 16'o000240;
	 830: fetch = 16'o105013;
	 832: fetch = 16'o004737;
	 834: fetch = 16'o173736;
	 836: fetch = 16'o000137;
	 838: fetch = 16'o173022;
	 840: fetch = 16'o010246;
	 842: fetch = 16'o005002;
	 844: fetch = 16'o112501;
	 846: fetch = 16'o001410;
	 848: fetch = 16'o042701;
	 850: fetch = 16'o177770;
	 852: fetch = 16'o006302;
	 854: fetch = 16'o006302;
	 856: fetch = 16'o006302;
	 858: fetch = 16'o050102;
	 860: fetch = 16'o000137;
	 862: fetch = 16'o173514;
	 864: fetch = 16'o010200;
	 866: fetch = 16'o012602;
	 868: fetch = 16'o000207;
	 870: fetch = 16'o004737;
	 872: fetch = 16'o173560;
	 874: fetch = 16'o004737;
	 876: fetch = 16'o173750;
	 878: fetch = 16'o000207;
	 880: fetch = 16'o010246;
	 882: fetch = 16'o010346;
	 884: fetch = 16'o012703;
	 886: fetch = 16'o010000;
	 888: fetch = 16'o004737;
	 890: fetch = 16'o173672;
	 892: fetch = 16'o004737;
	 894: fetch = 16'o173672;
	 896: fetch = 16'o004737;
	 898: fetch = 16'o173672;
	 900: fetch = 16'o004737;
	 902: fetch = 16'o173672;
	 904: fetch = 16'o004737;
	 906: fetch = 16'o173672;
	 908: fetch = 16'o004737;
	 910: fetch = 16'o173672;
	 912: fetch = 16'o114301;
	 914: fetch = 16'o004737;
	 916: fetch = 16'o174234;
	 918: fetch = 16'o114301;
	 920: fetch = 16'o004737;
	 922: fetch = 16'o174234;
	 924: fetch = 16'o114301;
	 926: fetch = 16'o004737;
	 928: fetch = 16'o174234;
	 930: fetch = 16'o114301;
	 932: fetch = 16'o004737;
	 934: fetch = 16'o174234;
	 936: fetch = 16'o114301;
	 938: fetch = 16'o004737;
	 940: fetch = 16'o174234;
	 942: fetch = 16'o114301;
	 944: fetch = 16'o004737;
	 946: fetch = 16'o174234;
	 948: fetch = 16'o012603;
	 950: fetch = 16'o012602;
	 952: fetch = 16'o000207;
	 954: fetch = 16'o010102;
	 956: fetch = 16'o042702;
	 958: fetch = 16'o177770;
	 960: fetch = 16'o062702;
	 962: fetch = 16'o000060;
	 964: fetch = 16'o110223;
	 966: fetch = 16'o042701;
	 968: fetch = 16'o000007;
	 970: fetch = 16'o000241;
	 972: fetch = 16'o006001;
	 974: fetch = 16'o006001;
	 976: fetch = 16'o006001;
	 978: fetch = 16'o000207;
	 980: fetch = 16'o012700;
	 982: fetch = 16'o174305;
	 984: fetch = 16'o004737;
	 986: fetch = 16'o174220;
	 988: fetch = 16'o000207;
	 990: fetch = 16'o012700;
	 992: fetch = 16'o174302;
	 994: fetch = 16'o004737;
	 996: fetch = 16'o174220;
	 998: fetch = 16'o000207;
	 1000: fetch = 16'o112701;
	 1002: fetch = 16'o000040;
	 1004: fetch = 16'o004737;
	 1006: fetch = 16'o174234;
	 1008: fetch = 16'o000207;
	 1010: fetch = 16'o012705;
	 1012: fetch = 16'o006000;
	 1014: fetch = 16'o004737;
	 1016: fetch = 16'o174250;
	 1018: fetch = 16'o022701;
	 1020: fetch = 16'o000015;
	 1022: fetch = 16'o001410;
	 0: fetch = 16'o022701;
	 2: fetch = 16'o000177;
	 4: fetch = 16'o001412;
	 6: fetch = 16'o004737;
	 8: fetch = 16'o174234;
	 10: fetch = 16'o110125;
	 12: fetch = 16'o000137;
	 14: fetch = 16'o173766;
	 16: fetch = 16'o112725;
	 18: fetch = 16'o000000;
	 20: fetch = 16'o112725;
	 22: fetch = 16'o000000;
	 24: fetch = 16'o000207;
	 26: fetch = 16'o022705;
	 28: fetch = 16'o006000;
	 30: fetch = 16'o001420;
	 32: fetch = 16'o162705;
	 34: fetch = 16'o000001;
	 36: fetch = 16'o012701;
	 38: fetch = 16'o000010;
	 40: fetch = 16'o004737;
	 42: fetch = 16'o174234;
	 44: fetch = 16'o012701;
	 46: fetch = 16'o000040;
	 48: fetch = 16'o004737;
	 50: fetch = 16'o174234;
	 52: fetch = 16'o012701;
	 54: fetch = 16'o000010;
	 56: fetch = 16'o004737;
	 58: fetch = 16'o174234;
	 60: fetch = 16'o000137;
	 62: fetch = 16'o173766;
	 64: fetch = 16'o012701;
	 66: fetch = 16'o000007;
	 68: fetch = 16'o004737;
	 70: fetch = 16'o174234;
	 72: fetch = 16'o000137;
	 74: fetch = 16'o173766;
	 76: fetch = 16'o000240;
	 78: fetch = 16'o005000;
	 80: fetch = 16'o012701;
	 82: fetch = 16'o001234;
	 84: fetch = 16'o010110;
	 86: fetch = 16'o012700;
	 88: fetch = 16'o002000;
	 90: fetch = 16'o012701;
	 92: fetch = 16'o004321;
	 94: fetch = 16'o010110;
	 96: fetch = 16'o000240;
	 98: fetch = 16'o005000;
	 100: fetch = 16'o011002;
	 102: fetch = 16'o012701;
	 104: fetch = 16'o001234;
	 106: fetch = 16'o020102;
	 108: fetch = 16'o001015;
	 110: fetch = 16'o000240;
	 112: fetch = 16'o012700;
	 114: fetch = 16'o002000;
	 116: fetch = 16'o011002;
	 118: fetch = 16'o012701;
	 120: fetch = 16'o004321;
	 122: fetch = 16'o020102;
	 124: fetch = 16'o001007;
	 126: fetch = 16'o012700;
	 128: fetch = 16'o174264;
	 130: fetch = 16'o004737;
	 132: fetch = 16'o174220;
	 134: fetch = 16'o000207;
	 136: fetch = 16'o000137;
	 138: fetch = 16'o173016;
	 140: fetch = 16'o000137;
	 142: fetch = 16'o173020;
	 144: fetch = 16'o112001;
	 146: fetch = 16'o001403;
	 148: fetch = 16'o004737;
	 150: fetch = 16'o174234;
	 152: fetch = 16'o000773;
	 154: fetch = 16'o000207;
	 156: fetch = 16'o105737;
	 158: fetch = 16'o177564;
	 160: fetch = 16'o100375;
	 162: fetch = 16'o110137;
	 164: fetch = 16'o177566;
	 166: fetch = 16'o000207;
	 168: fetch = 16'o105737;
	 170: fetch = 16'o177560;
	 172: fetch = 16'o100375;
	 174: fetch = 16'o113701;
	 176: fetch = 16'o177562;
	 178: fetch = 16'o000207;
	 180: fetch = 16'o005015;
	 182: fetch = 16'o062510;
	 184: fetch = 16'o066154;
	 186: fetch = 16'o020157;
	 188: fetch = 16'o067567;
	 190: fetch = 16'o066162;
	 192: fetch = 16'o020544;
	 194: fetch = 16'o005015;
	 196: fetch = 16'o006400;
	 198: fetch = 16'o071012;
	 200: fetch = 16'o066557;
	 202: fetch = 16'o020076;
	 204: fetch = 16'o012400;
 `endif
	 
	 default: fetch = 16'o0;
       endcase // case(offset)

`ifdef debug_bootrom
	  #2 $display("bootrom: rom fetch %o (offset %o) %o", iopage_addr, offset, fetch);
`endif
	  
       end // if (iopage_rd)
     else
       fetch = 16'b0;
   
endmodule

