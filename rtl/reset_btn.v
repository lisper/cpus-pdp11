// reset.v (from debounce.v)

module reset_btn(clk, in, out);
   input clk;
   input in;
   output out;

   reg [14:0] clkdiv;
   reg 	      slowclk;
   reg 	      resetclk;
   reg [9:0]  hold;
   reg 	      onetime;
   
   initial
     begin
	onetime = 0;
	hold = 0;
	clkdiv = 0;
	slowclk = 0;
	resetclk = 0;
     end
   
   assign out = hold == 10'b1111111111 || ~onetime;
		
   always @(posedge clk)
     begin
       clkdiv <= clkdiv + 15'b1;
       if (clkdiv == 0)
         slowclk <= ~slowclk;
       if (clkdiv[7:0] == 0)
         resetclk <= ~resetclk;
     end

   always @(posedge slowclk)
     begin
	hold <= { hold[8:0], in };
     end
   
   always @(negedge resetclk)
     begin
	//$display("onetime = 1, %t", $time);
	onetime <= 1;
     end
   
endmodule
