// debounce.v

module debounce(clk, in, out);
   input clk;
   input in;
   output out;

   reg [14:0] clkdiv;
   reg 	      slowclk;
   reg [9:0]  hold;
   reg 	      onetime;
   
   initial
     begin
	onetime = 0;
	hold = 0;
	clkdiv = 0;
	slowclk = 0;
     end
   
   assign     out = hold == 10'b1 || ~onetime;
		
   always @(posedge clk)
     clkdiv <= clkdiv + 1'b1;

   always @(posedge clk)
     if (clkdiv == 0)
       slowclk <= ~slowclk;

   always @(posedge slowclk)
     begin
	hold <= { hold[8:0], 1'b0 };
	onetime <= 1;
     end
   
endmodule
