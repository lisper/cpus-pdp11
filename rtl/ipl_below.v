//
module ipl_below_func(ipl, asserting, result);
   input [2:0] ipl;
   input [7:0] asserting;
   output      result;
   reg 	       result;

   always @(ipl or asserting)
     begin
	case (ipl)
	  0: result = |asserting[7:1];
	  1: result = |asserting[7:2];
	  2: result = |asserting[7:3];
	  3: result = |asserting[7:4];
	  4: result = |asserting[7:5];
	  5: result = |asserting[7:6];
	  6: result = |asserting[7];
	  7: result = 0;
	endcase // case(ipl)
     end
endmodule
