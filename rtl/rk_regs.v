// rk_regs.v
//
// simulated rk05 (RK11) drive
// simple state machine which talks to IDE drive
// (the idea came from pop-11, thanks!)
// copyright Brad Parker <brad@heeltoe.com> 2009

`include "ide.v"

module rk_regs (clk, reset, iopage_addr, data_in, data_out, decode,
		iopage_rd, iopage_wr, iopage_byte_op,
		interrupt, vector,
		ide_data_bus, ide_dior, ide_diow, ide_cs, ide_da,
   		dma_req, dma_ack, dma_addr, dma_data_in, dma_data_out,
		dma_rd, dma_wr);
   
   input clk;
   input reset;
   input [12:0] iopage_addr;
   input [15:0] data_in;
   input 	iopage_rd, iopage_wr, iopage_byte_op;
   output [15:0] data_out;
   reg [15:0] 	 data_out;
   output 	 decode;

   output 	 interrupt;
   output [7:0]  vector;

   output 	 dma_req;
   input 	 dma_ack;
   output [17:0] dma_addr;
   output [15:0] dma_data_in;
   input [15:0]  dma_data_out;
   output 	 dma_rd;
   output 	 dma_wr;

   reg 		 dma_req;
   reg [17:0] 	 dma_addr;
   reg [15:0] 	 dma_data_in;
   reg 		 dma_rd;
   reg 		 dma_wr;
   
   //
   reg [7:0]  vector;
   
   reg [15:0] 	 rkds, rker, rkwc, rkda;
   reg [17:0] 	 rkba;

   reg 		 rkcs_err;
   reg [3:0] 	 rkcs_cmd;
   reg 		 rkcs_done;
   reg 		 rkcs_ie;
   reg [1:0] 	 rkcs_mex;
   
   reg 	 assert_int;
   reg 	 clear_err, set_err;
   reg 	 clear_cmd;
   reg 	 set_done;
   reg 	 clear_done;
 	 
   reg inc_ba;
   reg inc_wc;
   
   parameter CSR_BIT_GO = 0;
   parameter CSR_BIT_IE = 6;
   parameter CSR_BIT_DONE = 7;

   parameter ATA_ALTER   = 5'b01110;
   parameter ATA_DEVCTRL = 5'b01110; /* bit [2] is a nIEN */
   parameter ATA_DATA    = 5'b10000;
   parameter ATA_ERROR   = 5'b10001;
   parameter ATA_FEATURE = 5'b10001;
   parameter ATA_SECCNT  = 5'b10010;
   parameter ATA_SECNUM  = 5'b10011; /* LBA[7:0] */
   parameter ATA_CYLLOW  = 5'b10100; /* LBA[15:8] */
   parameter ATA_CYLHIGH = 5'b10101; /* LBA[23:16] */
   parameter ATA_DRVHEAD = 5'b10110; /* LBA + DRV + LBA[27:24] */
   parameter ATA_STATUS  = 5'b10111;
   parameter ATA_COMMAND = 5'b10111;

   parameter IDE_STATUS_BSY =  7;
   parameter IDE_STATUS_DRDY = 6;
   parameter IDE_STATUS_DWF =  5;
   parameter IDE_STATUS_DSC =  4;
   parameter IDE_STATUS_DRQ =  3;
   parameter IDE_STATUS_CORR = 2;
   parameter IDE_STATUS_IDX =  1;
   parameter IDE_STATUS_ERR =  0;
   
   parameter ATA_CMD_READ = 16'h0020;
   parameter ATA_CMD_WRITE = 16'h0030;

   reg [4:0] rk_state;
   reg [4:0] rk_state_next;

   parameter ready = 5'd0;
   parameter init0 = 5'd1;
   parameter init1 = 5'd2;
   parameter init2 = 5'd3;
   parameter init3 = 5'd4;
   parameter init4 = 5'd5;
   parameter init5 = 5'd6;
   parameter init6 = 5'd7;
   parameter init7 = 5'd8;
   parameter init8 = 5'd9;
   parameter init9 = 5'd10;
   parameter init10 = 5'd11;
   parameter init11 = 5'd12;
   parameter read0 = 5'd13;
   parameter read1 = 5'd14;
   parameter write0 = 5'd15;
   parameter write1 = 5'd16;
   parameter last0 = 5'd17;
   parameter last1 = 5'd18;
   parameter last2 = 5'd19;
   parameter last3 = 5'd20;
   parameter wait0 = 5'd21;
   parameter wait1 = 5'd22;

   reg 	     ata_rd;
   reg 	     ata_wr;
   reg [4:0] ata_addr;
   reg [15:0] ata_in;
   wire [15:0] ata_out;
   wire        ata_done;

   inout [15:0] ide_data_bus;
   output 	ide_dior, ide_diow;
   output [1:0] ide_cs;
   output [2:0] ide_da;
   
   ide ide1(.clk(clk), .reset(reset),
	    .ata_rd(ata_rd), .ata_wr(ata_wr), .ata_addr(ata_addr),
	    .ata_in(ata_in), .ata_out(ata_out), .ata_done(ata_done),
	    .ide_data_bus(ide_data_bus),
	    .ide_dior(ide_dior), .ide_diow(ide_diow),
	    .ide_cs(ide_cs), .ide_da(ide_da));

   
   assign    decode = (iopage_addr == 13'o17400) |
		      (iopage_addr == 13'o17404) |
		      (iopage_addr == 13'o17402) |
		      (iopage_addr == 13'o17406) |
		      (iopage_addr == 13'o17410) |
   		      (iopage_addr == 13'o17412);

   // register read
   always @(clk or iopage_addr or iopage_rd or iopage_byte_op)
     begin
	if (decode)
	  case (iopage_addr)
	    13'o17400: data_out = rkda;
	    13'o17402: data_out = rker;
	    13'o17404: data_out = { rkcs_err, 7'b0,
				    rkcs_done, rkcs_ie, rkcs_mex,
				    rkcs_cmd };
	    13'o17406: data_out = rkwc;
	    13'o17410: data_out = rkba[15:0];
	    13'o17412: data_out = rkda;
	  endcase
     end

   // register write
   always @(posedge clk)
     if (reset)
       begin
	  rker <= 0;
	  rkwc <= 0;
	  rkba <= 0;
	  rkda <= 0;

	  rkcs_err <= 0;
	  rkcs_done <= 0;
	  rkcs_ie <= 0;
	  rkcs_mex <= 0;
	  rkcs_cmd <= 0;
       end
     else
       begin
	  if (iopage_wr)
	    case (iopage_addr)
	      //13'o17400:
	      //13'o17402:
	      13'o17404:
		begin
		   rkcs_done <= data_in[7];
		   rkcs_ie <= data_in[6];
		   rkcs_mex <= data_in[5:4];
		   rkcs_cmd <= data_in[3:0];
		   $display("rk: write rkcs %o", data_in);
		end
	      
	      13'o17406:
		begin
		   rkwc <= data_in;
		   $display("rk: write rkwc %o", data_in);
		end
	      
	      13'o17410:
		begin
		   rkba[15:0] <= data_in;
		   $display("rk: write rkba %o", data_in);
		end
	      
	      13'o17412:
		begin
		   rkda <= data_in;
		   $display("rk: write rkda %o", data_in);
		end

	    endcase
	  else
	    begin
   	       if (clear_err)
		 rkcs_err <= 0;
	       else
		 if (set_err)
		   rkcs_err <= 1;
	       
	       if (clear_cmd)
		 rkcs_cmd <= 0;

	       if (set_done)
		 rkcs_done <= 1;
	       else
		 if (clear_done)
		   rkcs_done <= 0;

	       if (inc_ba)
		 begin
		    rkba <= rkba + 2;
		    rkcs_mex <= rkba[17:16];
		    $display("rk: inc ba %o", rkba);
		 end
	       
	       if (inc_wc)
		 rkwc <= rkwc + 1;
	       
	    end
       end

   assign interrupt = assert_int;
   
   // rk state machine
   always @(posedge clk)
     if (reset)
       rk_state <= ready;
     else
       begin
	  rk_state <= rk_state_next;
	  $display("rk_state %d, dma_req %b, dma_ack %b, rkwc %o",
		   rk_state, dma_req, dma_ack, rkwc);
       end

   always @(rk_state or rkcs_cmd or rkcs_ie or ata_done or ata_out or dma_ack)
     begin
	rk_state_next = rk_state;

	assert_int = 0;
	vector = 0;
	
	clear_err = 0;
	clear_cmd = 0;
	set_done = 0;
	clear_done = 0;
	
	inc_ba = 0;
	inc_wc = 0;

	ata_rd = 0;
	ata_wr = 0;

	dma_req = 0;
	dma_rd = 0;
	dma_wr = 0;
	dma_addr = 0;
	dma_data_in = 0;
	
	case (rk_state)
	  ready:
	    begin
	       if (rkcs_cmd[0])
		 rk_state_next = init0;
	    end
	  
	  init0:
	    begin
	       ata_addr = ATA_STATUS;
	       ata_rd = 1;
	       if (ata_done &&
		   ~ata_out[IDE_STATUS_BSY] &&
		   ~ata_out[IDE_STATUS_DRQ])
		 rk_state_next = init1;
	    end

	  init1:
	    begin
	       ata_wr = 1;
	       ata_addr = ATA_DRVHEAD;
	       ata_in = 16'h0040;
	       if (ata_done)
		 rk_state_next = wait0;
	    end

	  wait0:
	    begin
	       // rk_cnt = 1;
	       // if (rk_cnt_rdy)
	       rk_state_next = init2;
	    end

	  init2:
	    begin
	       ata_addr = ATA_STATUS;
	       ata_rd = 1;
	       if (ata_done &&
		   ~ata_out[IDE_STATUS_BSY] &&
		   ~ata_out[IDE_STATUS_DRQ])
		 rk_state_next = init3;
	    end

	  init3:
	    begin
	       ata_wr = 1;
	       ata_addr = ATA_DEVCTRL;
	       ata_in = 16'h0002;		// nIEN
	       if (ata_done)
		 rk_state_next = init4;
	    end
	  
	  init4:
	    begin
	       ata_wr = 1;
	       ata_addr = ATA_SECCNT;
	       ata_in = { 8'b0, ~rkwc[15:8] + 8'd1 };
	       if (ata_done)
		 rk_state_next = init5;
	    end
	  
	  init5:
	    begin
	       ata_wr = 1;
	       ata_addr = ATA_SECNUM;
	       ata_in = {8'b0, rkda[7:0]};	// LBA[7:0]
	       if (ata_done)
		 rk_state_next = init6;
	    end

	  init6:
	    begin
	       ata_wr = 1;
	       ata_addr = ATA_CYLLOW;
	       ata_in = {8'b0, rkda[15:8]};	// LBA[15:8]
	       if (ata_done)
		 rk_state_next = init7;
	    end

	  
	  init7:
	    begin
	       ata_wr = 1;
	       ata_addr = ATA_CYLHIGH;
	       ata_in = 0;			// LBA[23:16]
	       if (ata_done)
		 rk_state_next = init8;
	    end

	  init8:
	    begin
	       ata_wr = 1;
	       ata_addr = ATA_DRVHEAD;
	       ata_in = 16'h0040;		// LBA[27:24] + LBA
	       if (ata_done)
		 rk_state_next = init9;
	    end

	  init9:
	    begin
	       ata_wr = 1;
	       ata_addr = ATA_COMMAND;
	       ata_in = rkcs_cmd[3:1] == 3'b001 ? ATA_CMD_WRITE :
			rkcs_cmd[3:1] == 3'b010 ? ATA_CMD_READ : 0;
	       if (ata_done)
		 rk_state_next = wait1;
	    end

	  wait1:
	    begin
//	       rk_cnt = 1;
//	       if (rk_cnt_rdy)
		 rk_state_next = init10;
	    end
	  
	  init10:
	    begin
	       ata_rd = 1;
	       ata_addr = ATA_ALTER;
	       if (ata_done)
		 rk_state_next = init11;
	    end
	  
	  init11:
	    begin
	       ata_rd = 1;
	       ata_addr = ATA_STATUS;

if (ata_done) $display("rk: ata_out %x", ata_out);
	       if (ata_done &&
		   ~ata_out[IDE_STATUS_BSY] &&
		   ata_out[IDE_STATUS_DRQ])
		 begin
		    if (rkcs_cmd[3:1] == 3'b001)
		      rk_state_next = write0;
		    else
		    if (rkcs_cmd[3:1] == 3'b010)
		      rk_state_next = read0;
		 end

	       if (ata_out[IDE_STATUS_ERR])
		    set_err = 1;
	       
	    end

	  read0:
	    begin
	       ata_rd = 1;
	       ata_addr = ATA_DATA;

	       if (ata_done)
		 begin
		    inc_wc = 1;
		    rk_state_next = read1;
		 end
	    end
	  
	  read1:
	    begin
	       // mem write
	       // after mem-ack, inc18 {mex,ba}, set mex bits
	       dma_req = 1;
	       dma_addr = { rkcs_mex, rkba };
	       dma_data_in = ata_out;
$display("read1: ata_out %o, dma_addr %o", ata_out, dma_addr);
			    
	       if (dma_ack)
		 begin
		    dma_wr = 1;
		    inc_ba = 1;
		    if (rkwc == 0)
		      rk_state_next = last0;
		    else
		      if (rkwc == 16'hff00)
			rk_state_next = init10;
		      else
			rk_state_next = read0;
		 end
	    end

	  write0:
	    begin
	       //mem read
	       //after mem-ack, inc wc
	       dma_req = 1;
	       dma_addr = { rkcs_mex, rkba };
	       
	       if (dma_ack)
		 begin
		    dma_rd = 1;
		    ata_in = dma_data_out;
		    inc_wc = 1;
		    rk_state_next = write1;
		 end
	    end

	  write1:
	    begin
	       // inc18 {mex,ba}, set mex bits
	       inc_ba = 1;
	       if (rkwc == 0)
		 rk_state_next = last0;
	       else
	       if (rkwc == 16'hff00)
		 rk_state_next = init10;
	       else
		 rk_state_next = write0;
	    end

	  last0:
	    begin
	       ata_rd = 1;
	       ata_addr = ATA_ALTER;
	       if (ata_done)
		 rk_state_next = last1;
	    end
	  
	  last1:
	    begin
	       ata_rd = 1;
	       ata_addr = ATA_STATUS;
	       if (ata_done)
		 rk_state_next = last2;
	    end

	  last2:
	    begin
	       if (rkcs_ie)
		 begin
		    assert_int = 1;
		    vector = 16'o220;
		 end
	       
	       clear_err = 1;
	       clear_cmd = 1;
	       set_done = 1;
	       
	       rk_state_next = last3;
	       
	    end

	  last3:
	    begin
	       if (rkcs_ie)
		 begin
		    assert_int = 1;
		    vector = 16'o220;
		 end
	       
	       rk_state_next = ready;
	    end
		 
	  default:
	    begin
	    end
	  
	endcase
     end
   
endmodule
