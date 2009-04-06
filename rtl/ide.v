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
   reg [15:0] ata_out;
   output ata_done;
   reg 	  ata_done;

   inout [15:0] ide_data_bus;
   output ide_dior;
   output ide_diow;
   output [1:0] ide_cs;
   output [2:0] ide_da;

   reg ide_dior;
   reg ide_diow;
   reg [1:0] ide_cs;
   reg [2:0] ide_da;
   
   
   reg [2:0] ata_state;

   parameter idle = 0;
   parameter s0 = 1;
   parameter s1 = 2;
   parameter s2 = 3;
   parameter s3 = 4;
   parameter s4 = 5;

   reg [2:0] ata_state_next;
   
   
   always @(posedge clk)
     if (reset)
       ata_state <= idle;
     else
       ata_state <= ata_state_next;

   assign ide_data_bus = (ata_wr && (ata_state == s0 ||
				     ata_state == s1 ||
				     ata_state == s2 ||
   				     ata_state == s3)) ? ata_in : 16'bz;
			
   always @(clk or ata_state or ata_rd or ata_wr or ata_addr or ide_data_bus)
     begin
	ata_state_next = ata_state;
	ata_done = 0;

	case (ata_state)
	  idle:
	    begin
	       ide_dior = 1;
	       ide_diow = 1;

	       if (ata_rd || ata_wr)
		 begin
		    ata_state_next = s0;
		    ide_cs = ata_addr[4:3];
		    ide_da = ata_addr[2:0];
		 end
	    end
	  
	  s0:
	    begin
	       if (ata_rd)
		 begin
		    ide_dior = 0;
		    ata_state_next = s1;
		 end
	       else
		 if (ata_wr)
		   begin
		      ide_diow = 0;
		      ata_state_next = s1;
		   end
	    end

	  s1: ata_state_next = s2;
	  s2: ata_state_next = s3;
	    
	  s3:
	    begin
	       ide_dior = 1;
	       ide_diow = 1;

	       ata_done = 1;
	       ata_state_next = s4;
	    end

	  s4:
	    begin
	       ide_cs = 2'b11;
	       ide_da = 3'b111;
	       ata_state_next = idle;
	    end

	  default:
	    begin
	       ata_state_next = ata_state_next;
	    end
	endcase
     end

   always @(posedge clk)
     if (reset)
       ata_out <= 0;
     else
       if (ata_state == s2 && ata_rd)
	 ata_out <= ide_data_bus;

endmodule // ide
