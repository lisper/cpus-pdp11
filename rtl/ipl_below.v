//
module ipl_below(clk, reset, ipl, asserting, result, winner);
   input clk;
   input reset;
   input [2:0] ipl;
   input [7:0] asserting;
   output      result;
   reg 	       result;
   output [7:0] winner;

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

   wire [7:0] mask, hot;
   
   assign mask =
		ipl == 0 ? 8'hfe :
		ipl == 1 ? 8'hfc :
		ipl == 2 ? 8'hf8 :
		ipl == 3 ? 8'hf0 :
		ipl == 4 ? 8'he0 :
		ipl == 5 ? 8'hc0 :
		ipl == 6 ? 8'h80 :
		0;

   assign hot = asserting & mask;
   
   assign winner =
   		  hot[7] ? 8'h80 :
		  hot[6] ? 8'h40 :
		  hot[5] ? 8'h20 :
		  hot[4] ? 8'h10 :
		  hot[3] ? 8'h08 :
		  hot[2] ? 8'h04 :
		  hot[1] ? 8'h02 :
		  hot[0] ? 8'h01 :
		  0;

`ifdef debug
   wire [7:0] count;

   assign count =
		 (asserting[0] ? 1 : 0) +
		 (asserting[1] ? 1 : 0) +
		 (asserting[2] ? 1 : 0) +
		 (asserting[3] ? 1 : 0) +
		 (asserting[4] ? 1 : 0) +
		 (asserting[5] ? 1 : 0) +
		 (asserting[6] ? 1 : 0) +
		 (asserting[7] ? 1 : 0);

   always @(posedge clk)
     begin
	if (count > 1) begin
	   $display("XXX bus_int_ipl count=%d; %t", count, $time);
	   //$finish;
	end
     end
`endif

endmodule
