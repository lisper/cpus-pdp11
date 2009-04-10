// mul1616.v
// Multiply 16x16 bit signed
// based on Patterson and Hennessy's algorithm
//
// ready can be asserted for all cycles;
// state machine won't reset until ready deasserts
// done will assert for only one cycle - valid to clock output

module mul1616(clk, reset, ready, done, multiplier, multiplicand, product); 

   input         clk;
   input 	 reset;
   input 	 ready;
   input [15:0]  multiplier, multiplicand;

   output 	 done;
   output [31:0] product;
   
   reg [31:0]    product, product_temp;

   reg [15:0]    multiplier_copy;
   reg [31:0]    multiplicand_copy;
   reg           negative_output;
   
   reg [4:0]     bitnum; 

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
		       (state == running) ? (bitnum == 5'd1 ? last : running) :
		       (state == last) ? (ready ? last : idle) : idle;
   
   always @(posedge clk)
     if (reset)
       begin
	  bitnum <= 0;
	  negative_output <= 0;
	  product <= 0;
	  product_temp <= 0;
	  multiplicand_copy <= 0;
	  multiplier_copy <= 0;
       end
   else
     if (state == idle)
       begin
          bitnum            <= 5'd16;
          product           <= 0;
          product_temp      <= 0;
          multiplicand_copy <= !multiplicand[15] ? 
                               { 16'd0, multiplicand } : 
                               { 16'd0, ~multiplicand + 1'b1};
          multiplier_copy   <= !multiplier[15] ?
                              multiplier : ~multiplier + 1'b1; 

	  negative_output <= multiplier[15] ^ multiplicand[15];
       end 
     else
       if (bitnum != 0)
	 begin
            if (multiplier_copy[0] == 1'b1)
	      product_temp <= product_temp + multiplicand_copy;

            product <= !negative_output ? 
		       product_temp : ~product_temp + 1'b1;

            multiplier_copy <= multiplier_copy >> 1;
            multiplicand_copy <= multiplicand_copy << 1;
            bitnum <= bitnum - 1'b1;
	 end
   
endmodule
