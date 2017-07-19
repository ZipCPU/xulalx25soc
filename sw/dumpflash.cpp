////////////////////////////////////////////////////////////////////////////////
//
// Filename:	dumpflash.cpp
//
// Project:	XuLA2-LX25 SoC based upon the ZipCPU
//
// Purpose:	Read/Empty the entire contents of the flash memory to a file.
//		The flash is unchanged by this process.
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
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

#include "port.h"
#include "regdefs.h"

FPGA	*m_fpga;
void	closeup(int v) {
	m_fpga->kill();
	exit(0);
}

// #define	DUMPMEM		RAMBASE
// #define	DUMPWORDS	MEMWORDS

#define	DUMPMEM		SPIFLASH
#define	DUMPWORDS	FLASHWORDS	// 1MB Flash

int main(int argc, char **argv) {
	FILE	*fp;
	const int	BUFLN = MEMWORDS; // 1MB Flash
	FPGA::BUSW	*buf = new FPGA::BUSW[BUFLN];

	FPGAOPEN(m_fpga);
	fprintf(stderr, "Before starting, nread = %ld\n", 
		m_fpga->m_total_nread);

	// Start with testing the version:
	printf("VERSION: %08x\n", m_fpga->readio(R_VERSION));

	// SPI flash testing
	// Enable the faster (vector) reads
	bool	vector_read = true;
	unsigned	sz;

	if (vector_read) {
		m_fpga->readi(DUMPMEM, BUFLN, buf);
	} else {
		for(int i=0; i<BUFLN; i++) {
			buf[i] = m_fpga->readio(DUMPMEM+i);
			// if (0 == (i&0x0ff))
				printf("i = %02x / %04x, addr = i + %04x = %08x\n", i, BUFLN, DUMPMEM, i+DUMPMEM);
		}
	}
	printf("\nREAD-COMPLETE\n");

	// Now, let's find the end
	sz = BUFLN-1;
	while((sz>0)&&(buf[sz] == 0xffffffff))
		sz--;
	sz+=1;
	printf("The size of the buffer is 0x%06x or %d words\n", sz, sz);

	fp = fopen("spiftest.bin","w");
	fwrite(buf, sizeof(buf[0]), sz, fp);
	fclose(fp);

	printf("The read was accomplished in %ld bytes over the UART\n",
		m_fpga->m_total_nread);

	if (m_fpga->poll())
		printf("FPGA was interrupted\n");
	delete	m_fpga;
}


