// rk_regs.v
//
// simulated rk05 (RK11) drive
// simple state machine which talks to IDE drive
// (the idea came from pop-11, thanks!)
// copyright Brad Parker <brad@heeltoe.com> 2009
//
// special cases:
// RK05 has sectors of 256 words, or 512 bytes. 
//
//

module rk_regs (clk, reset, iopage_addr, data_in, data_out, decode,
		iopage_rd, iopage_wr, iopage_byte_op,
		interrupt, interrupt_ack, vector,
		ide_data_bus, ide_dior, ide_diow, ide_cs, ide_da,
   		dma_req, dma_ack, dma_addr, dma_data_in, dma_data_out,
		dma_rd, dma_wr, rk_state_out);
   
   input clk;
   input reset;
   input [12:0] iopage_addr;
   input [15:0] data_in;
   input 	iopage_rd, iopage_wr, iopage_byte_op;
   output [15:0] data_out;
   output 	 decode;
   output [4:0]  rk_state_out;

   output 	 interrupt;
   reg 		 interrupt;
   output [7:0]  vector;
   input 	 interrupt_ack;
   
   output 	 dma_req;
   input 	 dma_ack;
   output [17:0] dma_addr;
   output [15:0] dma_data_out;
   input [15:0]  dma_data_in;
   output 	 dma_rd;
   output 	 dma_wr;

   reg 		 dma_req;
   reg [17:0] 	 dma_addr;
   reg [15:0] 	 dma_data_out;
   reg 		 dma_rd;
   reg 		 dma_wr;

   wire [15:0] 	 rkds;
   reg [15:0] 	 rker, rkwc, rkda;
   reg [17:0] 	 rkba;

   reg [15:0] 	 rkide_reg, rkide_data;

   reg 		 rkcs_err;
   reg [3:0] 	 rkcs_cmd;
   reg 		 rkcs_done;
   reg 		 rkcs_ie;
   wire [1:0] 	 rkcs_mex;
   
   reg 	 assert_int;
   reg 	 clear_err, set_err;
   reg 	 clear_cmd;
   reg 	 set_done;
   reg 	 clear_done;
   reg 	 clear_da, clear_ba;
   
   reg inc_ba;
   reg inc_wc;

   reg [15:0] dma_data_hold;
   
   wire [15:0] 	 reg_in;
   reg [15:0] 	 reg_out;

   //
   wire [15:0] lba;
   
   parameter CSR_BIT_GO = 0;
   parameter CSR_BIT_IE = 6;
   parameter CSR_BIT_DONE = 7;

   parameter
       RKCS_CMD_CTLRESET = 3'd0,
       RKCS_CMD_WRITE = 3'd1,
       RKCS_CMD_READ = 3'd2,
       RKCS_CMD_WCHK = 3'd3,
       RKCS_CMD_SEEK = 3'd4,
       RKCS_CMD_RCHK = 3'd5,
       RKCS_CMD_DRVRESET = 3'd6,
       RKCS_CMD_WLK = 3'd7;

   parameter
       RKDS_RK05 = 16'o004000,
       RKDS_RDY  = 16'o000200,
       RKDS_RWS  = 16'o000100;
       
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
   
   parameter
       ATA_CMD_READ = 16'h0020,
       ATA_CMD_WRITE = 16'h0030;

   reg [5:0] rk_state;
   reg [5:0] rk_state_next;

   parameter [5:0]
		ready = 0,
		init0 = 1,
		init1 = 2,
		init2 = 3,
		init3 = 4,
		init4 = 5,
		init5 = 6,
		init6 = 7,
		init7 = 8,
		init8 = 9,
		init9 = 10,
		init10 = 11,
		init11 = 12,
		read0 = 13,
		read1 = 14,
		write0 = 15,
		write1 = 16,
		last0 = 17,
		last1 = 18,
		last2 = 19,
		last3 = 20,
		wait0 = 21,
		wait1 = 22,
		done0 = 30,
		done1 = 31,
		ide_wr0 = 32,
		ide_wr1 = 33,
		ide_rd0 = 34,
		ide_rd1 = 35;
   

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
   		      (iopage_addr == 13'o17412) |
      		      (iopage_addr == 13'o17414) |
      		      (iopage_addr == 13'o17416);

   assign rkcs_mex = rkba[17:16];

   //
   // 111111
   // 5432109876543210
   //   ttttttttttssss
   //
   wire [3:0] sector;
   wire [8:0] track;
   wire [10:0] trackx3;
   wire [15:0] trackx12;
   
   assign sector = rkda[3:0];
   assign track = rkda[12:4];
   
   // (track*12)+sector = 4*(track+track+track) + sector
   assign trackx3 = {2'b0, track} + {2'b0, track} + {2'b0, track};

   assign trackx12 = {3'b0, trackx3, 2'b0};

   assign lba = trackx12 + { 12'b0, sector };

   //
   assign rkds = RKDS_RK05 | RKDS_RDY | RKDS_RWS;

   // register read
   always @(clk or decode or iopage_addr or iopage_rd or iopage_byte_op or
	    rkds or rkda or rker or rkwc or rkba or
	    rkcs_err or rkcs_done or rkcs_ie or rkcs_mex or rkcs_cmd)
     begin
	if (decode/* && iopage_rd ?? */)
	  case (iopage_addr)
	    13'o17400: reg_out = rkds;
	    13'o17402: reg_out = rker;
	    13'o17404:
	      begin
		 reg_out = { rkcs_err, 7'b0,
			      rkcs_done, rkcs_ie, rkcs_mex,
			      rkcs_cmd };
`ifdef debug_rk_regs
		 if (reg_out != 16'o5)
		   $display("rk: XXX read rkcs %o",
			    { rkcs_err, 7'b0,
			      rkcs_done, rkcs_ie, rkcs_mex,
			      rkcs_cmd });
`endif
	      end
	    13'o17406: reg_out = rkwc;
	    13'o17410: reg_out = rkba[15:0];
	    13'o17412: reg_out = rkda;
	    13'o17414: reg_out = ata_out;
	    13'o17416: reg_out = rkide_data;
	    default: reg_out = 16'b0;
	  endcase
	else
	  reg_out = 16'b0;
     end

   assign data_out = iopage_byte_op ?
		     {8'b0, iopage_addr[0] ? reg_out[15:8] : reg_out[7:0]} :
		     reg_out;

   // register write
   assign reg_in = (iopage_byte_op & iopage_addr[0]) ? {8'b0, data_in[15:8]} :
		   data_in;

   // RKCS
   always @(posedge clk)
     if (reset)
       begin
	  rkcs_done <= 0;
	  rkcs_ie <= 0;
	  rkcs_cmd <= 0;
	  rkcs_err <= 0;
       end
     else
       if (iopage_wr && decode && iopage_addr == 13'o17404)
	 begin
	    rkcs_done <= data_in[7];
	    rkcs_ie <= data_in[6];
	    //rkba[17:16] registed below from data_in[5:4];
	    rkcs_cmd <= data_in[3:0];
`ifdef debug
	    $display("rk: write rkcs %o", data_in);
`endif
	 end
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
	 end

   // RKWC
   always @(posedge clk)
     if (reset)
       rkwc <= 0;
     else
       if (iopage_wr && decode && iopage_addr == 13'o17406)
	 begin
	    rkwc <= data_in;
`ifdef debug
	    $display("rk: write rkwc %o", data_in);
`endif
	 end
       else
	 if (inc_wc)
	   begin
	      rkwc <= rkwc + 16'd1;
	      if (0) $display("rk: inc wc %o", rkwc);
	   end

   // RKBA
   always @(posedge clk)
     if (reset)
       rkba <= 0;
     else
       if (iopage_wr && decode)
	 begin
	    if (iopage_addr == 13'o17410)
	      begin
		 rkba[15:0] <= data_in;
`ifdef debug
		 $display("rk: write rkba %o", data_in);
`endif
	      end

	    if (iopage_addr == 13'o17404)
	      rkba[17:16] <= data_in[5:4];
	 end
       else
	 if (inc_ba)
	   begin
	      rkba <= rkba + 18'd2;
	      if (0) $display("rk: inc ba %o", rkba);
	   end
	 else
	   if (clear_ba)
	     rkba <= 18'd0;

   // RKDA
   always @(posedge clk)
     if (reset)
       rkda <= 0;
     else
       if (iopage_wr && decode && iopage_addr == 13'o17412)
	 begin
	    rkda <= data_in;
`ifdef debug
	    $display("rk: XXX write rkda %o", data_in);
`endif
	 end
       else
   	 if (clear_da)
	   rkda <= 0;

   // RKIDE
   always @(posedge clk)
     if (reset)
       rkide_reg <= 0;
     else
       if (iopage_wr && decode && iopage_addr == 13'o17414)
	 rkide_reg <= data_in;

   always @(posedge clk)
     if (reset)
       rkide_data <= 0;
     else
       if (iopage_wr && decode && iopage_addr == 13'o17416)
	 rkide_data <= data_in;
       else
	   if (rk_state == ide_rd0 && ata_done)
	     rkide_data <= ata_out;

   assign vector = 8'o220;

   // rk state machine
   always @(posedge clk)
     if (reset)
       rk_state <= ready;
     else
       begin
	  if (rkcs_cmd[0] && rkcs_cmd[3:1] == RKCS_CMD_CTLRESET && rk_state != ready)
	    rk_state <= ready;
	  else
	    rk_state <= rk_state_next;
       end

   assign rk_state_out = rk_state[4:0];
   
   always @(posedge clk)
     if (reset)
       interrupt <= 0;
     else
       if (assert_int)
	 begin
`ifdef debug
	    $display("rk: XXX assert interrupt\n");
`endif
	    interrupt <= 1;
	 end
       else
	 if (interrupt_ack)
	   begin
	      interrupt <= 0;
`ifdef debug
	      $display("rk: XXX ack interrupt\n");
`endif
	   end

   // grab the dma'd data, later used by ide
   always @(posedge clk)
     if (reset)
       dma_data_hold <= 0;
     else
     if (rk_state == write0 && dma_ack)
       dma_data_hold <= dma_data_in;

   // combinatorial logic based on rk_state
   always @(rk_state or rkcs_cmd or rkcs_ie or 
	    rkwc or rkda or rkba or lba or
            ata_done or ata_out or
	    dma_data_in or dma_ack or dma_data_hold)
     begin
	rk_state_next = rk_state;

	assert_int = 0;
	
	clear_err = 0;
	set_err = 0;
	clear_cmd = 0;
	set_done = 0;
	clear_done = 0;

	clear_ba = 0;
	clear_da = 0;
	
	inc_ba = 0;
	inc_wc = 0;

	ata_rd = 0;
	ata_wr = 0;
	ata_addr = 0;
	ata_in = 0;
	
	dma_req = 0;
	dma_rd = 0;
	dma_wr = 0;
	dma_addr = 0;
	dma_data_out = 0;

	case (rk_state)
	  ready:
	    begin
	       if (rkcs_cmd[0] && ~rkcs_done)
		 begin
		    case (rkcs_cmd[3:1])
		      RKCS_CMD_CTLRESET:
			begin
			   clear_da = 1;
			   clear_ba = 1;
			   rk_state_next = done0;
			end
		      RKCS_CMD_DRVRESET:
			rk_state_next = done0;
		      RKCS_CMD_SEEK:
			rk_state_next = done0;
		      /*RKCS_CMD_WCHK, RKCS_CMD_RCHK, */RKCS_CMD_WLK:
			begin
`ifdef debug
			   $display("rk: unhandled command %o", rkcs_cmd);
			   $finish;
`endif
			end
		      RKCS_CMD_WCHK:
			rk_state_next = ide_wr0;
		      RKCS_CMD_RCHK:
			rk_state_next = ide_rd0;
		      default:
			rk_state_next = init0;
		      endcase
`ifdef debug
		    if (rkcs_done == 0)
		      $display("rk: XXX go! rkcs_cmd %b", rkcs_cmd);

		    $display("rk: XXX rk_state %d, rk_state_next %d, rkcs_done %b",
			     rk_state, rk_state_next, rkcs_done);
`endif
		 end
	    end
	  
	  init0:
	    begin
	       ata_addr = ATA_STATUS;
	       ata_rd = 1;
	       if (ata_done &&
		   ~ata_out[IDE_STATUS_BSY] &&
		   ata_out[IDE_STATUS_DRDY])
		 rk_state_next = init1;
`ifdef debug_rk
	       $display("rk: init0, status %x", ata_out);
`endif
	    end

	  init1:
	    begin
`ifdef debug_rk
	       $display("rk: init1");
`endif
	       ata_wr = 1;
	       ata_addr = ATA_DRVHEAD;
	       ata_in = 16'h0040;
	       if (ata_done)
		 rk_state_next = wait0;
	       //$display("rk_regs: init1, write 0040 -> drvhead");
	    end

	  wait0:
	    begin
	       // rk_cnt = 1;
	       // if (rk_cnt_rdy)
	       rk_state_next = init2;
	       //$display("rk: XXX wait0");
	    end

	  init2:
	    begin
	       ata_addr = ATA_STATUS;
	       ata_rd = 1;
	       if (ata_done &&
		   ~ata_out[IDE_STATUS_BSY] &&
		   ata_out[IDE_STATUS_DRDY])
		 rk_state_next = init3;
	    end

	  init3:
	    begin
	       //$display("rk: XXX init3");
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
	       ata_in = {8'b0, lba[7:0]};	// LBA[7:0]
	       if (ata_done)
		 rk_state_next = init6;
	    end

	  init6:
	    begin
	       ata_wr = 1;
	       ata_addr = ATA_CYLLOW;
	       ata_in = {8'b0, lba[15:8]};	// LBA[15:8]
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
	       //$display("rk_regs: init8, write 0040 -> drvhead");
	    end

	  init9:
	    begin
	       ata_wr = 1;
	       ata_addr = ATA_COMMAND;
	       ata_in = rkcs_cmd[3:1] == 3'b001 ? ATA_CMD_WRITE :
			rkcs_cmd[3:1] == 3'b010 ? ATA_CMD_READ : 16'b0;
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
	       //$display("rk: XXX init10");
	       ata_rd = 1;
	       ata_addr = ATA_ALTER;
	       if (ata_done)
		 rk_state_next = init11;
	    end
	  
	  init11:
	    begin
	       ata_rd = 1;
	       ata_addr = ATA_STATUS;

	       //if (ata_done) $display("rk: XXX init11 ata_out %x", ata_out);
	       if (ata_done && ~ata_out[IDE_STATUS_BSY])
		 begin
		    if (rkcs_cmd[3:1] == 3'b001)
		      rk_state_next = write0;
		    else
		    if (rkcs_cmd[3:1] == 3'b010 && ata_out[IDE_STATUS_DRQ])
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
	       dma_addr = rkba;
	       dma_data_out = ata_out;
	       dma_wr = 1;
	       
	       if (0) $display("read1: XXX ata_out %o, dma_addr %o",
			       ata_out, dma_addr);
			    
	       if (dma_ack)
		 begin
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
	       dma_addr = rkba;
	       dma_rd = 1;
	       
	       if (dma_ack)
		 begin
		    inc_wc = 1;
		    rk_state_next = write1;
		 end
	    end

	  write1:
	    begin
	       ata_wr = 1;
	       ata_addr = ATA_DATA;
	       ata_in = dma_data_hold;

	       if (ata_done)
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
		 begin
//$display("ata_out %x", ata_out);
		    if (ata_out[IDE_STATUS_ERR])
		      set_err = 1;
		      
		    // if buffer is not empty, flush
		    if (ata_out[IDE_STATUS_DRQ])
		      begin
			 if (rkcs_cmd[3:1] == 3'b010)
			   rk_state_next = last2;
			 else
			   if (rkcs_cmd[3:1] == 3'b001)
			     rk_state_next = last3;
		      end
		    else
		      // otherwise, we're done
		      rk_state_next = done0;
		 end
	    end

	  last2:
	    begin
	       ata_rd = 1;
	       ata_addr = ATA_DATA;

	       if (ata_done)
		 rk_state_next = last1;
	    end

	  last3:
	    begin
	       ata_wr = 1;
	       ata_addr = ATA_DATA;
	       ata_in = dma_data_hold;

	       if (ata_done)
		 rk_state_next = last1;
	    end
	  
	  done0:
	    begin
`ifdef debug
	       $display("rk: XXX done0");
`endif
	       if (rkcs_ie)
		 begin
		    assert_int = 1;
`ifdef debug
		    $display("rk: XXX last2, interrupt");
`endif
		 end
	       
	       clear_err = 1;
	       clear_cmd = 1;
	       set_done = 1;
	       
	       rk_state_next = done1;
	    end

	  done1:
	    begin
	       if (rkcs_ie)
		 begin
		    assert_int = 1;
`ifdef debug
		    $display("rk: XXX last3, interrupt");
`endif
		 end
	       
	       rk_state_next = ready;
`ifdef debug
	       $display("rk: XXX last3, done (ie %b)", rkcs_ie);
`endif
	    end

	  // ------
	  ide_wr0:
	    begin
	       ata_wr = 1;
	       ata_addr = rkide_reg[4:0];
	       ata_in = rkide_data;

	       if (ata_done)
		 begin
		    rk_state_next = ide_wr1;
		    set_done = 1;
		 end
	    end
	  
	  ide_wr1:
	    rk_state_next = ready;

	  ide_rd0:
	    begin
	       ata_rd = 1;
	       ata_addr = rkide_reg[4:0];
	       if (ata_done)
		 begin
		    rk_state_next = ide_rd1;
		    set_done = 1;
		 end
	    end

	  ide_rd1:
	    rk_state_next = ready;
	  // ------		 

	  default:
	    begin
	    end
	  
	endcase
     end
   
endmodule
