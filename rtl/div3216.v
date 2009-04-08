// div3216.v
// Divide 32 bit by 16 bit signed
// based on Patterson and Hennessy's algorithm
//
// ready can be asserted for multiple cycles
// done will assert for only one cycle - valid to clock output

module div3216(clk, reset, ready, done,
	       dividend, divider, quotient);

   input         clk;
   input 	 reset;
   input 	 ready;
   input [31:0]  dividend;
   input [15:0]  divider;
   output [31:0] quotient;
   output 	 done;
 	 
   reg [31:0]    quotient_temp;
   reg [63:0] 	 dividend_copy, divider_copy;
   reg           negative_output;

   wire [63:0] 	 diff;
   
   reg [5:0]     bit; 

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
		       (state == running) ? (bit == 5'd1 ? last : running) :
		       (state == last) ? idle : idle;

   assign diff = dividend_copy - divider_copy;

   assign quotient = ~negative_output ? quotient_temp : ~quotient_temp + 1'b1;

   
   always @(posedge clk) 
     if (reset)
       begin
	  bit <= 0;
	  negative_output <= 0;
	  quotient_temp <= 0;
	  dividend_copy <= 0;
	  divider_copy <= 0;
       end
     else
       if (state == idle)
	 begin
            bit <= 6'd32;
            quotient_temp <= 0;
            dividend_copy <= !dividend[31] ? 
                             {32'd0, dividend} : 
                             {32'd0, ~dividend + 1'b1};
            divider_copy <= !divider[15] ? 
			    {17'b0, divider, 31'd0} : 
			    {17'b0, ~divider + 1'b1, 31'd0};

            negative_output <= (divider[15] && !dividend[31]) ||
			       (!divider[15] && dividend[31]);
	 end 
       else
	 if (bit != 0)
	   begin
              quotient_temp <= { quotient_temp[30:0], diff[63] ? 1'b0 : 1'b1 };

	      if (~diff[63])
		dividend_copy <= diff;

              divider_copy <= { 1'b0, divider_copy[63:1] };
              bit <= bit - 1'b1;
	   end
   
endmodule

