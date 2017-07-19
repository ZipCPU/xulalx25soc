////////////////////////////////////////////////////////////////////////////////
//
// Filename:	ICAP_SPARTAN6.v
//
// Project:	XuLA2-LX25 SoC based upon the ZipCPU
//
// Purpose:	Verilator cannot build the ICAP_SPARTAN6 primitive for the
//		XuLA2 board.  This file is provided for Verilator to build
//	instead.  It is *not* a fully functional ICAP_SPARTAN6 primitive by
//	any stretch of the imagination, but it makes the build process work.
//	Reads and writes should "succeed", the values read or written however
//	will be meaningless.
//
//
// Creator:	Dan Gisselquist, Ph.D.
//		Gisselquist Technology, LLC
//
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2015,2017, Gisselquist Technology, LLC
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
module	ICAP_SPARTAN6(CLK, CE, WRITE, I, O, BUSY);
	input			CLK, CE, WRITE;
	input		[15:0]	I;
	output	wire	[15:0]	O;
	output	wire	BUSY;

	reg	[15:0]	rv;
	initial	rv = 16'h0000;
	always @(posedge CLK)
		if ((CE)&&(WRITE))
			rv <= I;
	assign	O = rv;
	assign	BUSY = 1'b0;
endmodule
