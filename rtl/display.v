// display.c
// display pc on led'and 4x7 segment digits

`include "sevenseg.v"

module display(clk, reset, pc, led, sevenseg, sevenseg_an);
   
    input 	clk;
    input 	reset;
    input [15:0] pc;
    output [7:0] led;
    output [7:0] sevenseg;
    output [3:0] sevenseg_an;

   //
   wire [3:0] 	 digit;
   reg 		 dig_clk;
   reg [3:0] 	 anode;
   reg [16:0] 	 counter;
   
   assign 	 led = {4'b0, pc[15:12]};
   assign 	 sevenseg_an = anode;
   
   assign digit = (anode == 4'b1000) ? pc[11:9] :
		  (anode == 4'b0100) ? pc[8:6] :
		  (anode == 4'b0010) ? pc[5:3] :
		  (anode == 4'b0001) ? pc[2:0] :
		  4'b0;

   assign sevenseg[0] = 1'b0;
   
   sevenseg decode(clk, reset, digit, sevenseg[7:1]);

   // digit scan clock
   always @(posedge clk)
     if (reset)
       begin
	  counter <= 0;
	  dig_clk <= 0;
       end
     else
       begin
	  counter <= counter + 1'b1;
	  if (counter == 0)
	    dig_clk <= ~dig_clk;
       end

   // anode driver
   always @(posedge dig_clk)
     if (reset)
       anode <= 4'b0001;
     else
       case (anode)
	 4'b0001: anode <= 4'b0010;
	 4'b0010: anode <= 4'b0100;
	 4'b0100: anode <= 4'b1000;
	 4'b1000: anode <= 4'b0001;
	 default: anode <= 4'b0001;
       endcase
   
endmodule
