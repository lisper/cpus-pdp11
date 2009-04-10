// shift32.v
//
// ready can be asserted for all cycles;
// state machine won't reset until ready deasserts
// done will assert for only one cycle - valid to clock output

module shift32(clk, reset, ready, done, in, out, last_bit, shift); 

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
   
   reg [5:0] 	 count; 
   wire 	 final;
   
   reg [1:0] 	 state;
   wire [1:0] 	 next_state;
   wire 	 up;
   
   parameter 	 idle    = 2'd0;
   parameter 	 running = 2'd1;
   parameter 	 last    = 2'd2;
   
   assign 	 done = state == last;
   assign 	 up = ~shift[5];
 	 
   always @(posedge clk)
     if (reset)
       state <= idle;
     else
       state <= next_state;

   assign next_state = (state == idle && ready) ? running :
		       (state == running) ? (final ? last : running) :
		       (state == last) ? (ready ? last : idle) : idle;

   assign final = up ? (count == 5'd1) : (count == -5'd1);
   
	  
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
       end 
     else
       if (state == running)
	 if (up)
	   begin
	      last_bit <= out[31];
	      out <= { out[30:0], 1'b0 };
	      count <= count - 1;
	   end
	 else
	   begin
	      last_bit <= out[0];
	      out <= { in[31], out[31:1] };
	      count <= count + 1;
	   end
   
endmodule

