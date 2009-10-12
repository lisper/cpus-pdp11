// div3216.v
// Divide 32 bit by 16 bit signed
// based on Patterson and Hennessy's algorithm
//
// tunes to the needs of the pdp-11
//
// ready can be asserted for all cycles;
// state machine won't reset until ready deasserts
// done will assert for only one cycle - valid to clock output

module div3216(clk, reset, ready, done,
	       dividend, divider, quotient, remainder, overflow);

   input         clk;
   input 	 reset;
   input 	 ready;
   input [31:0]  dividend;
   input [15:0]  divider;
   output [15:0] quotient;
   output [15:0] remainder;
   output 	 done;
   output 	 overflow;
	 
   reg [31:0]    quotient_temp;
   reg [63:0] 	 dividend_copy, divider_copy;
   reg           negative_output;

   wire [63:0] 	 diff;
   
   reg [5:0]     bitnum; 

   reg [1:0] 	 state;
   wire [1:0] 	 next_state;
 	 
   parameter 	 idle = 2'd0;
   parameter 	 running = 2'd1;
   parameter 	 last = 2'd2;
   
   assign 	 done = state == last;

   always @(posedge clk)
     if (reset)
       state <= idle;
     else
       state <= next_state;

   assign next_state = (state == idle && ready) ? running :
		       (state == running) ? (bitnum == 6'd1 ? last : running) :
		       (state == last) ? (ready ? last : idle) : idle;

   assign diff = dividend_copy - divider_copy;

   assign quotient = ~negative_output ?
		     quotient_temp[15:0] : ~quotient_temp[15:0] + 16'b1;

   assign remainder = ~dividend[31]/*quotient[15]*/ ?
		      dividend_copy[15:0] : ~dividend_copy[15:0] + 16'b1;

//   assign overflow = ($signed(quotient_temp) > 32'sh00007fff) ||
//		     ($signed(quotient_temp) < 32'shffff8000);
   assign overflow = ($signed(quotient_temp) > 32'sh00008000) ||
		     ($signed(quotient_temp) < 32'shffff8000);
   
   always @(posedge clk) 
     if (reset)
       begin
	  bitnum <= 0;
	  negative_output <= 0;
	  quotient_temp <= 0;
	  dividend_copy <= 0;
	  divider_copy <= 0;
       end
     else
       if (state == idle)
	 begin
            bitnum <= 6'd32;
            quotient_temp <= 0;
            dividend_copy <= !dividend[31] ? 
                             {32'd0, dividend} : 
                             {32'd0, ~dividend + 32'b1};
            divider_copy <= !divider[15] ? 
			    {17'b0, divider, 31'd0} : 
			    {17'b0, ~divider + 16'b1, 31'd0};

            negative_output <= (divider[15] && !dividend[31]) ||
			       (!divider[15] && dividend[31]);
	 end 
       else
	 if (bitnum != 0)
	   begin
              quotient_temp <= { quotient_temp[30:0], diff[63] ? 1'b0 : 1'b1 };

	      if (~diff[63])
		dividend_copy <= diff;

              divider_copy <= { 1'b0, divider_copy[63:1] };
              bitnum <= bitnum - 6'b1;
	   end
   
endmodule

