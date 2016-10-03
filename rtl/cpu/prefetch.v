////////////////////////////////////////////////////////////////////////////////
//
// Filename:	prefetch.v
//
// Project:	Zip CPU -- a small, lightweight, RISC CPU soft core
//
// Purpose:	This is a very simple instruction fetch approach.  It gets
//		one instruction at a time.  Future versions should pipeline
//		fetches and perhaps even cache results--this doesn't do that.
//		It should, however, be simple enough to get things running.
//
//		The interface is fascinating.  The 'i_pc' input wire is just
//		a suggestion of what to load.  Other wires may be loaded
//		instead. i_pc is what must be output, not necessarily input.
//
//	20150919 -- Added support for the WB error signal.  When reading an
//		instruction results in this signal being raised, the pipefetch
//		module will set an illegal instruction flag to be returned to
//		the CPU together with the instruction.  Hence, the ZipCPU
//		can trap on it if necessary.
//
// Creator:	Dan Gisselquist, Ph.D.
//		Gisselquist Technology, LLC
//
////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////
//
// Flash requires a minimum of 4 clocks per byte to read, so that would be
// 4*(4bytes/32bit word) = 16 clocks per word read---and that's in pipeline
// mode which this prefetch does not support.  In non--pipelined mode, the
// flash will require (16+6+6)*2 = 56 clocks plus 16 clocks per word read,
// or 72 clocks to fetch one instruction.
module	prefetch(i_clk, i_rst, i_ce, i_stalled_n, i_pc, i_aux,
			o_i, o_pc, o_aux, o_valid, o_illegal,
		o_wb_cyc, o_wb_stb, o_wb_we, o_wb_addr, o_wb_data,
			i_wb_ack, i_wb_stall, i_wb_err, i_wb_data);
	parameter		ADDRESS_WIDTH=32, AUX_WIDTH = 1, AW=ADDRESS_WIDTH;
	input				i_clk, i_rst, i_ce, i_stalled_n;
	input		[(AW-1):0]	i_pc;
	input	[(AUX_WIDTH-1):0]	i_aux;
	output	reg	[31:0]		o_i;
	output	reg	[(AW-1):0]	o_pc;
	output	reg [(AUX_WIDTH-1):0]	o_aux;
	output	reg			o_valid, o_illegal;
	// Wishbone outputs
	output	reg			o_wb_cyc, o_wb_stb;
	output	wire			o_wb_we;
	output	reg	[(AW-1):0]	o_wb_addr;
	output	wire	[31:0]		o_wb_data;
	// And return inputs
	input			i_wb_ack, i_wb_stall, i_wb_err;
	input		[31:0]	i_wb_data;

	assign	o_wb_we = 1'b0;
	assign	o_wb_data = 32'h0000;

	// Let's build it simple and upgrade later: For each instruction
	// we do one bus cycle to get the instruction.  Later we should
	// pipeline this, but for now let's just do one at a time.
	initial	o_wb_cyc = 1'b0;
	initial	o_wb_stb = 1'b0;
	initial	o_wb_addr= 0;
	always @(posedge i_clk)
		if ((i_rst)||(i_wb_ack))
		begin
			o_wb_cyc <= 1'b0;
			o_wb_stb <= 1'b0;
		end else if ((i_ce)&&(~o_wb_cyc)) // Initiate a bus cycle
		begin
			o_wb_cyc <= 1'b1;
			o_wb_stb <= 1'b1;
		end else if (o_wb_cyc) // Independent of ce
		begin
			if ((o_wb_cyc)&&(o_wb_stb)&&(~i_wb_stall))
				o_wb_stb <= 1'b0;
			if (i_wb_ack)
				o_wb_cyc <= 1'b0;
		end

	always @(posedge i_clk)
		if (i_rst) // Set the address to guarantee the result is invalid
			o_wb_addr <= {(AW){1'b1}};
		else if ((i_ce)&&(~o_wb_cyc))
			o_wb_addr <= i_pc;
	always @(posedge i_clk)
		if ((o_wb_cyc)&&(i_wb_ack))
			o_aux <= i_aux;
	always @(posedge i_clk)
		if ((o_wb_cyc)&&(i_wb_ack))
			o_i <= i_wb_data;
	always @(posedge i_clk)
		if ((o_wb_cyc)&&(i_wb_ack))
			o_pc <= o_wb_addr;
	initial o_valid   = 1'b0;
	initial o_illegal = 1'b0;
	always @(posedge i_clk)
		if ((o_wb_cyc)&&(i_wb_ack))
		begin
			o_valid <= (i_pc == o_wb_addr)&&(~i_wb_err);
			o_illegal <= i_wb_err;
		end else if (i_stalled_n)
		begin
			o_valid <= 1'b0;
			o_illegal <= 1'b0;
		end

endmodule
