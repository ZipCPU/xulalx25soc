////////////////////////////////////////////////////////////////////////////////
//
// Filename:	wbgpio.v
//
// Project:	XuLA2 board
//
// Purpose:	This extremely simple GPIO controller, although minimally 
//		featured, is designed to control up to sixteen general purpose
//	input and sixteen general purpose output lines of a module from a
//	single address on a 32-bit wishbone bus.
//
//	Input GPIO values are contained in the top 16-bits.  Any change in
//	input values will generate an interrupt.
//
//	Output GPIO values are contained in the bottom 16-bits.  To change an
//	output GPIO value, writes to this port must also set a bit in the
//	upper sixteen bits.  Hence, to set GPIO output zero, one would write
//	a 0x010001 to the port, whereas a 0x010000 would clear the bit.  This
//	interface makes it possible to change only the bit of interest, without
//	needing to capture and maintain the prior bit values--something that
//	might be difficult from a interrupt context within a CPU.
//	
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
module wbgpio(i_clk, i_wb_cyc, i_wb_stb, i_wb_we, i_wb_data, o_wb_data,
		i_gpio, o_gpio, o_int);
	parameter	NIN=16, NOUT=16, DEFAULT=16'h00;
	input			i_clk;
	//
	input			i_wb_cyc, i_wb_stb, i_wb_we;
	input		[31:0]	i_wb_data;
	output	wire	[31:0]	o_wb_data;
	//
	input		[(NIN-1):0]	i_gpio;
	output	reg	[(NOUT-1):0]	o_gpio;
	//
	output	reg		o_int;

	// 9LUT's, 16 FF's
	always @(posedge i_clk)
		if ((i_wb_stb)&&(i_wb_we))
			o_gpio <= ((o_gpio)&(~i_wb_data[(NOUT+16-1):16]))
				|((i_wb_data[(NOUT-1):0])&(i_wb_data[(NOUT+16-1):16]));

	reg	[(NIN-1):0]	x_gpio, r_gpio;
	// 3 LUTs, 33 FF's
	always @(posedge i_clk)
	begin
		x_gpio <= i_gpio;
		r_gpio <= x_gpio;
		o_int  <= (x_gpio != r_gpio);
	end

	assign	o_wb_data = { {(16-NIN){1'b0}}, r_gpio,
					{(16-NOUT){1'b0}}, o_gpio };
endmodule
