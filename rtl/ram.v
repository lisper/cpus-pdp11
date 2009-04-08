//
// 4kx16 static ram
// with byte read/write capability
//
module ram_16kx16(CLK, RESET, A, DI, DO, CE_N, WE_N, BYTE_OP);

   input CLK;
   input RESET;
   input[15:0] A;
   input [15:0] DI;
   input 	CE_N, WE_N;
   input 	BYTE_OP;
   output [15:0] DO;

   reg [7:0] 	 ram_h [0:8191];
   reg [7:0] 	 ram_l [0:8191];

   wire [12:0] 	 BA;

   // synthesis translate_off
   integer 	 i;
   reg [15:0] 	 v;
   integer 	 file;
   reg [1023:0]  str;
   reg [1023:0]  testfilename;
   integer 	 n;

   initial
     begin
	for (i = 0; i < 8192; i=i+1)
	  begin
             ram_h[i] = 7'b0;
	     ram_l[i] = 7'b0;
	  end

 	n = $scan$plusargs("test=", testfilename);
	if (n > 0)
	  begin
	     $display("ram: code filename: %s", testfilename);
	     file = $fopen(testfilename, "r");

	     while ($fscanf(file, "%o %o", i, v) > 0)
	       begin
		  $display("ram[%o] <- %o", i, v);
		  ram_h[i/2] = v[15:8];
		  ram_l[i/2] = v[7:0];
	       end

	     $fclose(file);
	  end
	else
	  begin
	     for (i = 0; i < 20; i=i+ 1)
	       begin
		  case (i)
		    0: v = 16'o012706;
		    1: v = 16'o000500;
		    2: v = 16'o012701;
		    3: v = 16'o000700;
		    4: v = 16'o012702;
		    5: v = 16'o000712;
		    6: v = 16'o005201;
		    7: v = 16'o005211;
		    default:  v = 16'o000000;
		  endcase

		  ram_h[14'o0500/2 + i] = v[15:8];
		  ram_l[14'o0500/2 + i] = v[7:0];
	       end
	  end
     end
   // synthesis translate_on

   assign BA = A[13:1];

   // synchronous write
   always @(posedge CLK)
     begin
	if (WE_N == 0 && CE_N == 0)
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

   // synthesis translate_off
   always @(A or CE_N or WE_N or DO)
     begin
	if (CE_N == 0 && WE_N == 1)
	$display("ram: ce_n %b, we_n %b [%o] -> %o", CE_N, WE_N, A, DO);
     end
   // synthesis translate_on

endmodule

