///////////////////////////////////////////////////////////////////////////
//
// Filename:	pipemem.v
//
// Project:	Zip CPU -- a small, lightweight, RISC CPU soft core
//
// Purpose:	A memory unit to support a CPU, this time one supporting
//		pipelined wishbone memory accesses.  The goal is to be able
//	to issue one pipelined wishbone access per clock, and (given the memory
//	is fast enough) to be able to read the results back at one access per
//	clock.  This renders on-chip memory fast enough to handle single cycle
//	(pipelined) access.
//
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
// License:	GPL, v3, as defined and found on www.gnu.org,
//		http://www.gnu.org/licenses/gpl.html
//
//
///////////////////////////////////////////////////////////////////////////
//
module	pipemem(i_clk, i_rst, i_pipe_stb, i_lock,
		i_op, i_addr, i_data, i_oreg,
			o_busy, o_pipe_stalled, o_valid, o_err, o_wreg, o_result,
		o_wb_cyc_gbl, o_wb_cyc_lcl,
			o_wb_stb_gbl, o_wb_stb_lcl,
			o_wb_we, o_wb_addr, o_wb_data,
		i_wb_ack, i_wb_stall, i_wb_err, i_wb_data);
	parameter	ADDRESS_WIDTH=24, IMPLEMENT_LOCK=0, AW=ADDRESS_WIDTH;
	input			i_clk, i_rst;
	input			i_pipe_stb, i_lock;
	// CPU interface
	input			i_op;
	input		[31:0]	i_addr;
	input		[31:0]	i_data;
	input		[4:0]	i_oreg;
	// CPU outputs
	output	wire		o_busy;
	output	wire		o_pipe_stalled;
	output	reg		o_valid;
	output	reg		o_err;
	output	reg	[4:0]	o_wreg;
	output	reg	[31:0]	o_result;
	// Wishbone outputs
	output	wire		o_wb_cyc_gbl;
	output	reg		o_wb_stb_gbl;
	output	wire		o_wb_cyc_lcl;
	output	reg		o_wb_stb_lcl, o_wb_we;
	output	reg	[(AW-1):0]	o_wb_addr;
	output	reg	[31:0]	o_wb_data;
	// Wishbone inputs
	input			i_wb_ack, i_wb_stall, i_wb_err;
	input		[31:0]	i_wb_data;

	reg	cyc;
	reg			r_wb_cyc_gbl, r_wb_cyc_lcl;
	reg	[3:0]		rdaddr, wraddr;
	wire	[3:0]		nxt_rdaddr;
	reg	[(5-1):0]	fifo_oreg [0:15];
	initial	rdaddr = 0;
	initial	wraddr = 0;
	always @(posedge i_clk)
		fifo_oreg[wraddr] <= i_oreg;
	always @(posedge i_clk)
		if ((i_rst)||(i_wb_err))
			wraddr <= 0;
		else if (i_pipe_stb)
			wraddr <= wraddr + 4'h1;
	always @(posedge i_clk)
		if ((i_rst)||(i_wb_err))
			rdaddr <= 0;
		else if ((i_wb_ack)&&(cyc))
			rdaddr <= rdaddr + 4'h1;
	assign	nxt_rdaddr = rdaddr + 4'h1;

	wire	gbl_stb, lcl_stb;
	assign	lcl_stb = (i_addr[31:8]==24'hc00000)&&(i_addr[7:5]==3'h0);
	assign	gbl_stb = (~lcl_stb);
			//= ((i_addr[31:8]!=24'hc00000)||(i_addr[7:5]!=3'h0));

	initial	cyc = 0;
	initial	r_wb_cyc_lcl = 0;
	initial	r_wb_cyc_gbl = 0;
	always @(posedge i_clk)
		if (i_rst)
		begin
			r_wb_cyc_gbl <= 1'b0;
			r_wb_cyc_lcl <= 1'b0;
			o_wb_stb_gbl <= 1'b0;
			o_wb_stb_lcl <= 1'b0;
			cyc <= 1'b0;
		end else if (cyc)
		begin
			if ((~i_wb_stall)&&(~i_pipe_stb))
			begin
				o_wb_stb_gbl <= 1'b0;
				o_wb_stb_lcl <= 1'b0;
			// end else if ((i_pipe_stb)&&(~i_wb_stall))
			// begin
				// o_wb_addr <= i_addr[(AW-1):0];
				// o_wb_data <= i_data;
			end

			if (((i_wb_ack)&&(nxt_rdaddr == wraddr))||(i_wb_err))
			begin
				r_wb_cyc_gbl <= 1'b0;
				r_wb_cyc_lcl <= 1'b0;
				cyc <= 1'b0;
			end
		end else if (i_pipe_stb) // New memory operation
		begin // Grab the wishbone
			r_wb_cyc_lcl <= lcl_stb;
			r_wb_cyc_gbl <= gbl_stb;
			o_wb_stb_lcl <= lcl_stb;
			o_wb_stb_gbl <= gbl_stb;
			cyc <= 1'b1;
			// o_wb_addr <= i_addr[(AW-1):0];
			// o_wb_data <= i_data;
			// o_wb_we <= i_op
		end
	always @(posedge i_clk)
		if ((cyc)&&(i_pipe_stb)&&(~i_wb_stall))
		begin
			o_wb_addr <= i_addr[(AW-1):0];
			o_wb_data <= i_data;
		end else if ((~cyc)&&(i_pipe_stb))
		begin
			o_wb_addr <= i_addr[(AW-1):0];
			o_wb_data <= i_data;
		end
			
	always @(posedge i_clk)
		if ((i_pipe_stb)&&(~cyc))
			o_wb_we   <= i_op;

	initial	o_valid = 1'b0;
	always @(posedge i_clk)
		o_valid <= (cyc)&&(i_wb_ack)&&(~o_wb_we);
	initial	o_err = 1'b0;
	always @(posedge i_clk)
		o_err <= (cyc)&&(i_wb_err);
	assign	o_busy = cyc;

	always @(posedge i_clk)
		o_wreg <= fifo_oreg[rdaddr];
	always @(posedge i_clk)
		// if (i_wb_ack) isn't necessary, since o_valid won't be true
		// then either.
		o_result <= i_wb_data;

	assign	o_pipe_stalled = (cyc)
			&&((i_wb_stall)||((~o_wb_stb_lcl)&&(~o_wb_stb_gbl)));

	generate
	if (IMPLEMENT_LOCK != 0)
	begin
		reg	lock_gbl, lock_lcl;

		initial	lock_gbl = 1'b0;
		initial	lock_lcl = 1'b0;
		always @(posedge i_clk)
		begin
			lock_gbl <= (i_lock)&&((r_wb_cyc_gbl)||(lock_gbl));
			lock_lcl <= (i_lock)&&((r_wb_cyc_lcl)||(lock_gbl));
		end

		assign	o_wb_cyc_gbl = (r_wb_cyc_gbl)||(lock_gbl);
		assign	o_wb_cyc_lcl = (r_wb_cyc_lcl)||(lock_lcl);

	end else begin
		assign	o_wb_cyc_gbl = (r_wb_cyc_gbl);
		assign	o_wb_cyc_lcl = (r_wb_cyc_lcl);
	end endgenerate

endmodule
