//
module ipl_below_func(ipl, asserting, result);
   input [2:0] ipl;
   input [7:0] asserting;
   output      result;
   reg 	       result;

   always @(ipl or asserting)
     begin
	case (ipl)
	  0: result <= |asserting;
	  1: result <= |asserting[6:1];
	  2: result <= |asserting[6:2];
	  3: result <= |asserting[6:3];
	  4: result <= |asserting[6:4];
	  5: result <= |asserting[6:5];
	  6: result <= |asserting[6];
	  7: result <= 0;
	endcase // case(ipl)
     end
endmodule
