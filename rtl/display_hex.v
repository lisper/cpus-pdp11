// display_hex.v

module display_hex(clk, reset, hex, dots, sevenseg, sevenseg_an);
   
    input 	clk;
    input 	reset;
    input [15:0] hex;
    input [3:0]  dots;
    output [7:0] sevenseg;
    output [3:0] sevenseg_an;

   //
   wire [3:0] 	 digit;
   reg [1:0] 	 anode;

   reg [10:0]    divider;
   reg           aclk;

   assign digit = (anode == 2'b11) ? hex[15:12] :
		  (anode == 2'b10) ? hex[11:8] :
		  (anode == 2'b01) ? hex[7:4] :
		  (anode == 2'b00) ? hex[3:0] :
		  4'b0;

   assign sevenseg_an = (anode == 2'b11) ? 4'b0111 :
			(anode == 2'b10) ? 4'b1011 :
			(anode == 2'b01) ? 4'b1101 :
			(anode == 2'b00) ? 4'b1110 :
			4'b1111;

   assign sevenseg[0] = ~dots[anode];
   
   sevensegdecode decode(digit, sevenseg[7:1]);

   always @(posedge clk)
     begin
       divider <= divider + 1'b1;
       if (divider == 0)
          aclk = ~aclk;
     end

   // digit scan clock
   always @(posedge aclk)
       anode <= anode + 1'b1;

endmodule
