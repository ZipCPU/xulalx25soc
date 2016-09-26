///////////////////////////////////////////////////////////////////////////
//
// Filename:	ioslave
//
// Project:	XuLA2 board
//
// Purpose:	This handles a bunch of small, simple I/O registers.  To be
//		included here, the I/O register must take exactly a single
//	clock to access and never stall.
//
//	Particular peripherals include:
//		- the interrupt controller
//		- Realtime Clock
//		- Realtime clock Date
//		- A bus error register--records the address of the last
//			bus error.  Cannot be written to, save by a bus error.
//	Other peripherals have been removed due to a lack of bus address space.
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
//
`include "builddate.v"
module	ioslave(i_clk,
		// Wishbone control
		i_wb_cyc, i_wb_stb, i_wb_we, i_wb_addr, i_wb_data,
			o_wb_ack, o_wb_stall, o_wb_data,
		// GPIO wires
		i_gpio,
		o_gpio,
		// Other registers
		i_bus_err_addr,
		brd_interrupts, o_ints_to_zip_cpu, o_interrupt);
	parameter	NGPO=15, NGPI=15;
	input			i_clk;
	// Wishbone control
	//	inputs...
	input			i_wb_cyc, i_wb_stb, i_wb_we;
	input		[4:0]	i_wb_addr;
	input		[31:0]	i_wb_data;
	//	outputs...
	output	reg		o_wb_ack;
	output	wire		o_wb_stall;
	output	wire	[31:0]	o_wb_data;
	// GPIO
	input	[(NGPI-1):0]	i_gpio;
	output wire [(NGPO-1):0] o_gpio;
	// Other registers
	input		[31:0]	i_bus_err_addr;
	input		[6:0]	brd_interrupts;
	output	wire	[8:0]	o_ints_to_zip_cpu;
	output	wire		o_interrupt;


	wire	i_sdcard_int, i_uart_tx_int, i_uart_rx_int, i_pwm_int,
		i_scop_int, i_flash_int;
	assign	i_sdcard_int  = brd_interrupts[6];
	assign	i_uart_tx_int = brd_interrupts[5];
	assign	i_uart_rx_int = brd_interrupts[4];
	assign	i_pwm_int     = brd_interrupts[3];
	assign	i_scop_int    = brd_interrupts[2];
	assign	i_flash_int   = brd_interrupts[1];

	// reg		[31:0]	pwrcount;
	// reg		[31:0]	rtccount;
	wire		[31:0]	ictrl_data, gpio_data, date_data, timer_data;

	reg	[31:0]	r_wb_data;
	reg		r_wb_addr;
	always @(posedge i_clk)
	begin
		r_wb_addr <= i_wb_addr[4];
		// if ((i_wb_cyc)&&(i_wb_stb)&&(i_wb_we)&&(~i_wb_addr[4]))
		// begin
			// casez(i_wb_addr[3:0])
			// // 4'h0: begin end // Reset register
			// // 4'h1: begin end // Status/Control register
			// // 4'h2: begin end // Reset register
			// // 4'h3: begin end // Interrupt Control register
			// // 4'h4: // R/O Power count
			// // 4'h5: // RTC count
			// default: begin end
			// endcase
		// end else
		if ((i_wb_stb)&&(~i_wb_we))
		begin
			casez(i_wb_addr[3:0])
			4'h01: r_wb_data <= `DATESTAMP;
			4'h02: r_wb_data <= ictrl_data;
			4'h03: r_wb_data <= i_bus_err_addr;
			4'h04: r_wb_data <= timer_data;
			4'h05: r_wb_data <= date_data;
			4'h06: r_wb_data <= gpio_data;
			default: r_wb_data <= 32'h0000;
			endcase
		end
	end

	// The Zip Timer
	wire		tm_int, tm_ack, tm_stall;
	ziptimer	timer(i_clk, 1'b0, 1'b1,
				(i_wb_cyc),(i_wb_stb)&&(i_wb_addr==5'h04),
					i_wb_we, i_wb_data,
				tm_ack, tm_stall, timer_data, tm_int);

	// The interrupt controller
	wire		ck_int;
	wire	[8:0]	interrupt_vector;
	assign	interrupt_vector = { tm_int,
			i_uart_tx_int, i_uart_rx_int, i_pwm_int, gpio_int,
			i_scop_int, i_flash_int, ck_int, brd_interrupts[0] };
	icontrol #(9)	intcontroller(i_clk, 1'b0,
				((i_wb_stb)&&(i_wb_we)
					&&(i_wb_addr==5'h2)), i_wb_data, 
				ictrl_data, interrupt_vector,
				o_interrupt);

	/*
	// The ticks since power up register
	initial	pwrcount = 32'h00;
	always @(posedge i_clk)
		if (~ (&pwrcount))
			pwrcount <= pwrcount+1;

	// The time since power up register
	reg	[15:0]	subrtc;
	reg		subpps;
	initial	rtccount = 32'h00;
	initial	subrtc = 16'h00;
	always @(posedge i_clk)
		{ subpps, subrtc } <= subrtc + 16'd43;
	always @(posedge i_clk)
		rtccount <= rtccount + ((subpps)? 32'h1 : 32'h0);
	*/

	//
	// GPIO controller
	//
	wire	gpio_int;
	wbgpio	#(NGPI, NGPO)
		gpiodev(i_clk, i_wb_cyc, (i_wb_stb)&&(i_wb_addr[4:0]==5'h6),
			i_wb_we, i_wb_data, gpio_data, i_gpio, o_gpio,gpio_int);

	//
	// 4'b1xxx
	// BUS access to a real time clock (not calendar, just clock)
	//
	//
	wire	[31:0]	ck_data;
	wire		ck_ppd;
	rtclight
		// #(32'h3ba6fe)	//  72 MHz clock	(2^48 / 72e6)
		// #(32'h388342)	//  76 MHz clock	(2^48 / 76e6)
		#(32'h35afe5)	//  80 MHz clock
		// #(32'h2eaf36)	//  92 MHz clock
		// #(32'h2af31d)	// 100 MHz clock
		theclock(i_clk, i_wb_cyc, (i_wb_stb)&&(i_wb_addr[4]),
				i_wb_we, i_wb_addr[2:0], i_wb_data,
			ck_data, ck_int, ck_ppd);

	wire		date_ack, date_stall;
	rtcdate	thedate(i_clk, ck_ppd,
			i_wb_cyc, (i_wb_stb)&&(i_wb_addr[3:0]==4'h5),
				i_wb_we, i_wb_data,
			date_ack, date_stall, date_data);

	always @(posedge i_clk)
		o_wb_ack <= (i_wb_stb)&&(i_wb_cyc);
	assign	o_wb_stall = 1'b0;

	assign	o_wb_data = (r_wb_addr)? ck_data : r_wb_data;

	//
	//
	assign	o_ints_to_zip_cpu = { i_sdcard_int,
			i_uart_tx_int, i_uart_rx_int,
			i_pwm_int, gpio_int, i_scop_int, i_flash_int,
			ck_int, o_interrupt };
endmodule
