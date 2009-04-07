//
// 4kx16 static ram
// with byte read/write capability
//
module ram_4kx16(A, DI, DO, CE_N, WE_N, BYTE_OP);

   input[15:0] A;
   input [15:0] DI;
   input 	CE_N, WE_N;
   input 	BYTE_OP;
   output [15:0] DO;

   reg [7:0] 	 ram_h [0:16383];
   reg [7:0] 	 ram_l [0:16383];
   integer 	 i;

   reg [15:0] 	 v;
   wire [12:0] 	 BA;

   integer 	 file;
   reg [1023:0]  str;
   reg [1023:0]  testfilename;
   integer 	 n;
   
   initial
     begin
	for (i = 0; i < 16384; i=i+1)
	  begin
             ram_h[i] = 7'b0;
	     ram_l[i] = 7'b0;
	  end

`ifdef CANNED
	for (i = 0; i < 20; i=i+ 1)
	  begin
	     case (i)
`ifdef TEST0
	       0: v = 16'o0010101;
	       1: v = 16'o0010101;
	       2: v = 16'o0000000;
`endif
`ifdef TEST2
	       0: v = 16'o0012706;
	       1: v = 16'o0000500;
	       2: v = 16'o0012701;
	       3: v = 16'o0000700;
	       4: v = 16'o0012702;
	       5: v = 16'o0000712;
	       6: v = 16'o0005201;
	       7: v = 16'o0005201;
	       8: v = 16'o0005211;
	       9: v = 16'o0005221;
	       10: v = 16'o0005231;
	       11: v = 16'o0005241;
	       12: v = 16'o0005251;
	       13: v = 16'o0005261;
	       14: v = 16'o0000002;
	       15: v = 16'o0005271;
	       16: v = 16'o0000004;
	       17: v = 16'o0000000;
`endif
	       default:  v = 16'o0000000;
	       
	     endcase // case(i)
	     ram_h[14'o0500/2 + i] = v[15:8];
	     ram_l[14'o0500/2 + i] = v[7:0];
	  end
`endif

 	n = $scan$plusargs("test=", testfilename);

	$display("ram: code filename: %s", testfilename);

//	file = $fopen("test2.mem", "r");
	file = $fopen(testfilename, "r");

	while ($fscanf(file, "%o %o", i, v) > 0)
	  begin
	     $display("ram[%o] <- %o", i, v);
	     ram_h[i/2] = v[15:8];
	     ram_l[i/2] = v[7:0];
	  end

	$fclose(file);
     end

   assign BA = A[13:1];
   
   always @(negedge WE_N)
     begin
	if (CE_N == 0)
          begin
	     $display("ram: write [%o] <- %o", A, DI);
	     if (BYTE_OP)
	       begin
		  if (A[0])
		    ram_h[ BA ] = DI[7:0];
		  else
		    ram_l[ BA ] = DI[7:0];
	       end
	     else
	       begin
		  ram_h[ BA ] = DI[15:8];
		  ram_l[ BA ] = DI[7:0];
	       end
          end
     end

   assign DO = { BYTE_OP ? 8'b0 : ram_h[BA],
		 BYTE_OP ? (A[0] ? ram_l[BA] : ram_h[BA]) : ram_l[BA] };

   always @(A or CE_N or WE_N or DO)
     begin
	if (WE_N)
	$display("ram: ce %b, we %b [%o] -> %o", CE_N, WE_N, A, DO);
     end

endmodule

