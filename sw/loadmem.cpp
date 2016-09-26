////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	loadmem.cpp
//
// Project:	XuLA2 board
//
// Purpose:	Load a local file into the SDRAM's memory
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
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

#include "llcomms.h"
#include "usbi.h"
#include "port.h"
#include "regdefs.h"

FPGA	*m_fpga;

int main(int argc, char **argv) {
	FILE		*fpin;
	unsigned	pos=0;
	int		port = FPGAPORT, skp;
	const int	BUFLN = 127;
	FPGA::BUSW	*buf = new FPGA::BUSW[BUFLN];
	bool		use_usb = true;

	skp = 1;
	for(int argn=0; argn<argc-skp; argn++) {
		if (argv[argn+skp][0] == '-') {
			if (argv[argn+skp][1] == 'u')
				use_usb = true;
			else if (argv[argn+skp][1] == 'p') {
				use_usb = false;
				if (isdigit(argv[argn+skp][2]))
					port = atoi(&argv[argn+skp][2]);
			} skp++; argn--;
		} else
			argv[argn] = argv[argn+skp];
	} argc -= skp;

	if (argc!=2) {
		printf("Usage: loadmem [-p [port]] srcfile\n");
		exit(-1);
	}

	fpin = fopen(argv[0], "rb");
	if (fpin == NULL) {
		fprintf(stderr, "Could not open %s\n", argv[1]);
		exit(-1);
	}

	if (use_usb)
		m_fpga = new FPGA(new USBI());
	else
		m_fpga = new FPGA(new NETCOMMS(FPGAHOST, port));


	try {
		int	nr;
		pos = SDRAMBASE;
		do {
			nr = BUFLN;
			if (nr + pos > SDRAMBASE*2)
				nr = SDRAMBASE*2 - pos;
			nr = fread(buf, sizeof(FPGA::BUSW), nr, fpin);
			if (nr <= 0)
				break;

			m_fpga->writei(pos, nr, buf);
			pos += nr;
		} while((nr > 0)&&(pos < 2*SDRAMBASE));

		printf("SUCCESS::fully wrote full file to memory (pos = %08x)\n", pos);
	} catch(BUSERR a) {
		fprintf(stderr, "BUS Err while writing at address 0x%08x\n", a.addr);
		fprintf(stderr, "... is your program too long for this memory?\n");
		exit(-2);
	} catch(...) {
		fprintf(stderr, "Other error\n");
		exit(-3);
	}

	fclose(fpin);
	delete	m_fpga;
}

