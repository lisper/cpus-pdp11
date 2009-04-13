//
// 4kx16 static ram
// with byte read/write capability
//

module ram_16kx8(clk, reset, a, di, do, ce, we);

   input clk;
   input reset;
   input [14/*12*/:0] a;
   input [7:0] di;
   input ce, we;
   output [7:0] do;

   reg [7:0] ram[32767/*8191*/:0];

   assign do = ram[a];

   always @(posedge clk)
     begin
	if (we && ce)
	  begin
	     //$display("ram8 write %o %o", a, di);
	     ram[a] = di;
	  end
     end
endmodule

module ram_16kx16(CLK, RESET, A, DI, DO, CE_N, WE_N, BYTE_OP);

   input CLK;
   input RESET;
   input [15:0] A;
   input [15:0] DI;
   input 	CE_N, WE_N;
   input 	BYTE_OP;
   output [15:0] DO;

   wire [7:0] do_h, do_l;
   wire [7:0] di_h, di_l;
   wire we_h, we_l;

   wire [14/*12*/:0] 	 BA;

   ram_16kx8 ram_h(CLK, RESET, BA, di_h, do_h, ~CE_N, we_h);
   ram_16kx8 ram_l(CLK, RESET, BA, di_l, do_l, ~CE_N, we_l);

   // synthesis translate_off
   integer 	 i;
   reg [15:0] 	 v;
   integer 	 file;
   reg [1023:0]  str;
   reg [1023:0]  testfilename;
   integer 	 n;

   initial
     begin
	for (i = 0; i < 32768/*8192*/; i=i+1)
	  begin
             ram_h.ram[i] = 7'b0;
	     ram_l.ram[i] = 7'b0;
	  end

 	n = $scan$plusargs("test=", testfilename);
	if (n > 0)
	  begin
	     $display("ram: code filename: %s", testfilename);
	     file = $fopen(testfilename, "r");

	     while ($fscanf(file, "%o %o", i, v) > 0)
	       begin
		  $display("ram[%o] <- %o", i, v);
		  ram_h.ram[i/2] = v[15:8];
		  ram_l.ram[i/2] = v[7:0];
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

		  ram_h.ram[14'o0500/2 + i] = v[15:8];
		  ram_l.ram[14'o0500/2 + i] = v[7:0];
	       end
	  end
     end

   always @(A or CE_N or WE_N or DO)
     begin
	if (CE_N == 0 && WE_N == 1)
	$display("ram: ce_n %b, we_n %b [%o] -> %o", CE_N, WE_N, A, DO);
     end

   always @(posedge CLK)
     if (WE_N == 0 && CE_N == 0)
       $display("ram: write [%o] <- %o", A, DI);
   // synthesis translate_on

   assign BA = A[15/*13*/:1];

   assign DO = { BYTE_OP ? 8'b0 : do_h,
		 BYTE_OP ? (A[0] ? do_h : do_l) : do_l };
   
   assign we_h = ~WE_N && (~BYTE_OP | (BYTE_OP & A[0]));
   assign we_l = ~WE_N && (~BYTE_OP | (BYTE_OP & ~A[0]));

   assign di_h = ~BYTE_OP ? DI[15:8] : DI[7:0];
   assign di_l = DI[7:0];

endmodule

