///////////////////////////////////////////////////////////////////////////
//
// Filename:	jtagser.v
//
// Project:	XuLA2 board
//
// Purpose:	All interaction between the FPGA and the host computer on a
//		XuLA board takes place via the JTAG port and the USER 
//	instruction.  There is no serial port, such as I have had on the other
//	boards I have worked with.  (Actually, this SoC project will have a 
//	UART port, but that port will not command the bus ...)  Nevertheless, I
//	still need to convert this JTAG interface into one that looks like a
//	32-bit wishbone bus in order to be compatible with all of my other
//	work.  This module is part of that conversion.  This module turns bits
//	shifted into the JTAG, when it is in the shift-dr state, into bytes
//	plus a strobe output to the rest of the FPGA (you can handle a strobe
//	at all times, right?).  Likewise, it takes bytes in for transmitting
//	back to the host and turns them into bits sent across this port.
//
//
// Creator:	Dan Gisselquist
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
// License:	GPL, v3, as defined and found on www.gnu.org,
//		http://www.gnu.org/licenses/gpl.html
//
//
///////////////////////////////////////////////////////////////////////////
//
//
module	jtagser(i_clk,
		o_rx_stb, o_rx_data, i_tx_stb, i_tx_data, o_tx_busy);
	input		i_clk;
	//
	output	reg		o_rx_stb;
	output	reg	[7:0]	o_rx_data;
	//
	input			i_tx_stb;
	input		[7:0]	i_tx_data;
	output	reg		o_tx_busy;

	wire	w_capture, w_drck, w_reset, w_runtest, w_sel, w_shift, w_update,
		i_tck, i_tdi, i_tms;
	reg	o_tdo;
	BSCAN_SPARTAN6 #(.JTAG_CHAIN(1)) BSCANE2_inst(
		.CAPTURE(w_capture), // CAPTURE output from TAP controller
		.DRCK(w_drck),	// Gated TCK output. When SEL is asserted,
			// DRCK toggles when CAPTURE or SHIFT are asserted
		.RESET(w_reset), // RESET output from tap controller
		.RUNTEST(w_runtest),
		.SEL(w_sel),
		.SHIFT(w_shift), // SHIFT output from TAP controller
		.TCK(i_tck), // Fabric connection to TCK pin
		.TDI(i_tdi), // Fabric connection to TDI pin
		.TMS(i_tms), // Fabric connection to TMS pin
		.UPDATE(w_update), // Update output from TAP controller
		.TDO(o_tdo)); // Test Data output (TDO) input pin for user func

	reg	r_tck, ck_tck, last_tck, edge_tck, r_tdi, ck_tdi,
		r_shift, ck_shift, r_sel, ck_sel;
	initial	r_tck = 1'b0;
	initial	ck_tck = 1'b0;
	initial	last_tck = 1'b0;
	initial	edge_tck = 1'b0;
	always @(posedge i_clk)
	begin // 10 FF's, 1/2 - 1 LUT
		r_tck <= i_tck;     ck_tck <= r_tck;
		r_shift <= w_shift; ck_shift <= r_shift;
		r_sel   <= w_sel;   ck_sel <= r_sel;
		r_tdi   <= i_tdi;   ck_tdi <= r_tdi;
		//
		last_tck <= ck_tck;
		edge_tck <= (ck_tck)&&(~last_tck);
		//
	end

	reg	[2:0]	state;
	initial	state <= 0;
	always @(posedge i_clk) // Exactly 1 6-LUT and 3 FF's
		if (edge_tck)
		begin
			if ((~ck_sel)||(~ck_shift))
				state <= 0;
			else if ((ck_sel)&&(ck_shift))
				state <= state + 3'h1;
		end

	// Our receive data ... we'll ignore anything less than 8-bits, or
	// any fractions of a byte.
	always @(posedge i_clk)
		if (edge_tck)
			o_rx_data <= { ck_tdi, o_rx_data[7:1] };

	reg	nxt_clk_stb;
	initial nxt_clk_stb = 1'b0;
	always @(posedge i_clk)
		if (edge_tck)
			nxt_clk_stb <= (edge_tck)&&(state == 3'h7)&&(ck_sel)&&(ck_shift);
	initial o_rx_stb = 1'b0;
	always @(posedge i_clk)
		if (edge_tck)
			o_rx_stb <= nxt_clk_stb;
		else	o_rx_stb <=1'b0;

	reg	[7:0]	tx_buf;
	initial	o_tdo = 1'b1;
	always @(posedge i_clk) // 12 inputs, ... ? LUTs
		if (edge_tck)
			o_tdo <= tx_buf[state];

	reg	filled;
	initial	filled = 1'b0;
	always @(posedge i_clk)
	begin // 2-LUTs
		if ((i_tx_stb)&&(~o_tx_busy))
			filled <= 1'b1;
		else if ((edge_tck)&&(state == 7))
			filled <= 1'b0;
	end
	always @(posedge i_clk)
	begin // tx_busy <- 8 6-bit LUTs
		if ((i_tx_stb)&&(~o_tx_busy))
			o_tx_busy <= 1'b1;
		else if ((edge_tck)&&(state == 7))
			o_tx_busy <= 1'b0;
		else if (state == 0)
			o_tx_busy <= (filled)||((ck_sel)&&(ck_shift));
		else if (state == 7)
			o_tx_busy <= 1'b1; // filled;
		else
			o_tx_busy <= 1'b1;
	end
	initial	tx_buf = 8'hff;
	always @(posedge i_clk) // 8 FF's, 16-LUTs
		if ((i_tx_stb)&&(~o_tx_busy))
			tx_buf <= i_tx_data;
		else if ((edge_tck)&&(state == 7))
			tx_buf <= 8'hff;

endmodule

