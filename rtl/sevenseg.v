// sevenseg.v
// seven segment decoder for s3board

module sevenseg(clk, reset, digit, ss_out);

   input clk;
   input reset;
   input [3:0] digit;
   output [6:0] ss_out;
   
   reg [6:0] 	ss_out;

   always @(posedge clk or posedge reset)
     if (reset)
       ss_out <=0;
     else
       case (digit)
	 // segments abcdefg
	 //  a
	 // f b
	 //  g
	 // e c
	 //  d
	 4'd0: ss_out <= 7'b1111110;
	 4'd1: ss_out <= 7'b0110000;
	 4'd2: ss_out <= 7'b1101101;
	 4'd3: ss_out <= 7'b1111001;
	 4'd4: ss_out <= 7'b0110011;
	 4'd5: ss_out <= 7'b1011011;
	 4'd6: ss_out <= 7'b0011111;
	 4'd7: ss_out <= 7'b1110000;
	 4'd8: ss_out <= 7'b1111111;
	 4'd9: ss_out <= 7'b1110011;
	 default: ss_out <= 7'b0000000;
       endcase
   
endmodule
