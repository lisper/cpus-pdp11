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
   reg [2:0] ata_state;

   parameter idle = 3'd0;
   parameter s0 = 3'd1;
   parameter s1 = 3'd2;
   parameter s2 = 3'd3;
   parameter s3 = 3'd4;
   parameter s4 = 3'd5;

   wire      assert_cs;
   wire      assert_rw;

   reg [2:0] ata_state_next;
   
   reg [15:0] ide_d_out;
   reg [15:0] ide_d_in;

   // if write, drive ide_bus
   assign ide_data_bus = (ata_wr && (ata_state == s0 ||
				     ata_state == s1 ||
				     ata_state == s2 ||
   				     ata_state == s3)) ? ide_d_out : 16'bz;

   // assert cs & da during r/w cycle
   assign assert_cs = (ata_rd || ata_wr) && ata_state != s4;
   
   // assert r/w one cycle sort
   assign assert_rw = ata_state == s0 ||
		      ata_state == s1 ||
		      ata_state == s2;

   always @(posedge clk)
     if (reset)
       begin
	  ide_cs <= 2'b11;
	  ide_da <= 3'b111;
	  ide_dior <= 1;
	  ide_diow <= 1;
       end
     else
       begin
	  ide_cs <= assert_cs ? ata_addr[4:3] : 2'b11;
	  ide_da <= assert_cs ? ata_addr[2:0] : 3'b111;

	  ide_dior <= ~(assert_rw && ata_rd);
	  ide_diow <= ~(assert_rw && ata_wr);
       end


   always @(posedge clk)
     if (reset)
       begin
	  ide_d_out <= 0;
	  ide_d_in <= 0;
       end
     else
       begin
	  if (ata_state == idle)
	    ide_d_out <= ata_in;

	  if (ata_state == s2 && ata_rd)
	    ide_d_in <= ide_data_bus;
       end
   
   assign ata_out = ide_d_in;
   
   // send back done pulse at end
   assign ata_done = ata_state == s4;
   
   always @(posedge clk)
     if (reset)
       ata_state <= idle;
     else
       ata_state <= ata_state_next;

   always @(clk or ata_state or ata_rd or ata_wr or ata_addr or ide_data_bus)
     begin
	case (ata_state)
	  idle:
	    begin
	       if (ata_rd || ata_wr)
		    ata_state_next = s0;
	       else
		    ata_state_next = idle;
	    end
	  
	  s0: ata_state_next = s1;
	  s1: ata_state_next = s2;
	  s2: ata_state_next = s3;
	  s3: ata_state_next = s4;
	  s4: ata_state_next = idle;
	  default: ata_state_next = idle;
	endcase
     end

endmodule // ide
