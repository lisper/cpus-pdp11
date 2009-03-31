
// Simple Unsigned Combinational Multiplier
//   Multiplies using combinational logic only.
//   Cost of n-bit multiplier: proportional to n^2.
//   Speed of n-bit multiplier: time needed for n additions:
//      Proportional to n.
//
// Simple Unsigned Multiplier
//   Unlike combinational multiplier, uses a clock.
//   Cost of n-bit multiplier: 
//      Proportional to n.
//      Includes 5n bits of register storage.
//   Speed of n-bit multiplier: 
//      Product in n clocks.
//      Clock period includes 2n-bit add.
//   Uses more hardware than streamlined multipliers in next section.
//
// This is a straightforward coding of the longhand multiplication
// technique.

module simple_combinational_mult(product,multiplier,multiplicand);
   
   input [15:0]  multiplier, multiplicand;
   output        product;

   reg [31:0]    product;

   integer       i;

   always @( multiplier or multiplicand )
     begin
        product = 0;
        for (i = 0; i < 16; i=i+1)
          if (multiplier[i] == 1'b1)
	    product = product + (multiplicand << i);
     end
     
endmodule


// Simple Unsigned Multiplier
//
// Sequential multiplier. Handshaking signals are added to command
// multiplier to start and to indicate when finished.  Registers are
// provided for shifted versions of the multiplier and multiplicand.

module simple_mult(product,ready,multiplier,multiplicand,start,clk);
   
   input [15:0]  multiplier, multiplicand;
   input         start, clk;
   output        product;
   output        ready;

   reg [31:0]    product;

   reg [15:0]    multiplier_copy;
   reg [31:0]    multiplicand_copy;
   
   reg [4:0]     bit; 
   wire          ready = !bit;
   
   initial bit = 0;

   always @( posedge clk )
     if (ready && start)
       begin
          bit               = 16;
          product           = 0;
          multiplicand_copy = { 16'd0, multiplicand };
          multiplier_copy   = multiplier;
       end
     else
       if (bit)
	 begin
            if (multiplier_copy[0] == 1'b1)
	      product = product + multiplicand_copy;

            multiplier_copy = multiplier_copy >> 1;
            multiplicand_copy = multiplicand_copy << 1;
            bit = bit - 1;
	 end

endmodule



/// Division Hardware
//
// Simple Divider
//   A straightforward translation of binary division algorithm into hardware.
//   Cost of n-bit divider:
//      Proportional to n.
//      Uses 5n bits of register storage.
//   Speed of n-bit divider:
//      Quotient in n clock cycles.
//
// Streamlined Divider
//   Less storage needed.
//   Cost of n-bit divider:
//      Proportional to n.
//      Uses 2n bits of register storage.
//   Speed of n-bit divider:
//      Quotient in n clock cycles.


module simple_divider(quotient,remainder,ready,dividend,divider,start,clk);
   
   input [15:0]  dividend,divider;
   input         start, clk;
   output        quotient,remainder;
   output        ready;

//  """"""""|
//     1011 |  <----  dividend_copy
// -0011    |  <----  divider_copy
//  """"""""|    0  Difference is negative: copy dividend and put 0 in quotient.
//     1011 |  <----  dividend_copy
//  -0011   |  <----  divider_copy
//  """"""""|   00  Difference is negative: copy dividend and put 0 in quotient.
//     1011 |  <----  dividend_copy
//   -0011  |  <----  divider_copy
//  """"""""|  001  Difference is positive: use difference and put 1 in quotient.
//            quotient (numbers above)   

   reg [15:0]    quotient;
   reg [31:0]    dividend_copy, divider_copy, diff;
   wire [15:0]   remainder = dividend_copy[15:0];

   reg [4:0]     bit; 
   wire          ready = !bit;
   
   initial bit = 0;

   always @( posedge clk ) 

     if( ready && start )
       begin
          bit = 16;
          quotient = 0;
          dividend_copy = {16'd0,dividend};
          divider_copy = {1'b0,divider,15'd0};
       end
     else
       begin
        diff = dividend_copy - divider_copy;

        quotient = quotient << 1;

        if (!diff[31]) begin
           dividend_copy = diff;
           quotient[0] = 1'd1;
        end

        divider_copy = divider_copy >> 1;
        bit = bit - 1;
     end

endmodule


// Streamlined divider.  
// Uses less register storage than simple divider.

module streamlined_divider(quotient,remainder,ready,dividend,divider,start,clk);
   
   input [15:0]  dividend,divider;
   input         start, clk;
   output        quotient,remainder;
   output        ready;

   reg [31:0]    qr;
   reg [16:0]    diff;

//
//              0000 1011
//  """"""""|
//     1011 |   0001 0110     <- qr reg
// -0011    |  -0011          <- divider (never changes)
//  """"""""|           
//     1011 |   0010 110o     <- qr reg
//  -0011   |  -0011
//  """"""""|
//     1011 |   0101 10oo     <- qr reg
//   -0011  |  -0011
//  """"""""|   0010 1000     <- qr reg before shift
//     0101 |   0101 0ooi     <- after shift
//    -0011 |  -0011
//  """"""""|   0010 ooii
//       10 |   
//
// Quotient, 3 (0011); remainder 2 (10).

   
   wire [15:0]   remainder = qr[31:16];
   wire [15:0]   quotient = qr[15:0];

   reg [4:0]     bit; 
   wire          ready = !bit;
   
   initial bit = 0;

   always @( posedge clk ) 
     if (ready && start)
       begin
          bit = 16;
          qr = {16'd0,dividend};
       end 
     else
       begin
        diff = qr[31:15] - {1'b0,divider};

        if (diff[16])
          qr = {qr[30:0],1'd0};
        else
          qr = {diff[15:0],qr[14:0],1'd1};
        
        bit = bit - 1;
     end

endmodule


//-----------------------------------------------------------------------------


// Unsigned/Signed multiplication based on Patterson and Hennessy's algorithm.
// Description: Calculates product.  The "sign" input determines whether
// signs (two's complement) should be taken into consideration.

module multiply(ready, product, multiplier, multiplicand, sign, clk); 

   input         clk;
   input         sign;
   input [31:0]  multiplier, multiplicand;
   output [63:0] product;
   output        ready;

   reg [63:0]    product, product_temp;

   reg [31:0]    multiplier_copy;
   reg [63:0]    multiplicand_copy;
   reg           negative_output;
   
   reg [5:0]     bit; 
   wire          ready = !bit;

   initial bit = 0;
   initial negative_output = 0;

   always @(posedge clk)

     if (ready)
       begin
          bit               = 6'd32;
          product           = 0;
          product_temp      = 0;
          multiplicand_copy = (!sign || !multiplicand[31]) ? 
                              { 32'd0, multiplicand } : 
                              { 32'd0, ~multiplicand + 1'b1};
          multiplier_copy   = (!sign || !multiplier[31]) ?
                              multiplier :
                              ~multiplier + 1'b1; 

//          negative_output = sign && 
//                            ((multiplier[31] && !multiplicand[31]) 
//                             ||(!multiplier[31] && multiplicand[31]));

	  negative_output = sign && ((multiplier[31] ^ multiplicand[31]);
        
       end 
     else
       if (bit > 0)
	 begin

            if (multiplier_copy[0] == 1'b1)
	      product_temp = product_temp + multiplicand_copy;

            product = (!negative_output) ? 
                      product_temp : 
                      ~product_temp + 1'b1;

            multiplier_copy = multiplier_copy >> 1;
            multiplicand_copy = multiplicand_copy << 1;
            bit = bit - 1'b1;
     end
endmodule


// Unsigned/Signed division based on Patterson and Hennessy's algorithm.
// Description: Calculates quotient.  The "sign" input determines whether
//              signs (two's complement) should be taken into consideration.

module divide(ready, quotient, remainder, dividend, divider, sign, clk);

   input         clk;
   input         sign;
   input [31:0]  dividend, divider;
   output [31:0] quotient, remainder;
   output        ready;

   reg [31:0]    quotient, quotient_temp;
   reg [63:0]    dividend_copy, divider_copy, diff;
   reg           negative_output;
   
   wire [31:0]   remainder = (!negative_output) ? 
                             dividend_copy[31:0] : 
                             ~dividend_copy[31:0] + 1'b1;

   reg [5:0]     bit; 
   wire          ready = !bit;

   initial bit = 0;
   initial negative_output = 0;

   always @( posedge clk ) 

     if (ready)
       begin
        bit = 6'd32;
        quotient = 0;
        quotient_temp = 0;
        dividend_copy = (!sign || !dividend[31]) ? 
                        {32'd0,dividend} : 
                        {32'd0,~dividend + 1'b1};
        divider_copy = (!sign || !divider[31]) ? 
                       {1'b0,divider,31'd0} : 
                       {1'b0,~divider + 1'b1,31'd0};

        negative_output = sign &&
                          ((divider[31] && !dividend[31]) 
                        ||(!divider[31] && dividend[31]));
       end 
     else
       if (bit > 0)
	 begin
            diff = dividend_copy - divider_copy;
            quotient_temp = quotient_temp << 1;

            if (!diff[63])
	      begin
		 dividend_copy = diff;
		 quotient_temp[0] = 1'd1;
              end

            quotient = (!negative_output) ? 
                       quotient_temp : 
                       ~quotient_temp + 1'b1;

            divider_copy = divider_copy >> 1;
            bit = bit - 1'b1;
	 end
   
endmodule



    integer index;

    for (index = 0; index < 32; index = index + 1)
    begin
            if (multiplier_copy[0] == 1'b1)
	       product_temp = product_temp + multiplicand_copy;
      
            multiplier_copy = multiplier_copy >> 1; 
            multiplicand_copy = multiplicand_copy << 1; 
            bit = bit - 1'b1; 
    end

            product = (!negative_output) ?  
                      product_temp :  
                      ~product_temp + 1'b1;


