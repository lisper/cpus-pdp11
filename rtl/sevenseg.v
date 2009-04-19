// sevenseg.v
// seven segment decoder for s3board

module sevenseg(digit, ss_out);

   input [3:0] digit;
   output [6:0] ss_out;
   
   reg [6:0] 	ss_out;

   always @(digit)
       case (digit)
	 // segments abcdefg
	 //  a
	 // f b
	 //  g
	 // e c
	 //  d
	 4'd0: ss_out = 7'b0000001;
	 4'd1: ss_out = 7'b1001111;
	 4'd2: ss_out = 7'b0010010;
	 4'd3: ss_out = 7'b0000110;
	 4'd4: ss_out = 7'b1001100;
	 4'd5: ss_out = 7'b0100100;
	 4'd6: ss_out = 7'b1100000;
	 4'd7: ss_out = 7'b0001111;
	 4'd8: ss_out = 7'b0000000;
	 4'd9: ss_out = 7'b0001100;
	 4'ha: ss_out = 7'b0001001;
	 4'hb: ss_out = 7'b1100000;
	 4'hc: ss_out = 7'b0110001;
	 4'hd: ss_out = 7'b1000010;
	 4'he: ss_out = 7'b0010000;
	 4'hf: ss_out = 7'b0111000;
	 default: ss_out = 7'b1111111;
       endcase
   
endmodule
