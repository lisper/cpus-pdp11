// sr_regs.v

module sr_regs(clk, reset, iopage_addr, data_in, data_out, decode,
	       iopage_rd, iopage_wr, iopage_byte_op, switches,
	       unibus_to, memory_to);

   input clk;
   input reset;
   input [12:0] iopage_addr;
   input [15:0] data_in;
   input 	iopage_rd, iopage_wr, iopage_byte_op;
   output [15:0] data_out;
   reg [15:0] 	 data_out;
   output 	 decode;
   input [15:0]  switches;
   input 	 unibus_to, memory_to;

   //
   reg [15:0]  switch_sample;
   wire [15:0] cpu_err;
   
   assign decode = (iopage_addr == 13'o17570) || (iopage_addr == 13'o17766);
   
   always @(clk or decode or iopage_addr or iopage_rd or switch_sample)
     begin
	if (decode)
	  case (iopage_addr)
	    13'o17570: data_out = switch_sample;
	    13'o17766: data_out = cpu_err;
	    default: data_out = 16'b0;
	  endcase
        else
	  data_out = 16'b0;
     end

   always @(posedge clk)
     if (reset)
       switch_sample <= 0;
     else
       switch_sample <= switches;

   always @(posedge clk)
       if (iopage_wr)
	 case (iopage_addr)
	    13'o17570:
	      begin
`ifdef debug
		 $display("display: write %o", data_in);
`endif
	      end
	 endcase

   //
   wire wr_cpu_err;
   reg 	unibus_to_r, memory_to_r;

   assign wr_cpu_err = iopage_wr && iopage_addr == 13'o17766;
   assign cpu_err = { 10'b0, memory_to_r, unibus_to_r, 4'b0 };

   always @(posedge clk)
     if (reset)
       begin
	  unibus_to_r <= 0;
	  memory_to_r <= 0;
       end
     else
       begin
	  if (wr_cpu_err)
	    begin
	       unibus_to_r <= data_in[4];
	       memory_to_r <= data_in[5];
	    end
	  else
	    if (unibus_to)
	      unibus_to_r <= 1;
	    else
	      if (memory_to)
		memory_to_r <= 1;
       end
   
endmodule

