///////////////////////////////////////////////////////////////////////////
//
// Filename: 	wbsdram.v
//
// Project:	XuLA2 board
//
// Purpose:	Provide 32-bit wishbone access to the SDRAM memory on a XuLA2
//		LX-25 board.  Specifically, on each access, the controller will
//	activate an appropriate bank of RAM (the SDRAM has four banks), and
//	then issue the read/write command.  In the case of walking off the
//	bank, the controller will activate the next bank before you get to it.
//	Upon concluding any wishbone access, all banks will be precharged and
//	returned to idle.
//
//	This particular implementation represents a second generation version
//	because my first version was too complex.  To speed things up, this
//	version includes an extra wait state where the wishbone inputs are
//	clocked into a flip flop before any action is taken on them.
//
// Creator:	Dan Gisselquist, Ph.D.
//		Gisselquist Technology, LLC
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2015, Gisselquist Technology, LLC
//
// This program is free software (firmware): you can redistribute it and/or
// modify it under the terms of  the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or (at
// your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTIBILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program.  (It's in the $(ROOT)/doc directory, run make with no
// target there if the PDF file isn't present.)  If not, see
// <http://www.gnu.org/licenses/> for a copy.
//
// License:	GPL, v3, as defined and found on www.gnu.org,
//		http://www.gnu.org/licenses/gpl.html
//
//
/////////////////////////////////////////////////////////////////////////////
//
`define	DMOD_GETINPUT	1'b0
`define	DMOD_PUTOUTPUT	1'b1
`define	RAM_OPERATIONAL	2'b00
`define	RAM_POWER_UP	2'b01
`define	RAM_SET_MODE	2'b10
`define	RAM_INITIAL_REFRESH	2'b11

module	wbsdram(i_clk,
		i_wb_cyc, i_wb_stb, i_wb_we, i_wb_addr, i_wb_data,
			o_wb_ack, o_wb_stall, o_wb_data,
		o_ram_cs_n, o_ram_cke, o_ram_ras_n, o_ram_cas_n, o_ram_we_n, 
			o_ram_bs, o_ram_addr,
			o_ram_dmod, i_ram_data, o_ram_data, o_ram_dqm,
		o_debug);
	parameter	RDLY = 6;
	input			i_clk;
	// Wishbone
	//	inputs
	input			i_wb_cyc, i_wb_stb, i_wb_we;
	input		[22:0]	i_wb_addr;
	input		[31:0]	i_wb_data;
	//	outputs
	output	wire		o_wb_ack;
	output	reg		o_wb_stall;
	output	wire [31:0]	o_wb_data;
	// SDRAM control
	output	wire		o_ram_cke;
	output	reg		o_ram_cs_n,
				o_ram_ras_n, o_ram_cas_n, o_ram_we_n;
	output	reg	[1:0]	o_ram_bs;
	output	reg	[12:0]	o_ram_addr;
	output	reg		o_ram_dmod;
	input		[15:0]	i_ram_data;
	output	reg	[15:0]	o_ram_data;
	output	reg	[1:0]	o_ram_dqm;
	output	wire	[31:0]	o_debug;


	// Calculate some metrics

	//
	// First, do we *need* a refresh now --- i.e., must we break out of
	// whatever we are doing to issue a refresh command?
	//
	// The step size here must be such that 8192 charges may be done in
	// 64 ms.  Thus for a clock of:
	//	ClkRate(MHz)	(64ms/1000(ms/s)*ClkRate)/8192
	//	100 MHz		781
	//	 96 MHz		750
	//	 92 MHz		718
	//	 88 MHz		687
	//	 84 MHz		656
	//	 80 MHz		625
	//
	// However, since we do two refresh cycles everytime we need a refresh,
	// this standard is close to overkill--but we'll use it anyway.  At
	// some later time we should address this, once we are entirely
	// convinced that the memory is otherwise working without failure.  Of
	// course, at that time, it may no longer be a priority ...
	//
	reg		need_refresh;
	reg	[9:0]	refresh_clk;
	wire	refresh_cmd;
	assign	refresh_cmd = (~o_ram_cs_n)&&(~o_ram_ras_n)&&(~o_ram_cas_n)&&(o_ram_we_n);
	initial	refresh_clk = 0;
	always @(posedge i_clk)
	begin
		if (refresh_cmd)
			refresh_clk <= 10'd625; // Make suitable for 80 MHz clk
		else if (|refresh_clk)
			refresh_clk <= refresh_clk - 10'h1;
	end
	initial	need_refresh = 1'b0;
	always @(posedge i_clk)
		need_refresh <= (refresh_clk == 10'h00)&&(~refresh_cmd);

	reg	in_refresh;
	reg	[2:0]	in_refresh_clk;
	initial	in_refresh_clk = 3'h0;
	always @(posedge i_clk)
		if (refresh_cmd)
			in_refresh_clk <= 3'h6;
		else if (|in_refresh_clk)
			in_refresh_clk <= in_refresh_clk - 3'h1;
	always @(posedge i_clk)
		in_refresh <= (in_refresh_clk != 3'h0)||(refresh_cmd);


	reg	[2:0]	bank_active	[0:3];
	reg	[(RDLY-1):0]	r_barrell_ack;
	reg		r_pending;
	reg		r_we;
	reg	[22:0]	r_addr;
	reg	[31:0]	r_data;
	reg	[12:0]	bank_row	[0:3];


	//
	// Second, do we *need* a precharge now --- must be break out of 
	// whatever we are doing to issue a precharge command?
	//
	// Keep in mind, the number of clocks to wait has to be reduced by
	// the amount of time it may take us to go into a precharge state.
	// You may also notice that the precharge requirement is tighter
	// than this one, so ... perhaps this isn't as required?
	//
`ifdef	PRECHARGE_COUNTERS
	/*
	 *
	 * I'm commenting this out.  As long as we are doing one refresh
	 * cycle every 625 (or even 1250) clocks, and as long as that refresh
	 * cycle requires that all banks be precharged, then we will never run
	 * out of the maximum active to precharge command period.
	 *
	 * If the logic isn't needed, then, let's get rid of it.
	 *
	 */
	reg	[3:0]	need_precharge;
	genvar	k;
	generate
	for(k=0; k<4; k=k+1)
	begin : precharge_genvar_loop
		wire	precharge_cmd;
		assign	precharge_cmd = ((~o_ram_cs_n)&&(~o_ram_ras_n)&&(o_ram_cas_n)&&(~o_ram_we_n)
				&&((o_ram_addr[10])||(o_ram_bs == k[1:0])))
			// Also on read or write with precharge
			||(~o_ram_cs_n)&&(o_ram_ras_n)&&(~o_ram_cas_n)&&(o_ram_addr[10]);

		reg	[9:0]	precharge_clk;
		initial	precharge_clk = 0;
		always @(posedge i_clk)
		begin
			if ((precharge_cmd)||(bank_active[k] == 0))
				// This needs to be 100_000 ns, or 10_000
				// clocks.  A value of 1000 is *highly*
				// conservative.
				precharge_clk <= 10'd1000;
			else if (|precharge_clk)
				precharge_clk <= precharge_clk - 10'h1;
		end
		initial need_precharge[k] = 1'b0;
		always @(posedge i_clk)
			need_precharge[k] <= ~(|precharge_clk);
	end // precharge_genvar_loop
	endgenerate
`else
	wire	[3:0]	need_precharge;
	assign	need_precharge = 4'h0;
`endif



	reg	[15:0]	clocks_til_idle;
	reg	[1:0]	r_state;
	wire		bus_cyc;
	assign	bus_cyc  = ((i_wb_cyc)&&(i_wb_stb)&&(~o_wb_stall));
	reg	nxt_dmod;

	// Pre-process pending operations
	wire	pending;
	initial	r_pending = 1'b0;
	reg	[22:5]	fwd_addr;
	always @(posedge i_clk)
		if (bus_cyc)
		begin
			r_pending <= 1'b1;
			r_we      <= i_wb_we;
			r_addr    <= i_wb_addr;
			r_data    <= i_wb_data;
			fwd_addr  <= i_wb_addr[22:5] + 18'h01;
		end else if ((~o_ram_cs_n)&&(o_ram_ras_n)&&(~o_ram_cas_n))
			r_pending <= 1'b0;
		else if (~i_wb_cyc)
			r_pending <= 1'b0;

	reg	r_bank_valid;
	initial	r_bank_valid = 1'b0;
	always @(posedge i_clk)
		if (bus_cyc)
			r_bank_valid <=((bank_active[i_wb_addr[9:8]][2])
					&&(bank_row[i_wb_addr[9:8]]==r_addr[22:10]));
		else
			r_bank_valid <= ((bank_active[r_addr[9:8]][2])
					&&(bank_row[r_addr[9:8]]==r_addr[22:10]));
	reg	fwd_bank_valid;
	initial	fwd_bank_valid = 0;
	always @(posedge i_clk)
		fwd_bank_valid <= ((bank_active[fwd_addr[9:8]][2])
					&&(bank_row[fwd_addr[9:8]]==fwd_addr[22:10]));
			
	assign	pending = (r_pending)&&(o_wb_stall);

	// Address MAP:
	//	23-bits bits in, 24-bits out
	//
	//	222 1111 1111 1100 0000 0000
	//	210 9876 5432 1098 7654 3210
	//	rrr rrrr rrrr rrBB cccc cccc 0
	//	                   8765 4321 0
	//
	initial r_barrell_ack = 0;
	initial	r_state = `RAM_POWER_UP;
	initial	clocks_til_idle = 16'd20500;
	initial o_wb_stall = 1'b1;
	initial	o_ram_dmod = `DMOD_GETINPUT;
	initial	nxt_dmod = `DMOD_GETINPUT;
	initial o_ram_cs_n  = 1'b0;
	initial o_ram_ras_n = 1'b1;
	initial o_ram_cas_n = 1'b1;
	initial o_ram_we_n  = 1'b1;
	initial	o_ram_dqm   = 2'b11;
	assign	o_ram_cke   = 1'b1;
	initial bank_active[0] = 3'b000;
	initial bank_active[1] = 3'b000;
	initial bank_active[2] = 3'b000;
	initial bank_active[3] = 3'b000;
	always @(posedge i_clk)
	if (r_state == `RAM_OPERATIONAL)
	begin
		o_wb_stall <= (r_pending)||(bus_cyc);
		r_barrell_ack <= r_barrell_ack >> 1;
		nxt_dmod <= `DMOD_GETINPUT;
		o_ram_dmod <= nxt_dmod;

		//
		// We assume that, whatever state the bank is in, that it
		// continues in that state and set up a series of shift
		// registers to contain that information.  If it will not
		// continue in that state, all that therefore needs to be 
		// done is to set bank_active[?][2] below.
		//
		bank_active[0] <= { bank_active[0][2], bank_active[0][2:1] };
		bank_active[1] <= { bank_active[1][2], bank_active[1][2:1] };
		bank_active[2] <= { bank_active[2][2], bank_active[2][2:1] };
		bank_active[3] <= { bank_active[3][2], bank_active[3][2:1] };
		//
		o_ram_cs_n <= (~i_wb_cyc);
		// o_ram_cke  <= 1'b1;
		o_ram_dqm  <= 2'b0;
		if (|clocks_til_idle[2:0])
			clocks_til_idle[2:0] <= clocks_til_idle[2:0] - 3'h1;

		// Default command is a
		//	NOOP if (i_wb_cyc)
		//	Device deselect if (~i_wb_cyc)
		// o_ram_cs_n  <= (~i_wb_cyc) above, NOOP
		o_ram_ras_n <= 1'b1;
		o_ram_cas_n <= 1'b1;
		o_ram_we_n  <= 1'b1;

		// o_ram_data <= r_data[15:0];

		if (nxt_dmod)
			; 
		else
		if ((~i_wb_cyc)||(|need_precharge)||(need_refresh))
		begin // Issue a precharge all command (if any banks are open),
		// otherwise an autorefresh command
			if ((bank_active[0][2:1]==2'b10)
					||(bank_active[1][2:1]==2'b10)
					||(bank_active[2][2:1]==2'b10)
					||(bank_active[3][2:1]==2'b10)
				||(|clocks_til_idle[2:0]))
			begin
				// Do nothing this clock
				// Can't precharge a bank immediately after
				// activating it
			end else if (bank_active[0][2]
				||(bank_active[1][2])
				||(bank_active[2][2])
				||(bank_active[3][2]))
			begin  // Close all active banks
				o_ram_cs_n  <= 1'b0;
				o_ram_ras_n <= 1'b0;
				o_ram_cas_n <= 1'b1;
				o_ram_we_n  <= 1'b0;
				o_ram_addr[10] <= 1'b1;
				bank_active[0][2] <= 1'b0;
				bank_active[1][2] <= 1'b0;
				bank_active[2][2] <= 1'b0;
				bank_active[3][2] <= 1'b0;
			end else if ((|bank_active[0])
					||(|bank_active[1])
					||(|bank_active[2])
					||(|bank_active[3]))
				// Can't precharge yet, the bus is still busy
			begin end else if ((~in_refresh)&&((refresh_clk[9:8]==2'b00)||(need_refresh)))
			begin // Send autorefresh command
				o_ram_cs_n  <= 1'b0;
				o_ram_ras_n <= 1'b0;
				o_ram_cas_n <= 1'b0;
				o_ram_we_n  <= 1'b1;
			end // Else just send NOOP's, the default command
		// end else if (nxt_dmod)
		// begin
			// Last half of a two cycle write
			// o_ram_data <= r_data[15:0];
			//
			// While this does need to take precedence over all
			// other commands, it doesn't need to take precedence
			// over the the deactivate/precharge commands from
			// above.
			//
			// We could probably even speed ourselves up a touch
			// by moving this condition down below activating
			// and closing active banks. ... only problem is when I
			// last tried that I broke everything so ... that's not
			// my problem.
		end else if (in_refresh)
		begin
			// NOOPS only here, until we are out of refresh
		end else if ((pending)&&(~r_bank_valid)&&(bank_active[r_addr[9:8]]==3'h0))
		begin // Need to activate the requested bank
			o_ram_cs_n  <= 1'b0;
			o_ram_ras_n <= 1'b0;
			o_ram_cas_n <= 1'b1;
			o_ram_we_n  <= 1'b1;
			o_ram_addr  <= r_addr[22:10];
			o_ram_bs    <= r_addr[9:8];
			// clocks_til_idle[2:0] <= 1;
			bank_active[r_addr[9:8]][2] <= 1'b1;
			bank_row[r_addr[9:8]] <= r_addr[22:10];
			//
		end else if ((pending)&&(~r_bank_valid)
				&&(bank_active[r_addr[9:8]]==3'b111))
		begin // Need to close an active bank
			o_ram_cs_n  <= 1'b0;
			o_ram_ras_n <= 1'b0;
			o_ram_cas_n <= 1'b1;
			o_ram_we_n  <= 1'b0;
			// o_ram_addr  <= r_addr[22:10];
			o_ram_addr[10]<= 1'b0;
			o_ram_bs    <= r_addr[9:8];
			// clocks_til_idle[2:0] <= 1;
			bank_active[r_addr[9:8]][2] <= 1'b0;
			// bank_row[r_addr[9:8]] <= r_addr[22:10];
		end else if ((pending)&&(~r_we)
				&&(bank_active[r_addr[9:8]][2])
				&&(r_bank_valid)
				&&(clocks_til_idle[2:0] < 4))
		begin // Issue the read command
			o_ram_cs_n  <= 1'b0;
			o_ram_ras_n <= 1'b1;
			o_ram_cas_n <= 1'b0;
			o_ram_we_n  <= 1'b1;
			o_ram_addr  <= { 4'h0, r_addr[7:0], 1'b0 };
			o_ram_bs    <= r_addr[9:8];
			clocks_til_idle[2:0] <= 4;

			o_wb_stall <= 1'b0;
			r_barrell_ack[(RDLY-1)] <= 1'b1;
		end else if ((pending)&&(r_we)
			&&(bank_active[r_addr[9:8]][2])
			&&(r_bank_valid)
			&&(clocks_til_idle[2:0] == 0))
		begin // Issue the write command
			o_ram_cs_n  <= 1'b0;
			o_ram_ras_n <= 1'b1;
			o_ram_cas_n <= 1'b0;
			o_ram_we_n  <= 1'b0;
			o_ram_addr  <= { 4'h0, r_addr[7:0], 1'b0 };
			o_ram_bs    <= r_addr[9:8];
			clocks_til_idle[2:0] <= 3'h1;

			o_wb_stall <= 1'b0;
			r_barrell_ack[1] <= 1'b1;
			// o_ram_data <= r_data[31:16];
			//
			o_ram_dmod <= `DMOD_PUTOUTPUT;
			nxt_dmod <= `DMOD_PUTOUTPUT;
		end else if ((r_pending)&&(r_addr[7:0] >= 8'hf0)
				&&(~fwd_bank_valid))
		begin
			// Do I need to close the next bank I'll need?
			if (bank_active[fwd_addr[9:8]][2:1]==2'b11)
			begin // Need to close the bank first
				o_ram_cs_n <= 1'b0;
				o_ram_ras_n <= 1'b0;
				o_ram_cas_n <= 1'b1;
				o_ram_we_n  <= 1'b0;
				o_ram_addr[10] <= 1'b0;
				o_ram_bs       <= fwd_addr[9:8];
				bank_active[fwd_addr[9:8]][2] <= 1'b0;
			end else if (bank_active[fwd_addr[9:8]]==3'b000)
			begin
				// Need to (pre-)activate the next bank
				o_ram_cs_n  <= 1'b0;
				o_ram_ras_n <= 1'b0;
				o_ram_cas_n <= 1'b1;
				o_ram_we_n  <= 1'b1;
				o_ram_addr  <= fwd_addr[22:10];
				o_ram_bs    <= fwd_addr[9:8];
				// clocks_til_idle[3:0] <= 1;
				bank_active[fwd_addr[9:8]] <= 3'h4;
				bank_row[fwd_addr[9:8]] <= fwd_addr[22:10];
			end
		end
	end else if (r_state == `RAM_POWER_UP)
	begin
		// All signals must be held in NOOP state during powerup
		o_ram_dqm <= 2'b11;
		// o_ram_cke <= 1'b1;
		o_ram_cs_n  <= 1'b0;
		o_ram_ras_n <= 1'b1;
		o_ram_cas_n <= 1'b1;
		o_ram_we_n  <= 1'b1;
		o_ram_dmod  <= `DMOD_GETINPUT;
		if (clocks_til_idle == 0)
		begin
			r_state <= `RAM_INITIAL_REFRESH;
			clocks_til_idle[3:0] <= 4'ha;
			o_ram_cs_n  <= 1'b0;
			o_ram_ras_n <= 1'b0;
			o_ram_cas_n <= 1'b1;
			o_ram_we_n  <= 1'b0;
			o_ram_addr[10] <= 1'b1;
		end else
			clocks_til_idle <= clocks_til_idle - 16'h01;

		o_wb_stall  <= 1'b1;
		r_barrell_ack[(RDLY-1):0] <= 0;
	end else if (r_state == `RAM_INITIAL_REFRESH)
	begin
		//
		o_ram_cs_n  <= 1'b0;
		o_ram_ras_n <= 1'b0;
		o_ram_cas_n <= 1'b0;
		o_ram_we_n  <= 1'b1;
		o_ram_dmod  <= `DMOD_GETINPUT;
		o_ram_addr  <= { 3'b000, 1'b0, 2'b00, 3'b010, 1'b0, 3'b001 };
		if (clocks_til_idle[3:0] == 4'h0)
		begin
			r_state <= `RAM_SET_MODE;
			o_ram_we_n <= 1'b0;
			clocks_til_idle[3:0] <= 4'h2;
		end else
			clocks_til_idle[3:0] <= clocks_til_idle[3:0] - 4'h1;

		o_wb_stall  <= 1'b1;
		r_barrell_ack[(RDLY-1):0] <= 0;
	end else if (r_state == `RAM_SET_MODE)
	begin
		// Set mode cycle
		o_ram_cs_n  <= 1'b1;
		o_ram_ras_n <= 1'b0;
		o_ram_cas_n <= 1'b0;
		o_ram_we_n  <= 1'b0;
		o_ram_dmod  <= `DMOD_GETINPUT;

		if (clocks_til_idle[3:0] == 4'h0)
			r_state <= `RAM_OPERATIONAL;
		else
			clocks_til_idle[3:0] <= clocks_til_idle[3:0]-4'h1;

		o_wb_stall  <= 1'b1;
		r_barrell_ack[(RDLY-1):0] <= 0;
	end

	always @(posedge i_clk)
		if (nxt_dmod)
			o_ram_data <= r_data[15:0];
		else
			o_ram_data <= r_data[31:16];

`ifdef	VERILATOR
	// While I hate to build something that works one way under Verilator
	// and another way in practice, this really isn't that.  The problem
	// \/erilator is having is resolved in toplevel.v---one file that
	// \/erilator doesn't implement.  In toplevel.v, there's not only a
	// single clocked latch but two taking place.  Here, we replicate one
	// of those.  The second takes place (somehow) within the sdramsim.cpp
	// file.
	reg	[15:0]	ram_data, last_ram_data;
	always @(posedge i_clk)
		ram_data <= i_ram_data;
	always @(posedge i_clk)
		last_ram_data <= ram_data;
`else
	reg	[15:0]	last_ram_data;
	always @(posedge i_clk)
		last_ram_data <= i_ram_data;
`endif
	assign	o_wb_ack  = r_barrell_ack[0];
	assign	o_wb_data = { last_ram_data, i_ram_data };

	//
	// The following outputs are not necessary for the functionality of
	// the SDRAM, but they can be used to feed an external "scope" to
	// get an idea of what the internals of this SDRAM are doing.
	//
	// Just be aware of the r_we: it is set based upon the currently pending
	// transaction, or (if none is pending) based upon the last transaction.
	// If you want to capture the first value "written" to the device,
	// you'll need to write a nothing value to the device to set r_we.
	// The first value "written" to the device can be caught in the next
	// interaction after that.
	//
	reg	trigger;
	always @(posedge i_clk)
		trigger <= ((o_wb_data[15:0]==o_wb_data[31:16])
			&&(o_wb_ack)&&(~i_wb_we));


	assign	o_debug = { i_wb_cyc, i_wb_stb, i_wb_we, o_wb_ack, o_wb_stall, // 5
		o_ram_cs_n, o_ram_ras_n, o_ram_cas_n, o_ram_we_n, o_ram_bs,//6
			o_ram_dmod, r_pending, 				//  2
			trigger,					//  1
			o_ram_addr[9:0],				// 10 more
			(r_we) ? { o_ram_data[7:0] }			//  8 values
				: { o_wb_data[23:20], o_wb_data[3:0] }
			// i_ram_data[7:0]
			 };
endmodule
