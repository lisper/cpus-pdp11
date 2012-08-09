//
// sync interface to async sram
// used on s3board
//
// multiplexes between to high speed SRAMs
//

module ram_async(clk, reset,
		 addr, data_in, data_out, rd, wr, wr_inhibit, byte_op, done,
		 ram_a, ram_oe_n, ram_we_n,
		 ram1_io, ram1_ce_n, ram1_ub_n, ram1_lb_n,
		 ram2_io, ram2_ce_n, ram2_ub_n, ram2_lb_n);

   input clk;
   input reset;

   input [17:0] addr;
   input [15:0] data_in;
   output [15:0] data_out;
   input 	 rd, wr, wr_inhibit, byte_op;
   output 	 done;
   
   output [17:0] ram_a;
   output 	 ram_oe_n, ram_we_n;
   inout [15:0]  ram1_io;
   output 	 ram1_ce_n, ram1_ub_n, ram1_lb_n;
   inout [15:0]  ram2_io;
   output 	 ram2_ce_n, ram2_ub_n, ram2_lb_n;

   wire [15:0]  ram1_io;

   //
//   reg [17:0] ram_a;
   reg [15:0] ram_din;
   reg [15:0] ram_dout;
   reg 	      ram_we_n;
   reg 	      ram_oe_n;
   reg 	      ram1_ub_n;
   reg 	      ram1_lb_n;
   wire       ram1_ub, ram1_lb;
   
   //
   assign ram1_ub = ~byte_op || (byte_op && addr[0]);
   assign ram1_lb = ~byte_op || (byte_op && ~addr[0]);

   reg [3:0] state;
   wire [3:0] next_state;

   always @(posedge clk)
     if (reset)
       state <= 0;
     else
       state <= next_state;

//   assign next_state =
//		      (state == 0 && rd) ? 1 :
//		      (state == 0 && wr) ? 5 :
//
//		      (state == 1 && rd) ? 2 :
//		      (state == 2 && rd) ? 2 :
//
//		      (state == 5 && wr) ? 6 :
//		      (state == 6 && wr) ? 6 :
//		      0;

   assign next_state =
		      (state == 0 && rd) ? 1 :
		      (state == 0 && wr) ? 5 :
		      (state == 1 && rd) ? 2 :
		      (state == 5 && wr) ? 6 :
		      0;

//   assign next_state =
//		      (state == 0 && rd) ? 1 :
//		      (state == 0 && wr) ? 5 :
//		      (state == 1 && rd) ? 0 :
//		      (state == 5 && wr) ? 0 :
//		      0;
   
   // one hopes for IOBs...
   always @(posedge clk)
     if (reset)
       begin
//	  ram_a <= 0;
	  ram_din <= 0;
	  ram_we_n <= 1;
	  ram_oe_n <= 1;
	  ram1_ub_n <= 1;
	  ram1_lb_n <= 1;
       end
     else
       begin
	  if (state == 0 && (rd || wr))
	    begin
//	       ram_a <= {1'b0, addr[17:1]};
	       ram_din <= (byte_op ? {data_in[7:0],data_in[7:0]} : data_in);
	       ram_we_n <= ~(wr && ~wr_inhibit);
	       ram_oe_n <= ~rd;
	       ram1_ub_n <= ~ram1_ub;
	       ram1_lb_n <= ~ram1_lb;
	    end
	  else
	    if (state == 1 || state == 5)
	      begin
		 ram_we_n <= 1;
		 ram_oe_n <= 1;
		 ram1_ub_n <= 1;
		 ram1_lb_n <= 1;
	      end
       end

   assign ram_a = {1'b0, addr[17:1]};

   // transparent latch
   always @(ram_oe_n or ram1_io or reset)
     if (reset)
       ram_dout = 0;
     else
       if (state == 1 && rd)
	 ram_dout = ~byte_op ? ram1_io :
		    {8'b0, addr[0] ? ram1_io[15:8] : ram1_io[7:0]};

   assign done = state == 1 || state == 5;
   
   assign data_out = ram_dout;

   assign ram1_io = ~ram_we_n ? ram_din : 16'bz;
   
   assign ram1_ce_n = 1'b0;
   assign ram2_io = 16'bz;
   assign ram2_ce_n = 1'b1;
   assign ram2_ub_n = 1'b1;
   assign ram2_lb_n = 1'b1;

endmodule

`ifdef never

module ram_async(clk, reset,
		 addr, data_in, data_out, rd, wr, wr_inhibit, byte_op, done,
		 ram_a, ram_oe_n, ram_we_n,
		 ram1_io, ram1_ce_n, ram1_ub_n, ram1_lb_n,
		 ram2_io, ram2_ce_n, ram2_ub_n, ram2_lb_n);

   input        clk;
   input        reset;
   input [17:0] addr;
   input [15:0] data_in;
   output [15:0] data_out;
   input 	 rd, wr, wr_inhibit, byte_op;

   output [17:0] ram_a;
   output 	 ram_oe_n, ram_we_n;
   inout [15:0]  ram1_io;
   output 	 ram1_ce_n, ram1_ub_n, ram1_lb_n;
   inout [15:0]  ram2_io;
   output 	 ram2_ce_n, ram2_ub_n, ram2_lb_n;

   wire [15:0]  ram1_io;

   output 	done;
 	
   //
   wire 	 ram1_ub, ram1_lb;

   //
   assign ram1_ub = ~byte_op || (byte_op && addr[0]);
   assign ram1_lb = ~byte_op || (byte_op && ~addr[0]);

   assign data_out = ~byte_op ? ram1_io :
		     {8'b0, addr[0] ? ram1_io[15:8] : ram1_io[7:0]};

   //
   assign ram_a = {1'b0, addr[17:1]};
   assign ram_oe_n = ~rd;
   assign ram_we_n = ~(wr && ~wr_inhibit);

   assign ram1_io = ~ram_oe_n ? 16'bz :
		    (byte_op ? {data_in[7:0],data_in[7:0]} : data_in);

   assign ram1_ce_n = 1'b0;
   assign ram1_ub_n = ~ram1_ub;
   assign ram1_lb_n = ~ram1_lb;
   
   assign ram2_io = 16'b0;
   assign ram2_ce_n = 1'b1;
   assign ram2_ub_n = 1'b1;
   assign ram2_lb_n = 1'b1;

   assign done = 1'b1;
   
endmodule

`endif
