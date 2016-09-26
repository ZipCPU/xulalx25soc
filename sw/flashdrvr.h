////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	flashdrvr.h
//
// Project:	XuLA2-LX25 System on a Chip
//
// Purpose:	Flash driver.  Encapsulate writing to the flash device.
//
// Creator:	Dan Gisselquist
//		Gisselquist Tecnology, LLC
//
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2016, Gisselquist Technology, LLC
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
//
#ifndef	FLASHDRVR_H
#define	FLASHDRVR_H

#include "regdefs.h"

class	FLASHDRVR {
private:
	DEVBUS	*m_fpga;

	void	flwait(void);
public:
	FLASHDRVR(DEVBUS *fpga) : m_fpga(fpga) {}
	bool	erase_sector(const unsigned sector, const bool verify_erase=true);
	bool	write_page(const unsigned addr, const unsigned len,
			const unsigned *data, const bool verify_write=true);
	bool	write(const unsigned addr, const unsigned len,
			const unsigned *data, const bool verify=false);
};

#endif
