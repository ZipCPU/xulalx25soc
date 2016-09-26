////////////////////////////////////////////////////////////////////////////////
//
// Filename:	uartdev.v
//
// Project:	XuLA2 board
//
// Purpose:	This is a simple wrapper around the txuart and rxuart
//		modules.  The purpose is to make both of those modules
//	configurable from a single wishbone address, and capable of receiving
//	(or transmitting) via reads (writes) from two other addresses.
//
//	It also generates interrupts: a receive interrupt strobe on the clock
//	when data is made available, and a transmit not busy level interrupt
//	which is held high as long as the transmitter is idle.  Both should be
//	able to work nicely with the programmable interrupt controllers found
//	in the ZipCPU project.
//
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
//
module	uartdev(i_clk, i_rx_uart, o_tx_uart,
		i_wb_cyc, i_wb_stb, i_wb_we, i_wb_addr, i_wb_data,
			o_wb_ack, o_wb_stall, o_wb_data,
		o_rx_int, o_tx_int, o_debug);
	parameter	DEFAULT_SETUP = { 2'b00, 1'b0, 1'b0, 2'b00, 24'd8333 };
	input			i_clk, i_rx_uart;
	output	wire		o_tx_uart;
	input			i_wb_cyc, i_wb_stb, i_wb_we;
	input		[1:0]	i_wb_addr;
	input		[31:0]	i_wb_data;
	output	reg		o_wb_ack;
	output	wire		o_wb_stall;
	output	reg	[31:0]	o_wb_data;
	output	wire		o_rx_int, o_tx_int;
	output	wire	[31:0]	o_debug;

	reg	[29:0]	r_setup;
	reg		r_tx_stb, rx_rdy;
	reg	[7:0]	r_tx_data;
	initial	r_setup = DEFAULT_SETUP;
	always @(posedge i_clk)
		if ((i_wb_stb)&&(i_wb_we)&&(~i_wb_addr[1]))
			r_setup <= i_wb_data[29:0];

	initial r_tx_stb = 1'b0;
	always @(posedge i_clk)
		if ((i_wb_stb)&&(i_wb_we)&&(i_wb_addr == 2'b11))
		begin
			// Note: there's no check for overflow here.
			// You're on your own: verify that the device
			// isn't busy first.
			r_tx_data <= i_wb_data[7:0];
			r_tx_stb <= 1'b1;
		end else
			r_tx_stb <= 1'b0;

	wire	rx_stb, rx_break, rx_parity_err, rx_frame_err;
	wire	[7:0]	rx_data;
	rxuart	rxmod(i_clk, 1'b0, r_setup, i_rx_uart, 
			rx_stb, rx_data, rx_break,
			rx_parity_err, rx_frame_err);

	wire	tx_break, tx_busy;
	assign	tx_break = 1'b0;
	txuart	txmod(i_clk, 1'b0, r_setup, tx_break, r_tx_stb, r_tx_data,
			o_tx_uart, tx_busy);

	reg	[7:0]	r_data;
	always @(posedge i_clk)
		if (rx_stb)
			r_data <= rx_data;

	initial	o_wb_data = 32'h00;
	always @(posedge i_clk)
	begin
		if (rx_stb)
			rx_rdy <= (rx_rdy | rx_stb);

		case(i_wb_addr)
		2'b00: o_wb_data <= { 2'b00, r_setup };
		2'b01: o_wb_data <= { 2'b00, r_setup };
		2'b10: begin
			if ((i_wb_stb)&&(~i_wb_we))
				rx_rdy <= rx_stb;
			o_wb_data <= { 20'h00, rx_break, rx_frame_err, rx_parity_err, ~rx_rdy, r_data };
			end
		2'b11: o_wb_data <= { 31'h00,tx_busy };
		endcase
		o_wb_ack <= (i_wb_stb); // Read or write, we ack
	end

	assign	o_wb_stall = 1'b0;
	assign	o_rx_int = rx_stb;
	assign	o_tx_int = ~tx_busy;

	assign	o_debug = { (~i_rx_uart)||(~o_tx_uart), tx_busy, i_wb_addr,
			rx_break, rx_frame_err, rx_parity_err, rx_rdy,
			i_wb_cyc, i_wb_stb, i_wb_we, o_wb_ack,
			rx_stb, rx_data,
			r_tx_stb, r_tx_data,
			i_rx_uart, o_tx_uart };	
endmodule
