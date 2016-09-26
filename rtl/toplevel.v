`timescale 10ns / 100ps
////////////////////////////////////////////////////////////////////////////////
//
// Filename:	toplevel.v
//
// Project:	XuLA2-LX25 SoC based upon the ZipCPU
//
// Purpose:	This is the _top_level_ verilog file for the XuLA2-LX25
//		SoC project.  Everything else fits underneath here (logically).
//	This is also the only file that will not go through Verilator.  Specific
//	to this file are the Xilinx primitives necessary to build for the
//	XuLA2 board--with the only exception being the ICAPE_SPARTAN6 interface.
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
// `define	HELLO_WORLD
// `define	ECHO_TEST
//
module toplevel(i_clk_12mhz,
		i_ram_feedback_clk,
		o_sf_cs_n, o_sd_cs_n, o_spi_sck, o_spi_mosi, i_spi_miso,
		o_ram_clk, o_ram_cke, o_ram_cs_n, o_ram_ras_n, o_ram_cas_n,
		o_ram_we_n, o_ram_bs, o_ram_addr, o_ram_udqm, o_ram_ldqm,
		io_ram_data,
		i_gpio, o_gpio, o_pwm, i_rx_uart, o_tx_uart);
	input		i_clk_12mhz;
	input		i_ram_feedback_clk;
	//
	// SPI connection(s): Flash and SD Card
	output	wire	o_sf_cs_n;
	output	wire	o_sd_cs_n;
	output	wire	o_spi_sck;
	output	wire	o_spi_mosi;
	input		i_spi_miso;
	//
	// SD RAM
	output	wire	o_ram_clk, o_ram_cke;
	output	wire	o_ram_cs_n, o_ram_ras_n, o_ram_cas_n, o_ram_we_n;
	output	wire	[1:0]	o_ram_bs;
	output	wire	[12:0]	o_ram_addr;
	output	wire		o_ram_udqm, o_ram_ldqm;
	inout	wire	[15:0]	io_ram_data;
	//
	// General purpose I/O
	// output	[31:0]	io_chan;
	input		[13:0]	i_gpio;
	output	wire	[14:0]	o_gpio;
	output	wire		o_pwm;
	input			i_rx_uart;
	output	wire		o_tx_uart;

/////
	wire	[7:0]	rx_data, tx_data;
	wire		rx_stb, tx_stb, tx_busy;
	wire		clk_s, reset_s, intermediate_clk, intermediate_clk_n,
			ck_zero_0;
	wire	ck_zero_1;

	DCM_SP #(
		.CLKDV_DIVIDE(2.0),
		.CLKFX_DIVIDE(3),
		.CLKFX_MULTIPLY(20),
		.CLKIN_DIVIDE_BY_2("FALSE"),
		.CLKIN_PERIOD(83.0),	// 12MHz clock period in ns
		.CLKOUT_PHASE_SHIFT("NONE"),
		.CLK_FEEDBACK("1X"),
		.DESKEW_ADJUST("SYSTEM_SYNCHRONOUS"),
		.DLL_FREQUENCY_MODE("LOW"),
		.DUTY_CYCLE_CORRECTION("TRUE"),
		.PHASE_SHIFT(0),
		.STARTUP_WAIT("TRUE")
	) u0(	.CLKIN(i_clk_12mhz),
		.CLK0(ck_zero_0),
		.CLKFB(ck_zero_0),
		.CLKFX(clk_s),
		// .CLKFX180(intermediate_clk_n),
		.PSEN(1'b0),
		.RST(1'b0));

	DCM_SP #(
		.CLKDV_DIVIDE(2),
		.CLKFX_MULTIPLY(2),
		.CLKFX_DIVIDE(2),
		.CLKOUT_PHASE_SHIFT("FIXED"),
		.CLK_FEEDBACK("1X"),
		.DESKEW_ADJUST("SYSTEM_SYNCHRONOUS"),
		.DLL_FREQUENCY_MODE("LOW"),
		.DUTY_CYCLE_CORRECTION("TRUE"),
		// At a clock of 80MHz ...
		//
		//	This clock needs to be delayed so that what takes
		//	place within the SDRAM takes place at the middle
		//	of the clock interval.  This is separate from all the
		//	rest of the logic in the FPGA where what takes place
		//	happens at the clock transition.
		//
		.PHASE_SHIFT(-45),
		.STARTUP_WAIT("TRUE")
	) u1(	.CLKIN(clk_s),
		.CLK0(ck_zero_1),
		.CLKFB(ck_zero_1),
		.CLK180(intermediate_clk_n),
		.PSEN(1'b0),
		.RST(1'b0));
	assign	intermediate_clk = ck_zero_1;

	ODDR2 u2( .Q(o_ram_clk),
		.C0(intermediate_clk),
		.C1(intermediate_clk_n),
		.CE(1'b1), .D0(1'b1), .D1(1'b0), .R(1'b0), .S(1'b0));

	// Generate active-high reset.
	/*
	reg	r_reset;
	initial	r_reset = 1'b1;
	always @(posedge i_clk_12mhz)
		r_reset <= 1'b0;
	*/
	assign	reset_s = 1'b0;

	jtagser	jtagtxrx(clk_s, rx_stb, rx_data, tx_stb, tx_data, tx_busy);


	wire	[15:0]	ram_data;
	wire		ram_drive_data;
	reg	[15:0]	r_ram_data;

	busmaster #(24,15,14)
		wbbus(clk_s, reset_s,
			// External JTAG bus control
			rx_stb, rx_data, tx_stb, tx_data, tx_busy,
			// Board lights and switches ... none
			// SPI/SD-card flash
			o_sf_cs_n, o_sd_cs_n, o_spi_sck, o_spi_mosi, i_spi_miso,
			// SDRAM interface
			// o_ram_clk,	// SDRAM clock = clk_100mhz_s = clk_s
			o_ram_cs_n,	// Chip select
			o_ram_cke,	// Clock enable
			o_ram_ras_n,	// Row address strobe
			o_ram_cas_n,	// Column address strobe
			o_ram_we_n,	// Write enable
			o_ram_bs,	// Bank select
			o_ram_addr,	// Address lines
			ram_drive_data,
			r_ram_data,	// Data lines (input)
			ram_data,	// Data lines (output)
			{ o_ram_udqm, o_ram_ldqm },
			// GPIO
			i_gpio, o_gpio, o_pwm, i_rx_uart, o_tx_uart
		);

	assign io_ram_data = (ram_drive_data) ? ram_data : 16'bzzzz_zzzz_zzzz_zzzz;

	reg	[15:0]	r_ram_data_ext_clk;
	// always @(posedge intermediate_clk_n)
	always @(posedge clk_s)
		r_ram_data_ext_clk <= io_ram_data;
	always @(posedge clk_s)
		r_ram_data <= r_ram_data_ext_clk;

endmodule
