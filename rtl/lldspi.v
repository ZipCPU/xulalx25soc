///////////////////////////////////////////////////////////////////////////
//
// Filename: 	lldspi.v
//
// Project:	XuLA2 board
//
// Purpose:	Reads/writes a word (user selectable number of bytes) of data
//		to/from a Quad SPI port.  The port is understood to be 
//	a normal SPI port unless the driver requests two bit mode.  (Not yet
//	supported.)  When not in use, no bits will toggle.
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
// You should have received a copy of the GNU General Public License along
// with this program.  (It's in the $(ROOT)/doc directory, run make with no
// target there if the PDF file isn't present.)  If not, see
// <http://www.gnu.org/licenses/> for a copy.
//
// License:	GPL, v3, as defined and found on www.gnu.org,
//		http://www.gnu.org/licenses/gpl.html
//
//
///////////////////////////////////////////////////////////////////////////
`define	SPI_IDLE	3'h0
`define	SPI_START	3'h1
`define	SPI_BITS	3'h2
`define	SPI_READY	3'h3
`define	SPI_HOLDING	3'h4
`define	SPI_STOP	3'h5
`define	SPI_STOP_B	3'h6
`define	SPI_WAIT	3'h7

// Modes
// `define	SPI_MOD_SPI	2'b00
// `define	QSPI_MOD_QOUT	2'b10
// `define	QSPI_MOD_QIN	2'b11

module	lldspi(i_clk,
		// Module interface
		i_wr, i_hold, i_word, i_len,
			o_word, o_valid, o_busy,
		// QSPI interface
		o_sck, o_cs_n, i_cs_n, o_mosi, i_miso,
		// Bus grant information
		i_bus_grant);
	input			i_clk;
	// Chip interface
	//	Can send info
	//		i_hold = 0, i_wr = 1,
	//			i_word = { 1'b0, 32'info to send },
	//			i_len = # of bytes in word-1
	input			i_wr, i_hold;
	input		[31:0]	i_word;
	input		[1:0]	i_len;	// 0=>8bits, 1=>16 bits, 2=>24 bits, 3=>32 bits
	output	reg	[31:0]	o_word;
	output	reg		o_valid, o_busy;
	// Interface with the QSPI lines
	output	reg		o_sck;
	output	reg		o_cs_n;
	input			i_cs_n; // Feedback from the arbiter
	output	reg		o_mosi;
	input			i_miso;
	// Bus grant
	input			i_bus_grant;

	reg	[5:0]	spi_len;
	reg	[31:0]	r_word;
	reg	[30:0]	r_input;
	reg	[2:0]	state;
	initial	state = `SPI_IDLE;
	initial	o_sck   = 1'b1;
	initial	o_cs_n  = 1'b1;
	initial	o_mosi  = 1'b0;
	initial	o_valid = 1'b0;
	initial	o_busy  = 1'b0;
	initial	r_input = 31'h000;
	always @(posedge i_clk)
		if ((state == `SPI_IDLE)&&(o_sck))
		begin
			o_cs_n <= 1'b1;
			o_valid <= 1'b0;
			o_busy  <= 1'b0;
			if (i_wr)
			begin
				r_word <= i_word;
				state <= `SPI_WAIT;
				spi_len<= { 1'b0, i_len, 3'b000 } + 6'h8;
				o_cs_n <= 1'b0;
				o_busy <= 1'b1;
				o_sck <= 1'b1;
			end
		end else if (state == `SPI_WAIT)
		begin
			if (i_bus_grant)
				state <= `SPI_START;
		end else if (state == `SPI_START)
		begin // We come in here with sck high, stay here 'til sck is low
			if (~i_cs_n) // Wait 'til the bus has been granted
				o_sck <= 1'b0;
			if (o_sck == 1'b0)
			begin
				state <= `SPI_BITS;
				spi_len<= spi_len - 6'h1;
				r_word <= { r_word[30:0], 1'b0 };
			end
			o_cs_n <= 1'b0;
			o_busy <= 1'b1;
			o_valid <= 1'b0;
			o_mosi  <= r_word[31];
		end else if (~o_sck)
		begin
			o_sck <= 1'b1;
			o_busy <= ((state != `SPI_READY)||(~i_wr));
			o_valid <= 1'b0;
		end else if (state == `SPI_BITS)
		begin
			// Should enter into here with at least a spi_len
			// of one, perhaps more
			o_sck <= 1'b0;
			o_busy <= 1'b1;
			o_mosi <= r_word[31];
			r_word <= { r_word[30:0], 1'b0 };
			spi_len <= spi_len - 6'h1;
			if (spi_len == 6'h1)
				state <= `SPI_READY;

			o_valid <= 1'b0;
			r_input <= { r_input[29:0], i_miso };
		end else if (state == `SPI_READY)
		begin
			o_valid <= 1'b0;
			o_cs_n <= 1'b0;
			o_busy <= 1'b1;
			// This is the state on the last clock (both low and
			// high clocks) of the data.  Data is valid during
			// this state.  Here we chose to either STOP or
			// continue and transmit more.
			o_sck <= (i_hold); // No clocks while holding
			if((~o_busy)&&(i_wr))// Acknowledge a new request
			begin
				state <= `SPI_BITS;
				o_busy <= 1'b1;
				o_sck <= 1'b0;

				// Set up the first bits on the bus
				o_mosi <= i_word[31];
				r_word <= { i_word[30:0], 1'b0 };
				spi_len<= { 1'b0, i_len, 3'b000 } + 6'h8-6'h1;

				// Read a bit upon any transition
				o_valid <= 1'b1;
				r_input <= { r_input[29:0], i_miso };
				o_word  <= { r_input[30:0], i_miso };
			end else begin
				o_sck <= 1'b1;
				state <= (i_hold)?`SPI_HOLDING : `SPI_STOP;
				o_busy <= (~i_hold);

				// Read a bit upon any transition
				o_valid <= 1'b1;
				r_input <= { r_input[29:0], i_miso };
				o_word  <= { r_input[30:0], i_miso };
			end
		end else if (state == `SPI_HOLDING)
		begin
			// We need this state so that the o_valid signal
			// can get strobed with our last result.  Otherwise
			// we could just sit in READY waiting for a new command.
			//
			// Incidentally, the change producing this state was
			// the result of a nasty race condition.  See the
			// commends in wbqspiflash for more details.
			//
			o_valid <= 1'b0;
			o_cs_n <= 1'b0;
			o_busy <= 1'b0;
			if((~o_busy)&&(i_wr))// Acknowledge a new request
			begin
				state  <= `SPI_BITS;
				o_busy <= 1'b1;
				o_sck  <= 1'b0;

				// Set up the first bits on the bus
				o_mosi <= i_word[31];
				r_word <= { i_word[30:0], 1'b0 };
				spi_len<= { 1'b0, i_len, 3'b111 };
			end else begin
				o_sck <= 1'b1;
				state <= (i_hold)?`SPI_HOLDING : `SPI_STOP;
				o_busy <= (~i_hold);
			end
		end else if (state == `SPI_STOP)
		begin
			o_sck   <= 1'b1; // Stop the clock
			o_valid <= 1'b0; // Output may have just been valid, but no more
			o_busy  <= 1'b1; // Still busy till port is clear
			state <= `SPI_STOP_B;
		end else if (state == `SPI_STOP_B)
		begin
			o_cs_n <= 1'b1;
			o_sck <= 1'b1;
			// Do I need this????
			// spi_len <= 3; // Minimum CS high time before next cmd
			state <= `SPI_IDLE;
			o_valid <= 1'b0;
			o_busy <= 1'b1;
		end else begin // Invalid states, should never get here
			state   <= `SPI_STOP;
			o_valid <= 1'b0;
			o_busy  <= 1'b1;
			o_cs_n  <= 1'b1;
			o_sck   <= 1'b1;
		end

endmodule

