//
// // simulate IS61LV25616AL-10T on s3board
// debug only
//

module ram_256kx16(a, io, ce_n, ub_n, lb_n, we_n, oe_n);
   input [17:0] a;
   inout [15:0] io;
   input 	ce_n;
   input 	ub_n;
   input 	lb_n;
   input 	we_n;
   input 	oe_n;

   reg [7:0] ram_h[262143:0];
   reg [7:0] ram_l[262143:0];

   assign #1 io = { (oe_n | ub_n) ? 8'bz : ram_h[a],
		    (oe_n | lb_n) ? 8'bz : ram_l[a] };

   always @(we_n or ce_n or ub_n or lb_n or a or a or io)
     if (~we_n && ~ce_n)
       begin
	  if (0)
	    $display("ram_256kx16: %t ce_n %b ub_n %b lb_n %b we_n %b oe_n %b",
		     $time, ce_n, ub_n, lb_n, we_n, oe_n);
		   
`ifdef debug_ram_low
	  if (~ub_n && ~lb_n) $display("ram_256kx16: %t write %o <- %o", $time, a, io);
	  else
	    if (~ub_n) $display("ram_256kx16: writeh %o <- %o", a, io);
	    else
	      if (~lb_n) $display("ram_256kx16: writel %o <- %o", a, io);
`endif
	  
	  #1 if (~ub_n) ram_h[a] = io[15:8];
	  #1 if (~lb_n) ram_l[a] = io[7:0];
       end

`ifdef debug_ram_low
   always @(we_n or ce_n or ub_n or lb_n or a or a or io)
     if (we_n && ~ce_n)
       begin
	  if (0)
	    $display("ram_256kx16: %t ce_n %b ub_n %b lb_n %b we_n %b oe_n %b",
		     $time, ce_n, ub_n, lb_n, we_n, oe_n);

	  if (~ub_n && ~lb_n) $display("ram_256kx16: read %o -> %o", a, io);
	  else
	    if (~ub_n) $display("ram_256kx16: readh %o -> %o", a, io[7:0]);
	    else
	      if (~lb_n) $display("ram_256kx16: readl %o -> %o", a, io[7:0]);
       end
`endif

endmodule

module ram_s3board(ram_a, ram_oe_n, ram_we_n,
		   ram1_io, ram1_ce_n, ram1_ub_n, ram1_lb_n,
		   ram2_io, ram2_ce_n, ram2_ub_n, ram2_lb_n);
		   
   input [17:0] ram_a;
   input 	ram_oe_n, ram_we_n;
   inout [15:0] ram1_io;
   inout [15:0] ram2_io;
   input 	ram1_ce_n, ram1_ub_n, ram1_lb_n;
   input 	ram2_ce_n, ram2_ub_n, ram2_lb_n;

   // synthesis translate_off
   integer 	 i;
   reg [15:0] 	 v;
   integer 	 file;
   reg [1023:0]  str;
   reg [1023:0]  testfilename;
   integer 	 n;

   initial
     begin
	for (i = 0; i < 262144/*32768*//*8192*/; i=i+1)
	  begin
             ram1.ram_h[i] = 8'b0;
	     ram1.ram_l[i] = 8'b0;
	  end

	n = 0;
	
`ifdef __ICARUS__
 `define no_scan
`endif
`ifdef verilator
 `define no_scan
`endif

`ifdef Veritak
 	n = $value$plusargs("test=%s", testfilename);
`endif

`ifdef __CVER__
 	n = $scan$plusargs("test=", testfilename);
`endif

	$display("ram_s3board");
	
`ifdef MODELSIM
	if (n == 0)
	  begin
	     testfilename = "default.mem";
	     $display("using default file");
	     n = 1;
	  end
`endif
	
	if (n > 0)
	  begin
	     $display("ram_s3board: code filename: %s", testfilename);
	     file = $fopen(testfilename, "r");

	     while ($fscanf(file, "%o %o", i, v) > 0)
	       begin
		  if (0) $display("ram_s3board[%0o] <- %o", i, v);
		  ram1.ram_h[i/2] = v[15:8];
		  ram1.ram_l[i/2] = v[7:0];
	       end

	     $fclose(file);
	     $display("ram_s3board: done reading");
	  end
     end

`ifdef debug_ram
   always @(ram_a or ram_oe_n or ram1_ce_n or ram_we_n or ram1_io)
     begin
	if (0)
	  $display("ram_s3board: ce_n %b ub_n %b lb_n %b we_n %b oe_n %b",
		   ram1_ce_n, ram1_ub_n, ram1_lb_n, ram_we_n, ram_oe_n);

	#3 if (ram_oe_n == 0 && ram_we_n == 1)
	  $display("ram_s3board: read  [%o] -> %o %t", ram_a*2, ram1_io, $time);
	#3 if (ram_oe_n == 1 && ram_we_n == 0)
	  $display("ram_s3board: write [%o] <- %o %t", ram_a*2, ram1_io, $time);
     end
`endif

   // synthesis translate_on
   ram_256kx16 ram1(.a(ram_a), .io(ram1_io),
		    .ce_n(ram1_ce_n), .ub_n(ram1_ub_n), .lb_n(ram1_lb_n),
		    .we_n(ram_we_n), .oe_n(ram_oe_n));

   ram_256kx16 ram2(.a(ram_a), .io(ram2_io),
		    .ce_n(ram2_ce_n), .ub_n(ram2_ub_n), .lb_n(ram2_lb_n),
		    .we_n(ram_we_n), .oe_n(ram_oe_n));


endmodule
