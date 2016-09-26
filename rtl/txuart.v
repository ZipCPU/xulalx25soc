////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	txuart.v
//
// Project:	XuLA2 board
//
// Purpose:	Transmit outputs over a single UART line.
//
//	To interface with this module, connect it to your system clock,
//	pass it the 32 bit setup register (defined below) and the byte
//	of data you wish to transmit.  Strobe the i_wr line high for one
//	clock cycle, and your data will be off.  Wait until the 'o_busy'
//	line is low before strobing the i_wr line again--this implementation
//	has NO BUFFER, so strobing i_wr while the core is busy will just
//	cause your data to be lost.  The output will be placed on the o_txuart
//	output line.  If you wish to set/send a break condition, assert the
//	i_break line otherwise leave it low.
//
//	There is a synchronous reset line, logic high.
//
//	Now for the setup register.  The register is 32 bits, so that this
//	UART may be set up over a 32-bit bus.
//
//	i_setup[29:28]	Indicates the number of data bits per word.  This will
//	either be 2'b00 for an 8-bit word, 2'b01 for a 7-bit word, 2'b10
//	for a six bit word, or 2'b11 for a five bit word.
//
//	i_setup[27]	Indicates whether or not to use one or two stop bits.
//		Set this to one to expect two stop bits, zero for one.
//
//	i_setup[26]	Indicates whether or not a parity bit exists.  Set this
//		to 1'b1 to include parity.
//
//	i_setup[25]	Indicates whether or not the parity bit is fixed.  Set
//		to 1'b1 to include a fixed bit of parity, 1'b0 to allow the
//		parity to be set based upon data.  (Both assume the parity
//		enable value is set.)
//
//	i_setup[24]	This bit is ignored if parity is not used.  Otherwise,
//		in the case of a fixed parity bit, this bit indicates whether
//		mark (1'b1) or space (1'b0) parity is used.  Likewise if the
//		parity is not fixed, a 1'b1 selects even parity, and 1'b0
//		selects odd.
//
//	i_setup[23:0]	Indicates the speed of the UART in terms of clocks.
//		So, for example, if you have a 200 MHz clock and wish to
//		run your UART at 9600 baud, you would take 200 MHz and divide
//		by 9600 to set this value to 24'd20834.  Likewise if you wished
//		to run this serial port at 115200 baud from a 200 MHz clock,
//		you would set the value to 24'd1736
//
//	Thus, to set the UART for the common setting of an 8-bit word, 
//	one stop bit, no parity, and 115200 baud over a 200 MHz clock, you
//	would want to set the setup value to:
//
//	32'h0006c8		// For 115,200 baud, 8 bit, no parity
//	32'h005161		// For 9600 baud, 8 bit, no parity
//	
//
//
// Creator:	Dan Gisselquist, Ph.D.
//		Gisselquist Technology, LLC
//
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2015-2016, Gisselquist Technology, LLC
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
////////////////////////////////////////////////////////////////////////////////
//
//
//
`define	TXU_BIT_ZERO	4'h0
`define	TXU_BIT_ONE	4'h1
`define	TXU_BIT_TWO	4'h2
`define	TXU_BIT_THREE	4'h3
`define	TXU_BIT_FOUR	4'h4
`define	TXU_BIT_FIVE	4'h5
`define	TXU_BIT_SIX	4'h6
`define	TXU_BIT_SEVEN	4'h7
`define	TXU_PARITY	4'h8	// Constant 1
`define	TXU_STOP	4'h9	// Constant 1
`define	TXU_SECOND_STOP	4'ha
// 4'hb	// Unused
// 4'hc	// Unused
// `define	TXU_START	4'hd	// An unused state
`define	TXU_BREAK	4'he
`define	TXU_IDLE	4'hf
//
//
module txuart(i_clk, i_reset, i_setup, i_break, i_wr, i_data, o_uart, o_busy);
	input			i_clk, i_reset;
	input		[29:0]	i_setup;
	input			i_break;
	input			i_wr;
	input		[7:0]	i_data;
	output	reg		o_uart;
	output	wire		o_busy;

	wire	[27:0]	clocks_per_baud, break_condition;
	wire	[1:0]	data_bits;
	wire		use_parity, parity_even, dblstop, fixd_parity;
	reg	[29:0]	r_setup;
	assign	clocks_per_baud = { 4'h0, r_setup[23:0] };
	assign	break_condition = { r_setup[23:0], 4'h0 };
	assign	data_bits   = r_setup[29:28];
	assign	dblstop     = r_setup[27];
	assign	use_parity  = r_setup[26];
	assign	fixd_parity = r_setup[25];
	assign	parity_even = r_setup[24];

	reg	[27:0]	baud_counter;
	reg	[3:0]	state;
	reg	[7:0]	lcl_data;
	reg		calc_parity, r_busy, zero_baud_counter;

	initial	o_uart = 1'b1;
	initial	r_busy = 1'b1;
	initial	state  = `TXU_IDLE;
	initial	lcl_data= 8'h0;
	initial	calc_parity = 1'b0;
	// initial	baud_counter = clocks_per_baud;//ILLEGAL--not constant
	always @(posedge i_clk)
	begin
		if (i_reset)
		begin
			o_uart <= 1'b1;
			r_busy <= 1'b1;
			state <= `TXU_IDLE;
			lcl_data <= 8'h0;
			calc_parity <= 1'b0;
		end else if (i_break)
		begin
			o_uart <= 1'b0;
			state <= `TXU_BREAK;
			calc_parity <= 1'b0;
			r_busy <= 1'b1;
		end else if (~zero_baud_counter)
		begin // r_busy needs to be set coming into here
			r_busy <= 1'b1;
		end else if (state == `TXU_BREAK)
		begin
			state <= `TXU_IDLE;
			r_busy <= 1'b1;
			o_uart <= 1'b1;
			calc_parity <= 1'b0;
		end else if (state == `TXU_IDLE)	// STATE_IDLE
		begin
			// baud_counter <= 0;
			r_setup <= i_setup;
			calc_parity <= 1'b0;
			lcl_data <= i_data;
			if ((i_wr)&&(~r_busy))
			begin	// Immediately start us off with a start bit
				o_uart <= 1'b0;
				r_busy <= 1'b1;
				case(data_bits)
				2'b00: state <= `TXU_BIT_ZERO;
				2'b01: state <= `TXU_BIT_ONE;
				2'b10: state <= `TXU_BIT_TWO;
				2'b11: state <= `TXU_BIT_THREE;
				endcase
				// baud_counter <= clocks_per_baud-28'h01;
			end else begin // Stay in idle
				o_uart <= 1'b1;
				r_busy <= 0;
				// state <= state;
			end
		end else begin
			// One clock tick in each of these states ...
			// baud_counter <= clocks_per_baud - 28'h01;
			r_busy <= 1'b1;
			if (state[3] == 0) // First 8 bits
			begin
				o_uart <= lcl_data[0];
				calc_parity <= calc_parity ^ lcl_data[0];
				if (state == `TXU_BIT_SEVEN)
					state <= (use_parity)?`TXU_PARITY:`TXU_STOP;
				else
					state <= state + 1;
				lcl_data <= { 1'b0, lcl_data[7:1] };
			end else if (state == `TXU_PARITY)
			begin
				state <= `TXU_STOP;
				if (fixd_parity)
					o_uart <= parity_even;
				else
					o_uart <= calc_parity^((parity_even)? 1'b1:1'b0);
			end else if (state == `TXU_STOP)
			begin // two stop bit(s)
				o_uart <= 1'b1;
				if (dblstop)
					state <= `TXU_SECOND_STOP;
				else
					state <= `TXU_IDLE;
				calc_parity <= 1'b0;
			end else // `TXU_SECOND_STOP and default:
			begin
				state <= `TXU_IDLE; // Go back to idle
				o_uart <= 1'b1;
				// Still r_busy, since we need to wait
				// for the baud clock to finish counting
				// out this last bit.
			end
		end 
	end

	assign	o_busy = (r_busy);


	initial	zero_baud_counter = 1'b1;
	initial baud_counter = 28'd200000; // 1ms @ 200MHz
	always @(posedge i_clk)
	begin
		zero_baud_counter <= (baud_counter == 28'h01);
		if ((i_reset)||(i_break))
			// Give ourselves 16 bauds before being ready
			baud_counter <= break_condition;
		else if (~zero_baud_counter)
			baud_counter <= baud_counter - 28'h01;
		else if (state == `TXU_BREAK)
			// Give us two stop bits before becoming available
			baud_counter <= clocks_per_baud<<2;
		else if (state == `TXU_IDLE)
		begin
			if((i_wr)&&(~r_busy))
				baud_counter <= clocks_per_baud - 28'h01;
			else
				zero_baud_counter <= 1'b1;
		end else
			baud_counter <= clocks_per_baud - 28'h01;
	end
endmodule

