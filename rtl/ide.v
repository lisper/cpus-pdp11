//
// ide.v
// simple state machine to do proper read & write cycles to IDE device
//

module ide(clk, reset, ata_rd, ata_wr, ata_addr, ata_in, ata_out, ata_done,
	   ide_data_bus, ide_dior, ide_diow, ide_cs, ide_da);

   input clk;
   input reset;
   
   input ata_rd;
   input ata_wr;
   input [4:0] ata_addr;
   input [15:0] ata_in;
   output [15:0] ata_out;
   output ata_done;

   inout [15:0] ide_data_bus;
   output ide_dior;
   output ide_diow;
   output [1:0] ide_cs;
   output [2:0] ide_da;

   reg 		ide_dior;
   reg 		ide_diow;
   reg [1:0] 	ide_cs;
   reg [2:0] 	ide_da;

   //
   parameter [4:0] ATA_DELAY = 8;

   reg [4:0] ata_count;

   reg [2:0] ata_state;

   parameter [2:0]
		s0 = 3'd0,
		s1 = 3'd1,
		s2 = 3'd2,
		s3 = 3'd3,
		s4 = 3'd4,
		s5 = 3'd5,
		s6 = 3'd6,
		s7 = 3'd7;

   wire [2:0] ata_state_next;

   wire       ide_start, ide_busy, ide_stop;
   
   reg [15:0] ide_d_out;
   reg [15:0] ide_d_in;

   
   // if write, drive ide_bus
   assign ide_data_bus = (ata_wr && (ata_state == s1 ||
				     ata_state == s2 ||
				     ata_state == s3 ||
   				     ata_state == s4)) ? ide_d_out : 16'bz;

   // grab data bound for ide at start
   always @(posedge clk)
     if (reset)
       ide_d_out <= 0;
     else
       if (ata_state == s0 && (ata_rd || ata_wr))
	 ide_d_out <= ata_in;
   
   assign ata_out = ide_d_in;
   
   // send back done pulse at end
   assign ata_done = ata_state == s3;
   
   always @(posedge clk)
     if (reset)
       ata_state <= s0;
     else
       ata_state <= ata_state_next;

   assign ata_state_next =
			  (ata_state == s0 && (ata_rd || ata_wr)) ? s1 :
			  (ata_state == s1) ? s2 :
			  (ata_state == s2 && ata_count == ATA_DELAY) ? s3 :
			  (ata_state == s3) ? s4 :
			  (ata_state == s4) ? s5 :
			  (ata_state == s5) ? s6 :
			  (ata_state == s6) ? s7 :
			  (ata_state == s7) ? s0 :
			  ata_state;

   assign ide_start = ata_state == s1;
   assign ide_busy = ata_state == s2;
   assign ide_stop = ata_state == s3;
   
   always @(posedge clk)
     if (reset)
       ata_count <= 0;
     else
       if (ata_state == s1)
	 ata_count <= 0;
       else
	 if (ata_state == s2)
	   ata_count <= ata_count + 1;
   
   // register all the ide signals
   always @(posedge clk)
     if (reset)
       begin
	  ide_dior <= 1'b1;
	  ide_diow <= 1'b1;
	  ide_cs <= 2'b11;
	  ide_da <= 3'b111;
	  ide_d_in <= 0;
       end
     else
       if (ide_start)
	 begin
	    ide_cs <= ata_addr[4:3];
	    ide_da <= ata_addr[2:0];
	    ide_dior <= ata_rd ? 1'b0 : 1'b1;
	    ide_diow <= ata_wr ? 1'b0 : 1'b1;
	 end
       else
	 if (ide_stop)
	   begin
	      ide_dior <= 1'b1;
	      ide_diow <= 1'b1;
	      ide_cs <= 2'b11;
	      ide_da <= 3'b111;
	   end
	 else
	   if (ide_busy)
	     begin
		ide_d_in <= ide_data_bus;
	     end

endmodule
