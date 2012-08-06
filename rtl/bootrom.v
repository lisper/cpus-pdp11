// bootrom.v
// basic rk11 bootrom, residing at 1773000 (173000)

`define boot_rk
//`define boot_tt
//`define boot_diag
`define debug_bootrom

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

 `ifdef boot_diag
	 512: fetch = 16'o000240;
	 514: fetch = 16'o012706;
	 516: fetch = 16'o007000;
	 518: fetch = 16'o004737;
	 520: fetch = 16'o174110;
	 522: fetch = 16'o000137;
	 524: fetch = 16'o173022;
	 526: fetch = 16'o000000;
	 528: fetch = 16'o000000;
	 530: fetch = 16'o004737;
	 532: fetch = 16'o173720;
	 534: fetch = 16'o004737;
	 536: fetch = 16'o173756;
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
	 582: fetch = 16'o001563;
	 584: fetch = 16'o000137;
	 586: fetch = 16'o173022;
	 588: fetch = 16'o000000;
	 590: fetch = 16'o004737;
	 592: fetch = 16'o173732;
	 594: fetch = 16'o062705;
	 596: fetch = 16'o000002;
	 598: fetch = 16'o010501;
	 600: fetch = 16'o004737;
	 602: fetch = 16'o173504;
	 604: fetch = 16'o010004;
	 606: fetch = 16'o010401;
	 608: fetch = 16'o004737;
	 610: fetch = 16'o173554;
	 612: fetch = 16'o112701;
	 614: fetch = 16'o000072;
	 616: fetch = 16'o004737;
	 618: fetch = 16'o174230;
	 620: fetch = 16'o004737;
	 622: fetch = 16'o173744;
	 624: fetch = 16'o012702;
	 626: fetch = 16'o000010;
	 628: fetch = 16'o012401;
	 630: fetch = 16'o004737;
	 632: fetch = 16'o173542;
	 634: fetch = 16'o077204;
	 636: fetch = 16'o004737;
	 638: fetch = 16'o173732;
	 640: fetch = 16'o000137;
	 642: fetch = 16'o173022;
	 644: fetch = 16'o004737;
	 646: fetch = 16'o173732;
	 648: fetch = 16'o062705;
	 650: fetch = 16'o000002;
	 652: fetch = 16'o010501;
	 654: fetch = 16'o004737;
	 656: fetch = 16'o173504;
	 658: fetch = 16'o010004;
	 660: fetch = 16'o010401;
	 662: fetch = 16'o004737;
	 664: fetch = 16'o173554;
	 666: fetch = 16'o112701;
	 668: fetch = 16'o000072;
	 670: fetch = 16'o004737;
	 672: fetch = 16'o174230;
	 674: fetch = 16'o004737;
	 676: fetch = 16'o173744;
	 678: fetch = 16'o012401;
	 680: fetch = 16'o004737;
	 682: fetch = 16'o173542;
	 684: fetch = 16'o004737;
	 686: fetch = 16'o173732;
	 688: fetch = 16'o000137;
	 690: fetch = 16'o173022;
	 692: fetch = 16'o004737;
	 694: fetch = 16'o173732;
	 696: fetch = 16'o062705;
	 698: fetch = 16'o000002;
	 700: fetch = 16'o010501;
	 702: fetch = 16'o004737;
	 704: fetch = 16'o173504;
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
	 752: fetch = 16'o173732;
	 754: fetch = 16'o000137;
	 756: fetch = 16'o173022;
	 758: fetch = 16'o005002;
	 760: fetch = 16'o012705;
	 762: fetch = 16'o000010;
	 764: fetch = 16'o004737;
	 766: fetch = 16'o173732;
	 768: fetch = 16'o010201;
	 770: fetch = 16'o004737;
	 772: fetch = 16'o173554;
	 774: fetch = 16'o004737;
	 776: fetch = 16'o173744;
	 778: fetch = 16'o012704;
	 780: fetch = 16'o177414;
	 782: fetch = 16'o010224;
	 784: fetch = 16'o012703;
	 786: fetch = 16'o177404;
	 788: fetch = 16'o012713;
	 790: fetch = 16'o000013;
	 792: fetch = 16'o105713;
	 794: fetch = 16'o100376;
	 796: fetch = 16'o105013;
	 798: fetch = 16'o011401;
	 800: fetch = 16'o004737;
	 802: fetch = 16'o173554;
	 804: fetch = 16'o004737;
	 806: fetch = 16'o173732;
	 808: fetch = 16'o077525;
	 810: fetch = 16'o000137;
	 812: fetch = 16'o173022;
	 814: fetch = 16'o012703;
	 816: fetch = 16'o177404;
	 818: fetch = 16'o012713;
	 820: fetch = 16'o000001;
	 822: fetch = 16'o000240;
	 824: fetch = 16'o000240;
	 826: fetch = 16'o105013;
	 828: fetch = 16'o004737;
	 830: fetch = 16'o173732;
	 832: fetch = 16'o000137;
	 834: fetch = 16'o173022;
	 836: fetch = 16'o010246;
	 838: fetch = 16'o005002;
	 840: fetch = 16'o112501;
	 842: fetch = 16'o001410;
	 844: fetch = 16'o042701;
	 846: fetch = 16'o177770;
	 848: fetch = 16'o006302;
	 850: fetch = 16'o006302;
	 852: fetch = 16'o006302;
	 854: fetch = 16'o050102;
	 856: fetch = 16'o000137;
	 858: fetch = 16'o173510;
	 860: fetch = 16'o010200;
	 862: fetch = 16'o012602;
	 864: fetch = 16'o000207;
	 866: fetch = 16'o004737;
	 868: fetch = 16'o173554;
	 870: fetch = 16'o004737;
	 872: fetch = 16'o173744;
	 874: fetch = 16'o000207;
	 876: fetch = 16'o010246;
	 878: fetch = 16'o010346;
	 880: fetch = 16'o012703;
	 882: fetch = 16'o174311;
	 884: fetch = 16'o004737;
	 886: fetch = 16'o173666;
	 888: fetch = 16'o004737;
	 890: fetch = 16'o173666;
	 892: fetch = 16'o004737;
	 894: fetch = 16'o173666;
	 896: fetch = 16'o004737;
	 898: fetch = 16'o173666;
	 900: fetch = 16'o004737;
	 902: fetch = 16'o173666;
	 904: fetch = 16'o004737;
	 906: fetch = 16'o173666;
	 908: fetch = 16'o114301;
	 910: fetch = 16'o004737;
	 912: fetch = 16'o174230;
	 914: fetch = 16'o114301;
	 916: fetch = 16'o004737;
	 918: fetch = 16'o174230;
	 920: fetch = 16'o114301;
	 922: fetch = 16'o004737;
	 924: fetch = 16'o174230;
	 926: fetch = 16'o114301;
	 928: fetch = 16'o004737;
	 930: fetch = 16'o174230;
	 932: fetch = 16'o114301;
	 934: fetch = 16'o004737;
	 936: fetch = 16'o174230;
	 938: fetch = 16'o114301;
	 940: fetch = 16'o004737;
	 942: fetch = 16'o174230;
	 944: fetch = 16'o012603;
	 946: fetch = 16'o012602;
	 948: fetch = 16'o000207;
	 950: fetch = 16'o010102;
	 952: fetch = 16'o042702;
	 954: fetch = 16'o177770;
	 956: fetch = 16'o062702;
	 958: fetch = 16'o000060;
	 960: fetch = 16'o110223;
	 962: fetch = 16'o042701;
	 964: fetch = 16'o000007;
	 966: fetch = 16'o000241;
	 968: fetch = 16'o006001;
	 970: fetch = 16'o006001;
	 972: fetch = 16'o006001;
	 974: fetch = 16'o000207;
	 976: fetch = 16'o012700;
	 978: fetch = 16'o174301;
	 980: fetch = 16'o004737;
	 982: fetch = 16'o174214;
	 984: fetch = 16'o000207;
	 986: fetch = 16'o012700;
	 988: fetch = 16'o174276;
	 990: fetch = 16'o004737;
	 992: fetch = 16'o174214;
	 994: fetch = 16'o000207;
	 996: fetch = 16'o112701;
	 998: fetch = 16'o000040;
	 1000: fetch = 16'o004737;
	 1002: fetch = 16'o174230;
	 1004: fetch = 16'o000207;
	 1006: fetch = 16'o012705;
	 1008: fetch = 16'o006000;
	 1010: fetch = 16'o004737;
	 1012: fetch = 16'o174244;
	 1014: fetch = 16'o022701;
	 1016: fetch = 16'o000015;
	 1018: fetch = 16'o001410;
	 1020: fetch = 16'o022701;
	 1022: fetch = 16'o000177;
	 0: fetch = 16'o001412;
	 2: fetch = 16'o004737;
	 4: fetch = 16'o174230;
	 6: fetch = 16'o110125;
	 8: fetch = 16'o000137;
	 10: fetch = 16'o173762;
	 12: fetch = 16'o112725;
	 14: fetch = 16'o000000;
	 16: fetch = 16'o112725;
	 18: fetch = 16'o000000;
	 20: fetch = 16'o000207;
	 22: fetch = 16'o022705;
	 24: fetch = 16'o006000;
	 26: fetch = 16'o001420;
	 28: fetch = 16'o162705;
	 30: fetch = 16'o000001;
	 32: fetch = 16'o012701;
	 34: fetch = 16'o000010;
	 36: fetch = 16'o004737;
	 38: fetch = 16'o174230;
	 40: fetch = 16'o012701;
	 42: fetch = 16'o000040;
	 44: fetch = 16'o004737;
	 46: fetch = 16'o174230;
	 48: fetch = 16'o012701;
	 50: fetch = 16'o000010;
	 52: fetch = 16'o004737;
	 54: fetch = 16'o174230;
	 56: fetch = 16'o000137;
	 58: fetch = 16'o173762;
	 60: fetch = 16'o012701;
	 62: fetch = 16'o000007;
	 64: fetch = 16'o004737;
	 66: fetch = 16'o174230;
	 68: fetch = 16'o000137;
	 70: fetch = 16'o173762;
	 72: fetch = 16'o000240;
	 74: fetch = 16'o005000;
	 76: fetch = 16'o012701;
	 78: fetch = 16'o001234;
	 80: fetch = 16'o010110;
	 82: fetch = 16'o012700;
	 84: fetch = 16'o002000;
	 86: fetch = 16'o012701;
	 88: fetch = 16'o004321;
	 90: fetch = 16'o010110;
	 92: fetch = 16'o000240;
	 94: fetch = 16'o005000;
	 96: fetch = 16'o011002;
	 98: fetch = 16'o012701;
	 100: fetch = 16'o001234;
	 102: fetch = 16'o020102;
	 104: fetch = 16'o001015;
	 106: fetch = 16'o000240;
	 108: fetch = 16'o012700;
	 110: fetch = 16'o002000;
	 112: fetch = 16'o011002;
	 114: fetch = 16'o012701;
	 116: fetch = 16'o004321;
	 118: fetch = 16'o020102;
	 120: fetch = 16'o001007;
	 122: fetch = 16'o012700;
	 124: fetch = 16'o174260;
	 126: fetch = 16'o004737;
	 128: fetch = 16'o174214;
	 130: fetch = 16'o000207;
	 132: fetch = 16'o000137;
	 134: fetch = 16'o173016;
	 136: fetch = 16'o000137;
	 138: fetch = 16'o173020;
	 140: fetch = 16'o112001;
	 142: fetch = 16'o001403;
	 144: fetch = 16'o004737;
	 146: fetch = 16'o174230;
	 148: fetch = 16'o000773;
	 150: fetch = 16'o000207;
	 152: fetch = 16'o105737;
	 154: fetch = 16'o177564;
	 156: fetch = 16'o100375;
	 158: fetch = 16'o110137;
	 160: fetch = 16'o177566;
	 162: fetch = 16'o000207;
	 164: fetch = 16'o105737;
	 166: fetch = 16'o177560;
	 168: fetch = 16'o100375;
	 170: fetch = 16'o113701;
	 172: fetch = 16'o177562;
	 174: fetch = 16'o000207;
	 176: fetch = 16'o005015;
	 178: fetch = 16'o062510;
	 180: fetch = 16'o066154;
	 182: fetch = 16'o020157;
	 184: fetch = 16'o067567;
	 186: fetch = 16'o066162;
	 188: fetch = 16'o020544;
	 190: fetch = 16'o005015;
	 192: fetch = 16'o006400;
	 194: fetch = 16'o071012;
	 196: fetch = 16'o066557;
	 198: fetch = 16'o020076;
	 200: fetch = 16'o000000;
	 202: fetch = 16'o000000;
	 204: fetch = 16'o000000;
	 206: fetch = 16'o012400;
 `endif
	 
	 default: fetch = 16'o0;
       endcase // case(offset)

`ifdef debug_bootrom
	  #2 $display("rom fetch %o (offset %o) %o", iopage_addr, offset, fetch);
`endif
	  
       end // if (iopage_rd)
     else
       fetch = 16'b0;
   
endmodule

