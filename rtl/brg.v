// brg.v
// baud rate generator for uart

module brg(clk, reset, tx_baud_clk, rx_baud_clk);

   input clk;
   input reset;
   output tx_baud_clk;
   output rx_baud_clk;

   parameter SYS_CLK = 20000000;
   parameter BAUD = 9600;

   parameter RX_CLK_DIV = SYS_CLK / (BAUD * 16 * 2);
   parameter RX_CW = 9;		   // CW >= log2(CLK_DIV)
   parameter TX_CLK_DIV = SYS_CLK / (BAUD * 2);
   parameter TX_CW = 11;	   // CW >= log2(CLK_DIV)
   
   reg [RX_CW-1:0] rx_clk_div;
   reg [TX_CW-1:0] tx_clk_div;
   reg 		tx_baud_clk;
   reg 		rx_baud_clk;


   always @(posedge clk or posedge reset)
     if (reset)
       begin
	  rx_clk_div  <= 0;
	  rx_baud_clk <= 0; 
       end
     else 
       if (rx_clk_div == RX_CLK_DIV)
	 begin
	    rx_clk_div  <= 0;
	    rx_baud_clk <= ~rx_baud_clk;
	 end
       else
	 begin
	    rx_clk_div  <= rx_clk_div + 1;
	    rx_baud_clk <= rx_baud_clk;
	 end

   always @(posedge clk or posedge reset)
     if (reset)
       begin
	  tx_clk_div  <= 0;
	  tx_baud_clk <= 0; 
       end
     else 
       if (tx_clk_div == TX_CLK_DIV)
	 begin
	    tx_clk_div  <= 0;
	    tx_baud_clk <= ~tx_baud_clk;
	 end
       else
	 begin
	    tx_clk_div  <= tx_clk_div + 1;
	    tx_baud_clk <= tx_baud_clk;
	 end
endmodule


