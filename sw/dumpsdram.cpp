////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	dumpsdram.cpp
//
// Project:	XuLA2 board
//
// Purpose:	Read local memory, dump into a file.
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
	FILE		*fp, *fpin;
	unsigned	pos=0;
	int		port = FPGAPORT, skp;
	const int	BUFLN = 127;
	FPGA::BUSW	*buf = new FPGA::BUSW[BUFLN],
			*cmp = new FPGA::BUSW[BUFLN];
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
		printf("Usage: dumpsdram [-p [port]] srcfile outfile\n");
		exit(-1);
	}

	/*
	for(int i=0; i<argc; i++) {
		printf("ARG[%d] = %s\n", i, argv[i]);
	} */

	fpin = fopen(argv[0], "rb");
	if (fpin == NULL) {
		fprintf(stderr, "Could not open %s\n", argv[1]);
		exit(-1);
	}

	if (use_usb)
		m_fpga = new FPGA(new USBI());
	else
		m_fpga = new FPGA(new NETCOMMS(FPGAHOST, port));


	// Read our file and copy it into memory
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

			if (false) {
				for(int i=0; i<nr; i++)
					m_fpga->writeio(pos+i, buf[i]);
			} else
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

	rewind(fpin);

	fp = fopen(argv[1], "wb");
	if (fp == NULL) {
		fprintf(stderr, "Could not open: %s\n", argv[2]);
		exit(-1);
	}

	unsigned	mmaddr[65536], mmval[65536], mmidx = 0;

	// Read it back from memory
	try {
		pos = SDRAMBASE;
		const unsigned int MAXRAM = SDRAMBASE*2;
		bool	mismatch = false;
		unsigned	total_reread = 0;
		do {
			int nw, nr;
			if (MAXRAM-pos > BUFLN)
				nr = BUFLN;
			else
				nr = MAXRAM-pos;

			if (false) {
				for(int i=0; i<nr; i++)
					buf[i] = m_fpga->readio(pos+i);
			} else
				m_fpga->readi(pos, nr, buf);

			pos += nr;
			nw = fwrite(buf, sizeof(FPGA::BUSW), nr, fp);
			if (nw < nr) {
				printf("Only wrote %d of %d words!\n", nw, nr);
				exit(-2);
			} // printf("nr = %d, pos = %08x (%08x / %08x)\n", nr,
			//	pos, SDRAMBASE, MAXRAM);

			{int cr;
			cr = fread(cmp, sizeof(FPGA::BUSW), nr, fpin);
			total_reread += cr;
			for(int i=0; i<cr; i++)
				if (cmp[i] != buf[i]) {
					printf("MISMATCH: MEM[%08x] = %08x(read) != %08x(expected)\n",
						pos-nr+i, buf[i], cmp[i]);
					mmaddr[mmidx] = pos-nr+i;
					mmval[mmidx] = cmp[i];
					if (mmidx < 65536)
						mmidx++;
					mismatch = true;
				}
			if (cr != nr) {
				printf("Only read %d words from our input file\n", total_reread);
				break;
			}
			}
		} while(pos < MAXRAM);
		if (mismatch)
			printf("Read %04x (%6d) words from memory.  These did not match the source file.  (Failed test)\n",
				pos-SDRAMBASE, pos-SDRAMBASE);
		else
			printf("Successfully  read&copied %04x (%6d) words from memory\n",
				pos-SDRAMBASE, pos-SDRAMBASE);
	} catch(BUSERR a) {
		fprintf(stderr, "BUS Err at address 0x%08x\n", a.addr);
		fprintf(stderr, "... is your program too long for this memory?\n");
		exit(-2);
	} catch(...) {
		fprintf(stderr, "Other error\n");
		exit(-3);
	}

	for(unsigned i=0; i<mmidx; i++) {
		unsigned bv = m_fpga->readio(mmaddr[i]);
		if (bv == mmval[i])
			printf("Re-match, MEM[%08x]\n", mmaddr[i]);
		else
			printf("2ndary Fail: MEM[%08x] = %08x(read) != %08x(expected)\n",
				mmaddr[i], bv, mmval[i]);
	}

	delete	m_fpga;
}

