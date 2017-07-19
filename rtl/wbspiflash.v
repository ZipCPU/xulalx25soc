////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	wbspiflash.v
//
// Project:	XuLA2-LX25 SoC based upon the ZipCPU
//
// Purpose:	Access a SPI flash via a WISHBONE interface.  This
//		includes both read and write (and erase) commands to the SPI
//		flash.  All read/write commands are accomplished using the
//		high speed (4-bit) interface.  Further, the device will be
//		left/kept in the 4-bit read interface mode between accesses,
//		for a minimum read latency.
//
//	Wishbone Registers (See spec sheet for more detail):
//	0: local config(r) / erase commands(w) / deep power down cmds / etc.
//	R: (Write in Progress), (dirty-block), (spi_port_busy), 1'b0, 9'h00,
//		{ last_erased_sector, 14'h00 } if (WIP)
//		else { current_sector_being_erased, 14'h00 }
//		current if write in progress, last if written
//	W: (1'b1 to erase), (12'h ignored), next_erased_block, 14'h ignored)
//	1: Configuration register
//	2: Status register (R/w)
//	3: Read ID (read only)
//	(19 bits): Data (R/w, but expect writes to take a while)
//		
//
// Creator:	Dan Gisselquist, Ph.D.
//		Gisselquist Technology, LLC
//
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2015-2017, Gisselquist Technology, LLC
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
// with this program.  (It's in the $(ROOT)/doc directory.  Run make with no
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
`define	WBSPI_RESET		6'd0
// `define	WBSPI_RESET_QUADMODE	1
`define	WBSPI_IDLE		6'd2
// `define	WBQSPI_RDIDLE		3	// Idle, but in fast read mode
// `define	WBSPI_WBDECODE		4
`define	WBSPI_WAIT_WIP_CLEAR	6'd5
`define	WBSPI_CHECK_WIP_CLEAR	6'd6
`define	WBSPI_CHECK_WIP_DONE	6'd7
`define	WBSPI_WEN		6'd8
`define	WBSPI_PP		6'd9	// Program page
// `define	WBSPI_QPP		10	// Program page, 4 bit mode
`define	WBSPI_WR_DATA		6'd11
`define	WBSPI_WR_BUS_CYCLE	6'd12
`define	WBSPI_RD_DUMMY		6'd13
// `define	WBSPI_QRD_ADDRESS	14
// `define	WBSPI_QRD_DUMMY	15
`define	WBSPI_READ_CMD		6'd16
`define	WBSPI_READ_DATA		6'd17
//`define	WBSPI_WAIT_TIL_RDIDLE	18
`define	WBSPI_READ_ID_CMD	6'd19
`define	WBSPI_READ_ID		6'd20
`define	WBSPI_READ_STATUS	6'd21
`define	WBSPI_READ_CONFIG	6'd22
`define	WBSPI_WRITE_STATUS	6'd23
`define	WBSPI_WRITE_CONFIG	6'd24
`define	WBSPI_ERASE_WEN		6'd25
`define	WBSPI_ERASE_CMD		6'd26
`define	WBSPI_ERASE_BLOCK	6'd27
`define	WBSPI_CLEAR_STATUS	6'd28
`define	WBSPI_IDLE_CHECK_WIP	6'd29
`define	WBSPI_WAIT_TIL_IDLE	6'd30
//
module	wbspiflash(i_clk_100mhz,
		// Internal wishbone connections
		i_wb_cyc, i_wb_data_stb, i_wb_ctrl_stb, i_wb_we,
		i_wb_addr, i_wb_data,
		// Wishbone return values
		o_wb_ack, o_wb_stall, o_wb_data,
		// Quad Spi connections to the external device
		o_spi_sck, o_spi_cs_n, i_spi_cs_n, o_spi_mosi, i_spi_miso,
		o_interrupt,
		i_bus_grant);
	parameter	AW=18, // Address width, -2 for 32-bit word access
			PW=6,	// Page address width (256 bytes,64 words)
			SW=10;	// Sector address width (4kB, 1kW)
	// parameter	AW, PW, Sw; // Address, page, and sector width(s)
	input			i_clk_100mhz;
	// Wishbone, inputs first
	input			i_wb_cyc, i_wb_data_stb, i_wb_ctrl_stb, i_wb_we;
	input	[(AW-1):0]	i_wb_addr;
	input		[31:0]	i_wb_data;
	// then outputs
	output	reg		o_wb_ack;
	output	reg		o_wb_stall;
	output	reg	[31:0]	o_wb_data;
	// Quad SPI control wires
	output	wire		o_spi_sck;
	output	wire		o_spi_cs_n;
	input			i_spi_cs_n;
	output	wire		o_spi_mosi;
	input			i_spi_miso;
	// Interrupt line
	output	reg		o_interrupt;
	// Do we own the bus?
	input			i_bus_grant;

	// output	wire	[31:0]	o_debug;

	reg		spi_wr, spi_hold;
	reg	[31:0]	spi_in;
	reg	[1:0]	spi_len;
	wire	[31:0]	spi_out;
	wire		spi_valid, spi_busy;
	wire		w_spi_sck, w_spi_cs_n;
	wire		w_spi_mosi;
	// wire	[22:0]	spi_dbg;
	lldspi	lldriver(i_clk_100mhz,
			spi_wr, spi_hold, spi_in, spi_len,
				spi_out, spi_valid, spi_busy,
			w_spi_sck, w_spi_cs_n, i_spi_cs_n, w_spi_mosi,
				i_spi_miso,
			i_bus_grant);

	// Erase status tracking
	reg		write_in_progress, write_protect;
	reg [(AW-1-SW):0] erased_sector;
	reg		dirty_sector;
	initial	begin
		write_in_progress = 1'b0;
		erased_sector = 0;
		dirty_sector  = 1'b1;
		write_protect = 1'b1;
	end

	reg	[7:0]	last_status;
	reg		spif_cmd;
	reg [(AW-1):0]	spif_addr;
	reg	[31:0]	spif_data;
	reg	[5:0]	state;
	reg		spif_ctrl, spif_req;
	wire	[(AW-1-SW):0]	spif_sector;
	assign	spif_sector = spif_addr[(AW-1):SW];

	// assign	o_debug = { spi_wr, spi_hold, state, spi_dbg };

	initial	state = `WBSPI_RESET;
	initial o_wb_ack   = 1'b0;
	initial o_wb_stall = 1'b1;
	initial spi_wr     = 1'b0;
	initial	spi_len    = 2'b00;
	initial o_interrupt= 1'b0;
	always @(posedge i_clk_100mhz)
	begin
	if (state == `WBSPI_RESET)
	begin
		// From a reset, we should
		//	Enable the Quad I/O mode
		//	Disable the Write protection bits in the status register
		//	Chip should already be up and running, so we can start
		//	immediately ....
		o_wb_ack <= 1'b0;
		o_wb_stall <= 1'b1;
		spi_wr   <= 1'b0;
		spi_hold <= 1'b0;
		last_status <= 8'h00;
		state <= `WBSPI_IDLE;
		spif_req <= 1'b0;
	end else if (state == `WBSPI_IDLE)
	begin
		o_interrupt <= 1'b0;
		o_wb_stall <= 1'b0;
		o_wb_ack <= 1'b0;
		spif_cmd   <= i_wb_we;
		spif_addr  <= i_wb_addr;
		spif_data  <= i_wb_data;
		spif_ctrl  <= (i_wb_ctrl_stb)&&(~i_wb_data_stb);
		spif_req   <= (i_wb_ctrl_stb)||(i_wb_data_stb);
		spi_wr   <= 1'b0; // Keep the port idle, unless told otherwise
		spi_hold <= 1'b0;
		// Data register access
		if ((i_wb_data_stb)&&(i_wb_cyc))
		begin

			if (i_wb_we) // Request to write a page
			begin
				if((~write_protect)&&(~write_in_progress))
				begin // 00
					spi_wr <= 1'b1;
					spi_len <= 2'b00; // 8 bits
					// Send a write enable command
					spi_in <= { 8'h06, 24'h00 };
					state <= `WBSPI_WEN;

					o_wb_ack <= 1'b0;
					o_wb_stall <= 1'b1;
				end else if (write_protect)
				begin // whether or not write-in_progress ...
					// Do nothing on a write protect
					// violation
					//
					o_wb_ack <= 1'b1;
					o_wb_stall <= 1'b0;
				end else begin // write is in progress, wait
					// for it to complete
					state <= `WBSPI_WAIT_WIP_CLEAR;
					o_wb_ack <= 1'b0;
					o_wb_stall <= 1'b1;
				end
			end else if (~write_in_progress)
			begin // Read access, normal mode(s)
				o_wb_ack   <= 1'b0;
				o_wb_stall <= 1'b1;
				spi_wr     <= 1'b1;	// Write cmd to device
				spi_in <= { 8'h0b,
					{(22-AW){1'b0}},
					i_wb_addr[(AW-1):0], 2'b00 };
				state <= `WBSPI_RD_DUMMY;
				spi_len    <= 2'b11; // cmd+addr,32bits
			end else begin
				// A write is in progress ... need to stall
				// the bus until the write is complete.
				state <= `WBSPI_WAIT_WIP_CLEAR;
				o_wb_ack   <= 1'b0;
				o_wb_stall <= 1'b1;
			end
		end else if ((i_wb_cyc)&&(i_wb_ctrl_stb)&&(i_wb_we))
		begin
			o_wb_stall <= 1'b1;
			case(i_wb_addr[1:0])
			2'b00: begin // Erase command register
				write_protect <= ~i_wb_data[28];
				o_wb_stall <= 1'b0;

				if((i_wb_data[31])&&(~write_in_progress))
				begin
					// Command an erase--ack it immediately

					o_wb_ack <= 1'b1;
					o_wb_stall <= 1'b0;

					if ((i_wb_data[31])&&(~write_protect))
					begin
						spi_wr <= 1'b1;
						spi_len <= 2'b00;
						// Send a write enable command
						spi_in <= { 8'h06, 24'h00 };
						state <= `WBSPI_ERASE_CMD;
						o_wb_stall <= 1'b1;
					end
				end else if (i_wb_data[31])
				begin
					state <= `WBSPI_WAIT_WIP_CLEAR;
					o_wb_ack   <= 1'b1;
					o_wb_stall <= 1'b1;
				end else
					o_wb_ack   <= 1'b1;
					o_wb_stall <= 1'b0;
				end
			2'b01: begin
				/*
				// Write the configuration register
				o_wb_ack <= 1'b1;
				o_wb_stall <= 1'b1;

				// Need to send a write enable command first
				spi_wr <= 1'b1;
				spi_len <= 2'b00; // 8 bits
				// Send a write enable command
				spi_in <= { 8'h06, 24'h00 };
				state <= `WBSPI_WRITE_CONFIG;
				*/
				o_wb_ack <= 1'b1; // Ack immediately
				o_wb_stall <= 1'b0; // No stall, but do nothing
				end
			2'b10: begin
				/*
				// Write the status register
				o_wb_ack <= 1'b1; // Ack immediately
				o_wb_stall <= 1'b1; // Stall other cmds
				// Need to send a write enable command first
				spi_wr <= 1'b1;
				spi_len <= 2'b00; // 8 bits
				// Send a write enable command
				spi_in <= { 8'h06, 24'h00 };
				state <= `WBSPI_WRITE_STATUS;
				*/
				o_wb_ack <= 1'b1; // Ack immediately
				o_wb_stall <= 1'b0; // No stall, but do nothing
				end
			2'b11: begin // Write the ID register??? makes no sense
				o_wb_ack <= 1'b1;
				o_wb_stall <= 1'b0;
				end
			endcase
		end else if ((i_wb_cyc)&&(i_wb_ctrl_stb)) // &&(~i_wb_we))
		begin
			case(i_wb_addr[1:0])
			2'b00: begin // Read local register
				if (write_in_progress) // Read status
				begin// register, is write still in progress?
					state <= `WBSPI_READ_STATUS;
					spi_wr <= 1'b1;
					spi_len <= 2'b01;// 8 bits out, 8 bits in
					spi_in <= { 8'h05, 24'h00};

					o_wb_ack <= 1'b0;
					o_wb_stall <= 1'b1;
				end else begin // Return w/o talking to device
					o_wb_ack <= 1'b1;
					o_wb_stall <= 1'b0;
					o_wb_data <= { write_in_progress,
						dirty_sector, spi_busy,
						~write_protect,
						{(28-AW){1'b0}}, 
						erased_sector, {(SW){1'b0}} };
				end end
			2'b01: begin // Read configuration register
				state <= `WBSPI_READ_CONFIG;
				spi_wr <= 1'b1;
				spi_len <= 2'b01;
				spi_in <= { 8'h35, 24'h00};

				o_wb_ack <= 1'b0;
				o_wb_stall <= 1'b1;
				end
			2'b10: begin // Read status register
				state <= `WBSPI_READ_STATUS;
				spi_wr <= 1'b1;
				spi_len <= 2'b01; // 8 bits out, 8 bits in
				spi_in <= { 8'h05, 24'h00};

				o_wb_ack <= 1'b0;
				o_wb_stall <= 1'b1;
				end
			2'b11: begin // Read ID register
				state <= `WBSPI_READ_ID_CMD;
				spi_wr <= 1'b1;
				spi_len <= 2'b00;
				spi_in <= { 8'h9f, 24'h00};

				o_wb_ack <= 1'b0;
				o_wb_stall <= 1'b1;
				end
			endcase
		end else if ((~i_wb_cyc)&&(write_in_progress))
		begin
			state <= `WBSPI_IDLE_CHECK_WIP;
			spi_wr <= 1'b1;
			spi_len <= 2'b01; // 8 bits out, 8 bits in
			spi_in <= { 8'h05, 24'h00};

			o_wb_ack <= 1'b0;
			o_wb_stall <= 1'b1;
		end
	end else if (state == `WBSPI_WAIT_WIP_CLEAR)
	begin
		o_wb_stall <= 1'b1;
		o_wb_ack   <= 1'b0;
		spi_wr <= 1'b0;
		spif_req<= (spif_req) && (i_wb_cyc);
		if (~spi_busy)
		begin
			spi_wr   <= 1'b1;
			spi_in   <= { 8'h05, 24'h0000 };
			spi_hold <= 1'b1;
			spi_len  <= 2'b01; // 16 bits write, so we can read 8
			state <= `WBSPI_CHECK_WIP_CLEAR;
		end
	end else if (state == `WBSPI_CHECK_WIP_CLEAR)
	begin
		o_wb_stall <= 1'b1;
		o_wb_ack   <= 1'b0;
		// Repeat as often as necessary until we are clear
		spi_wr <= 1'b1;
		spi_in <= 32'h0000; // Values here are actually irrelevant
		spi_hold <= 1'b1;
		spi_len <= 2'b00; // One byte at a time
		spif_req<= (spif_req) && (i_wb_cyc);
		if ((spi_valid)&&(~spi_out[0]))
		begin
			state <= `WBSPI_CHECK_WIP_DONE;
			spi_wr   <= 1'b0;
			spi_hold <= 1'b0;
			write_in_progress <= 1'b0;
			last_status <= spi_out[7:0];
		end
	end else if (state == `WBSPI_CHECK_WIP_DONE)
	begin
		o_wb_stall <= 1'b1;
		o_wb_ack   <= 1'b0;
		// Let's let the SPI port come back to a full idle,
		// and the chip select line go low before continuing
		spi_wr   <= 1'b0;
		spi_len  <= 2'b00;
		spi_hold <= 1'b0;
		spif_req<= (spif_req) && (i_wb_cyc);
		if ((o_spi_cs_n)&&(~spi_busy)) // Chip select line is high, we can continue
		begin
			spi_wr   <= 1'b0;
			spi_hold <= 1'b0;

			casez({ spif_cmd, spif_ctrl, spif_addr[1:0] })
			4'b00??: begin // Read data from ... somewhere
				spi_wr     <= 1'b1;	// Write cmd to device
				spi_in <= { 8'h0b, {(22-AW){1'b0}},
					spif_addr[(AW-1):0], 2'b00 };
				state <= `WBSPI_RD_DUMMY;
				spi_len    <= 2'b11; // Send cmd and addr
				end
			4'b10??: begin // Write data to ... anywhere
				spi_wr <= 1'b1;
				spi_len <= 2'b00; // 8 bits
				// Send a write enable command
				spi_in <= { 8'h06, 24'h00 };
				state <= `WBSPI_WEN;
				end
			4'b0110: begin // Read status register
				state <= `WBSPI_READ_STATUS;
				spi_wr <= 1'b1;
				spi_len <= 2'b01; // 8 bits out, 8 bits in
				spi_in <= { 8'h05, 24'h00};
				end
			4'b0111: begin
				state <= `WBSPI_READ_ID_CMD;
				spi_wr <= 1'b1;
				spi_len <= 2'b00;
				spi_in <= { 8'h9f, 24'h00};
				end
			default: begin //
				o_wb_stall <= 1'b1;
				o_wb_ack <= spif_req;
				state <= `WBSPI_WAIT_TIL_IDLE;
				end
			endcase
		// spif_cmd   <= i_wb_we;
		// spif_addr  <= i_wb_addr;
		// spif_data  <= i_wb_data;
		// spif_ctrl  <= (i_wb_ctrl_stb)&&(~i_wb_data_stb);
		// spi_wr <= 1'b0; // Keep the port idle, unless told otherwise
		end
	end else if (state == `WBSPI_WEN)
	begin // We came here after issuing a write enable command
		spi_wr <= 1'b0;
		o_wb_ack <= 1'b0;
		o_wb_stall <= 1'b1;
		spif_req<= (spif_req) && (i_wb_cyc);
		if ((~spi_busy)&&(o_spi_cs_n)&&(~spi_wr)) // Let's come to a full stop
			state <= `WBSPI_PP;
	end else if (state == `WBSPI_PP)
	begin // We come here under a full stop / full port idle mode
		// Issue our command immediately
		spi_wr <= 1'b1;
		spi_in <= { 8'h02, {(22-AW){1'b0}}, spif_addr, 2'b00 };
		spi_len <= 2'b11;
		spi_hold <= 1'b1;
		spif_req<= (spif_req) && (i_wb_cyc);

		// Once we get busy, move on
		if (spi_busy)
			state <= `WBSPI_WR_DATA;
		if (spif_sector == erased_sector)
			dirty_sector <= 1'b1;
	end else if (state == `WBSPI_WR_DATA)
	begin
		o_wb_stall <= 1'b1;
		o_wb_ack   <= 1'b0;
		spi_wr   <= 1'b1; // write without waiting
		spi_in   <= {
			spif_data[ 7: 0],
			spif_data[15: 8],
			spif_data[23:16],
			spif_data[31:24] };
		spi_len  <= 2'b11; // Write 4 bytes
		spi_hold <= 1'b1;
		if (~spi_busy)
		begin
			o_wb_ack <= spif_req; // Ack when command given
			state <= `WBSPI_WR_BUS_CYCLE;
		end
		spif_req<= (spif_req) && (i_wb_cyc);
	end else if (state == `WBSPI_WR_BUS_CYCLE)
	begin
		o_wb_ack <= 1'b0; // Turn off our ack and stall flags
		o_wb_stall <= 1'b1;
		spi_wr <= 1'b0;
		spi_hold <= 1'b1;
		write_in_progress <= 1'b1;
		spif_req<= (spif_req) && (i_wb_cyc);
		if (~i_wb_cyc)
		begin
			state <= `WBSPI_WAIT_TIL_IDLE;
			spi_hold <= 1'b0;
		end else if (spi_wr)
		begin // Give the SPI a chance to get busy on the last write
			// Do nothing here.
		end else if ((i_wb_data_stb)&&(i_wb_we)
				&&(i_wb_addr == (spif_addr+1))
				&&(i_wb_addr[(AW-1):PW]==spif_addr[(AW-1):PW]))
		begin
// CHECK ME!
			spif_cmd  <= 1'b1;
			spif_data <= i_wb_data;
			spif_addr <= i_wb_addr;
			spif_ctrl  <= 1'b0;
			spif_req<= 1'b1;
			// We'll keep the bus stalled on this request
			// for a while
			state <= `WBSPI_WR_DATA;
			o_wb_ack   <= 1'b0;
			o_wb_stall <= 1'b0;
		end else if ((i_wb_data_stb|i_wb_ctrl_stb)&&(~o_wb_ack)) // Writing out of bounds
		begin
			spi_hold <= 1'b0;
			spi_wr   <= 1'b0;
			state <= `WBSPI_WAIT_TIL_IDLE;
		end // Otherwise we stay here
	end else if (state == `WBSPI_RD_DUMMY)
	begin
		o_wb_ack   <= 1'b0;
		o_wb_stall <= 1'b1;

		spi_wr <= 1'b1; // Non-stop
		// Need to read one byte of dummy data,
		// just to consume 8 clocks
		spi_in <= { 8'h00, 24'h00 };
		spi_len <= 2'b00; // Read 8 bits
		spi_hold <= 1'b0;
		spif_req<= (spif_req) && (i_wb_cyc);
		
		if ((~spi_busy)&&(~o_spi_cs_n))
			// Our command was accepted
			state <= `WBSPI_READ_CMD;
	end else if (state == `WBSPI_READ_CMD)
	begin // Issue our first command to read 32 bits.
		o_wb_ack   <= 1'b0;
		o_wb_stall <= 1'b1;

		spi_wr <= 1'b1;
		spi_in <= { 8'hff, 24'h00 }; // Empty
		spi_len <= 2'b11; // Read 32 bits
		spi_hold <= 1'b0;
		spif_req<= (spif_req) && (i_wb_cyc);
		if ((spi_valid)&&(spi_len == 2'b11))
			state <= `WBSPI_READ_DATA;
	end else if (state == `WBSPI_READ_DATA)
	begin
		// Pipelined read support
		spi_wr <=((i_wb_cyc)&&(i_wb_data_stb)&&(~i_wb_we)&&(i_wb_addr== (spif_addr+1)));
		spi_in <= 32'h00;
		spi_len <= 2'b11;
		// Don't let the device go to idle until the bus cycle ends.
		//	This actually prevents a *really* nasty race condition,
		//	where the strobe comes in after the lower level device
		//	has decided to stop waiting.  The write is then issued,
		//	but no one is listening.  By leaving the device open,
		//	the device is kept in a state where a valid strobe
		//	here will be useful.  Of course, we don't accept
		//	all commands, just reads.  Further, the strobe needs
		//	to be high for two clocks cycles without changing
		//	anything on the bus--one for us to notice it and pull
		//	our head out of the sand, and a second for whoever
		//	owns the bus to realize their command went through.
		spi_hold <= 1'b1;
		spif_req<= (spif_req) && (i_wb_cyc);
		if ((spi_valid)&&(~spi_in[31]))
		begin // Single pulse acknowledge and write data out
			o_wb_ack <= spif_req;
			o_wb_stall <= (~spi_wr);
			// adjust endian-ness to match the PC
			o_wb_data <= { spi_out[7:0], spi_out[15:8],
				spi_out[23:16], spi_out[31:24] };
			state <= (spi_wr)?`WBSPI_READ_DATA
				: `WBSPI_WAIT_TIL_IDLE;
			spif_req <= spi_wr;
			spi_hold <= (~spi_wr);
			if (spi_wr)
				spif_addr <= i_wb_addr;
		end else if (~i_wb_cyc)
		begin // FAIL SAFE: If the bus cycle ends, forget why we're
			// here, just go back to idle
			state <= `WBSPI_WAIT_TIL_IDLE;
			spi_hold <= 1'b0;
			o_wb_ack <= 1'b0;
			o_wb_stall <= 1'b1;
		end else begin
			o_wb_ack <= 1'b0;
			o_wb_stall <= 1'b1;
		end
	end else if (state == `WBSPI_READ_ID_CMD)
	begin // We came into here immediately after issuing a 0x9f command
		// Now we need to read 32 bits of data.  Result should be
		// 0x0102154d (8'h manufacture ID, 16'h device ID, followed
		// by the number of extended bytes available 8'h4d).
		o_wb_ack <= 1'b0;
		o_wb_stall<= 1'b1;

		spi_wr <= 1'b1; // No data to send, but need four bytes, since
		spi_len <= 2'b11; // 32 bits of data are ... useful
		spi_in <= 32'h00; // Irrelevant
		spi_hold <= 1'b0;
		spif_req <= (spif_req) && (i_wb_cyc);
		if ((~spi_busy)&&(~o_spi_cs_n)&&(spi_len == 2'b11))
			// Our command was accepted, now go read the result
			state <= `WBSPI_READ_ID;
	end else if (state == `WBSPI_READ_ID)
	begin
		o_wb_ack <= 1'b0; // Assuming we're still waiting
		o_wb_stall <= 1'b1;

		spi_wr <= 1'b0; // No more writes, we've already written the cmd
		spi_hold <= 1'b0;
		spif_req <= (spif_req) && (i_wb_cyc);

		// Here, we just wait until the result comes back
		// The problem is, the result may be the previous result.
		// So we use spi_len as an indicator
		spi_len <= 2'b00;
		if((spi_valid)&&(spi_len==2'b00))
		begin // Put the results out as soon as possible
			o_wb_data <= spi_out[31:0];
			o_wb_ack <= spif_req;
			spif_req <= 1'b0;
		end else if ((~spi_busy)&&(o_spi_cs_n))
		begin
			state <= `WBSPI_IDLE;
			o_wb_stall <= 1'b0;
		end
	end else if (state == `WBSPI_READ_STATUS)
	begin // We enter after the command has been given, for now just
		// read and return
		spi_wr <= 1'b0;
		o_wb_ack <= 1'b0;
		spi_hold <= 1'b0;
		spif_req <= (spif_req) && (i_wb_cyc);
		if (spi_valid)
		begin
			o_wb_ack <= spif_req;
			o_wb_stall <= 1'b1;
			spif_req <= 1'b0;
			last_status <= spi_out[7:0];
			write_in_progress <= spi_out[0];
			if (spif_addr[1:0] == 2'b00) // Local read, checking
			begin // status, 'cause we're writing
				o_wb_data <= { spi_out[0],
					dirty_sector, spi_busy,
					~write_protect,
					{(28-AW){1'b0}},
					erased_sector, {(SW){1'b0}} };
			end else begin
				o_wb_data <= { 24'h00, spi_out[7:0] };
			end
		end

		if ((~spi_busy)&&(~spi_wr))
			state <= `WBSPI_IDLE;
	end else if (state == `WBSPI_READ_CONFIG)
	begin // We enter after the command has been given, for now just
		// read and return
		spi_wr <= 1'b0;
		o_wb_ack <= 1'b0;
		o_wb_stall <= 1'b1;
		spi_hold <= 1'b0;
		spif_req <= (spif_req) && (i_wb_cyc);

		if (spi_valid)
			o_wb_data <= { 24'h00, spi_out[7:0] };

		if ((~spi_busy)&&(~spi_wr))
		begin
			state <= `WBSPI_IDLE;
			o_wb_ack   <= spif_req;
			o_wb_stall <= 1'b0;
			spif_req <= 1'b0;
		end
	end else if (state == `WBSPI_WRITE_CONFIG)
	begin // We enter immediately after commanding a WEN
		o_wb_ack   <= 1'b0;
		o_wb_stall <= 1'b1;

		spi_len <= 2'b10;
		spi_in <= { 8'h01, last_status,
				spif_data[7:2], 1'b0,spif_data[0], 8'h00 };
		spi_wr <= 1'b0;
		spi_hold <= 1'b0;
		spif_req <= (spif_req) && (i_wb_cyc);
		if ((~spi_busy)&&(~spi_wr))
		begin
			spi_wr <= 1'b1;
			state <= `WBSPI_WAIT_TIL_IDLE;
			write_in_progress <= 1'b1;
		end
	end else if (state == `WBSPI_WRITE_STATUS)
	begin // We enter immediately after commanding a WEN
		o_wb_ack   <= 1'b0;
		o_wb_stall <= 1'b1;

		spi_len <= 2'b01;
		spi_in <= { 8'h01, spif_data[7:0], 16'h00 };
		// last_status <= i_wb_data[7:0]; // We'll read this in a moment
		spi_wr <= 1'b0;
		spi_hold <= 1'b0;
		spif_req <= (spif_req) && (i_wb_cyc);
		if ((~spi_busy)&&(~spi_wr))
		begin
			spi_wr <= 1'b1;
			last_status <= spif_data[7:0];
			write_in_progress <= 1'b1;
			if(((last_status[6])||(last_status[5]))
				&&((~spif_data[6])&&(~spif_data[5])))
				state <= `WBSPI_CLEAR_STATUS;
			else
				state <= `WBSPI_WAIT_TIL_IDLE;
		end
	end else if (state == `WBSPI_ERASE_CMD)
	begin // Know that WIP is clear on entry, WEN has just been commanded
		spi_wr     <= 1'b0;
		o_wb_ack   <= 1'b0;
		o_wb_stall <= 1'b1;
		spi_hold   <= 1'b0;
		spif_req <= (spif_req) && (i_wb_cyc);

		// Here's the erase command
		//spi_in <= { 8'hd8, 4'h0, spif_data[17:14], 14'h000, 2'b00 };
		spi_in <= { 8'h20, 4'h0, spif_data[(AW-1):SW],
				{(SW){1'b0}}, 2'b00 };
		spi_len <= 2'b11; // 32 bit write
		// together with setting our copy of the WIP bit
		write_in_progress <= 1'b1;
		// keeping track of which sector we just erased
		erased_sector <= spif_data[(AW-1):SW];
		// and marking this erase sector as no longer dirty
		dirty_sector <= 1'b0;

		// Wait for a full stop before issuing this command
		if ((~spi_busy)&&(~spi_wr)&&(o_spi_cs_n))
		begin // When our command is accepted, move to the next state
			spi_wr <= 1'b1;
			state <= `WBSPI_ERASE_BLOCK;
		end
	end else if (state == `WBSPI_ERASE_BLOCK)
	begin
		spi_wr     <= 1'b0;
		spi_hold   <= 1'b0;
		o_wb_stall <= 1'b1;
		o_wb_ack   <= 1'b0;
		spif_req <= (spif_req) && (i_wb_cyc);
		// When the port clears, we can head back to idle
		if ((~spi_busy)&&(~spi_wr))
		begin
			o_wb_ack <= spif_req;
			state <= `WBSPI_IDLE;
		end
	end else if (state == `WBSPI_CLEAR_STATUS)
	begin // Issue a clear status command
		spi_wr <= 1'b1;
		spi_hold <= 1'b0;
		spi_len <= 2'b00; // 8 bit command
		spi_in <= { 8'h30, 24'h00 };
		last_status[6:5] <= 2'b00;
		spif_req <= (spif_req) && (i_wb_cyc);
		if ((spi_wr)&&(~spi_busy))
			state <= `WBSPI_WAIT_TIL_IDLE;
	end else if (state == `WBSPI_IDLE_CHECK_WIP)
	begin // We are now in read status register mode

		// No bus commands have (yet) been given
		o_wb_stall <= 1'b1;
		o_wb_ack   <= 1'b0;
		spif_req <= (spif_req) && (i_wb_cyc);

		// Stay in this mode unless/until we get a command, or
		// 	the write is over
		spi_wr <= (((~i_wb_cyc)||((~i_wb_data_stb)&&(~i_wb_ctrl_stb)))
				&&(write_in_progress));
		spi_len <= 2'b00; // 8 bit reads
		if (spi_valid)
		begin
			write_in_progress <= spi_out[0];
			if ((~spi_out[0])&&(write_in_progress))
				o_interrupt <= 1'b1;
		end else
			o_interrupt <= 1'b0;

		if ((~spi_wr)&&(~spi_busy)&&(o_spi_cs_n))
		begin // We can now go to idle and process a command
			o_wb_stall <= 1'b0;
			o_wb_ack   <= 1'b0;
			state <= `WBSPI_IDLE;
		end
	end else // if (state == `WBSPI_WAIT_TIL_IDLE) or anything else
	begin
		spi_wr     <= 1'b0;
		spi_hold   <= 1'b0;
		o_wb_stall <= 1'b1;
		o_wb_ack   <= 1'b0;
		spif_req   <= 1'b0;
		if ((~spi_busy)&&(o_spi_cs_n)&&(~spi_wr)) // Wait for a full
		begin // clearing of the SPI port before moving on
			state <= `WBSPI_IDLE;
			o_wb_stall <= 1'b0; 
			o_wb_ack   <= 1'b0; // Shouldn't be acking anything here
		end
	end
	end

	// Command and control during the reset sequence
	assign	o_spi_cs_n = w_spi_cs_n;
	assign	o_spi_sck  = w_spi_sck;
	assign	o_spi_mosi = w_spi_mosi;
endmodule
