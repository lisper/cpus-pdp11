// shift32.v
// simple 32 bit left/right shifter
//
// ready can be asserted for all cycles;
// state machine won't reset until ready deasserts
// done will assert for only one cycle - valid to clock output

module shift32(clk, reset, ready, done, in, out, shift,
	       last_bit, sign_change16, sign_change32); 

   input         clk;
   input 	 reset;
   input 	 ready;
   input [31:0]  in;
   input [5:0] 	 shift;

   output 	 done;
   output [31:0] out;
   reg [31:0] 	 out;
   output 	 last_bit;
   reg 		 last_bit;
   output 	 sign_change16;
   reg 		 sign_change16;
   output 	 sign_change32;
   reg 		 sign_change32;
   
   reg [5:0] 	 count; 
   wire 	 final_bit;
   
   reg [1:0] 	 state;
   wire [1:0] 	 next_state;
   wire 	 up;
   wire 	 sign_different16, sign_different32;
   
   parameter 	 idle    = 2'b00;
   parameter 	 running = 2'b01;
   parameter 	 last    = 2'b10;
   
   assign 	 done = state == last;
   assign 	 up = ~shift[5];
 	 
   always @(posedge clk)
     if (reset)
       state <= idle;
     else
       state <= next_state;

   assign next_state = (state == idle && ready) ? (shift==6'd0 ? last:running) :
		       (state == running) ? (final_bit ? last : running) :
			 (state == last) ? (ready ? last : idle) :
				 idle;

   assign final_bit = up ? (count == 6'd1) : (count == -6'd1);

   assign sign_different16 = out[15] ^ out[14];
   assign sign_different32 = out[31] ^ out[30];
	  
   always @(posedge clk)
     if (reset)
       begin
	  count <= 0;
	  out <= 0;
       end
   else
     if (state == idle)
       begin
          count <= shift;
	  out <= in;
	  sign_change16 <= 0;
	  sign_change32 <= 0;
	  last_bit <= 0;
       end 
     else
       if (state == running)
	 if (up)
	   begin
	      last_bit <= out[31];
	      out <= { out[30:0], 1'b0 };
	      count <= count - 1'b1;

	      if (sign_different16)
		sign_change16 <= 1'b1;

	      if (sign_different32)
		sign_change32 <= 1'b1;
	   end
	 else
	   begin
	      last_bit <= out[0];
	      out <= { in[31], out[31:1] };
	      count <= count + 1'b1;
	   end
   
endmodule

