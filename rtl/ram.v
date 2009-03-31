//
// 4kx16 static ram
// with byte read/write capability
//
module ram_4kx16(A, DI, DO, CE_N, WE_N, BYTE_OP);

  input[15:0] A;
  input[15:0] DI;
  input CE_N, WE_N;
  input BYTE_OP;
  output[15:0] DO;

  reg[7:0] ram_h [0:16383];
  reg[7:0] ram_l [0:16383];
  integer i;

  initial
    begin
      for (i = 0; i < 16384; i=i+1)
	begin
           ram_h[i] = 7'b0;
	   ram_l[i] = 7'b0;
	end
    end

  wire BA = A[13:1];
   
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

//always @(A)
//  begin
//    $display("ram: ce %b, we %b [%o] -> %o", CE_N, WE_N, A, ram[A]);
//  end

   assign DO = { BYTE_OP ? 0 : ram_h[BA],
		 BYTE_OP ? (A[0] ? ram_l[BA] : ram_h[BA]) : ram_l[BA] };

endmodule

