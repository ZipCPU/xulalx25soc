///////////////////////////////////////////////////////////////////////////
//
// Filename:	spiarbiter.v
//
// Project:	XuLA2 board
//
// Purpose:	The XuLA2 offers SPI access to both a FLASH and an SD Card.
//		This simple script arbitrates between the two of those to
//	determine who has access.  Drivers for both chips may interact with
//	this arbiter as though it were a SPI device with one additional piece
//	of functionality: the clock line may not be brought low until access
//	has been granted to the chip.  Thus, the controller wishing to access
//	its device should pull the CS line low, and then wait to read that
//	its grant line is high.  Once CS is low and grant is high, it may
//	then bring CK low and start its transaction.
//
//	When two or more controllers request access at the same time,
//	access will be given in priority order.  Further, access is always
//	granted to device 'A' without request as long as device 'B' isn't
//	busy.
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
module	spiarbiter(i_clk,
		i_cs_a_n, i_ck_a, i_mosi_a,
		i_cs_b_n, i_ck_b, i_mosi_b,
		o_cs_a_n, o_cs_b_n, o_ck, o_mosi,
		o_grant);
	input		i_clk;
	input		i_cs_a_n, i_ck_a, i_mosi_a;
	// output	wire	o_grant_a;
	input		i_cs_b_n, i_ck_b, i_mosi_b;
	// output	wire	o_grant_b;
	output	wire	o_cs_a_n, o_cs_b_n, o_ck, o_mosi;
	//
	output	wire	o_grant; // == o_grant_a = ~o_grant_b

	reg	a_owner;
	initial	a_owner = 1'b1;
	always @(posedge i_clk)
		if ((i_cs_a_n)&&(i_cs_b_n))
			a_owner <= 1'b1; // Keep control
		else if ((i_cs_a_n)&&(~i_cs_b_n))
			a_owner <= 1'b0; // Give up control
		else if ((~i_cs_a_n)&&(i_cs_b_n))
			a_owner <= 1'b1; // Take control

	// assign	o_grant_a = a_owner;
	// assign	o_grant_b = (~a_owner);

	assign	o_cs_a_n = (~a_owner)||(i_cs_a_n);
	assign	o_cs_b_n = ( a_owner)||(i_cs_b_n);
	assign	o_ck     = ( a_owner)?i_ck_a   : i_ck_b;
	assign	o_mosi   = ( a_owner)?i_mosi_a : i_mosi_b;

	assign	o_grant = ~a_owner;

endmodule

