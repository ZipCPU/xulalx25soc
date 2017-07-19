////////////////////////////////////////////////////////////////////////////////
//
// Filename:	ramscope.cpp
//
// Project:	XuLA2-LX25 SoC based upon the ZipCPU
//
// Purpose:	To read out, and decompose, the results of the wishbone scope
//		as applied to the SDRAM interaction.
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

#include "usbi.h"
#include "port.h"
#include "llcomms.h"
#include "regdefs.h"

#define	WBSCOPE		R_RAMSCOPE
#define	WBSCOPEDATA	R_RAMSCOPED

FPGA	*m_fpga;
void	closeup(int v) {
	m_fpga->kill();
	exit(0);
}

unsigned brev(const unsigned v) {
	unsigned int r, a;
	a = v;
	r = 0;
	for(int i=0; i<8; i++) {
		r <<= 1;
		r |= (a&1);
		a >>= 1;
	} return r;
}

unsigned wrev(const unsigned v) {
	unsigned r = brev(v&0x0ff);
	r |= brev((v>>8)&0x0ff)<<8;
	return r;
}

int main(int argc, char **argv) {
	bool		skipping = false;
	unsigned	v, lgln, scoplen;
	DEVBUS::BUSW	*buf;

	FPGAOPEN(m_fpga);

	signal(SIGSTOP, closeup);
	signal(SIGHUP, closeup);

	printf("Attempting to read address %08x\n", WBSCOPE);
	v = m_fpga->readio(WBSCOPE);
	if (0x60000000 != (v & 0x60000000)) {
		printf("Scope is not yet ready:\n");
		printf("\tRESET:\t\t%s\n", (v&0x80000000)?"Ongoing":"Complete");
		printf("\tSTOPPED:\t%s\n", (v&0x40000000)?"Yes":"No");
		printf("\tTRIGGERED:\t%s\n", (v&0x20000000)?"Yes":"No");
		printf("\tPRIMED:\t\t%s\n", (v&0x10000000)?"Yes":"No");
		printf("\tMANUAL:\t\t%s\n", (v&0x08000000)?"Yes":"No");
		printf("\tDISABLED:\t%s\n", (v&0x04000000)?"Yes":"No");
		printf("\tZERO:\t\t%s\n", (v&0x02000000)?"Yes":"No");
		exit(0);
	} else printf("SCOPD = %08x\n", v);

	lgln = (v>>20) & 0x1f;
	scoplen = (1<<lgln);

	buf = new DEVBUS::BUSW[scoplen];

	if (false) {
		printf("Attempting vector read\n");
		m_fpga->readz(WBSCOPEDATA, scoplen, buf);

		printf("Vector read complete\n");
	} else {
		for(unsigned int i=0; i<scoplen; i++)
			buf[i] = m_fpga->readio(WBSCOPEDATA);
	}

	for(unsigned int i=0; i<scoplen; i++) {
		int	cmd;

		if ((i>0)&&(buf[i] == buf[i-1])&&
				(i<scoplen-1)&&(buf[i] == buf[i+1])) {
			if (!skipping)
				printf("         ****\n");
			skipping = true;
			continue;
		} skipping = false;
		printf("%6d %08x:", i, buf[i]);
		printf("S(%x) ", (buf[i]>>27)&0x0f);
		if (buf[i] & 0x20000000)
			printf("W "); else printf("R ");
		printf("WB(%s%s%s%s%s",
			(buf[i]&0x80000000)?"CYC":"   ",
			(buf[i]&0x40000000)?"STB":"   ",
			(buf[i]&0x20000000)?"WE":"  ",
			(buf[i]&0x10000000)?"ACK":"   ",
			(buf[i]&0x08000000)?"STL":"   ");
			//
		if ((buf[i]&0xc8000000)==0xc0000000)
			printf("*");
		else
			printf(" ");
		printf(")-SD[%d%d%d%d,%d]",
			(buf[i]&0x04000000)?1:0,
			(buf[i]&0x02000000)?1:0,
			(buf[i]&0x01000000)?1:0,
			(buf[i]&0x00800000)?1:0,
			(buf[i]&0x00600000)>>21);
		cmd = (buf[i] >> 23)&0x0f;
		if (buf[i]&0x00100000)
			printf("<- ");
		else
			printf("-> ");
		printf("%s", (buf[i]&0x00080000)?"P":" "); // Pending
		printf("@%3x,", (buf[i]>>8)&0x07ff);
		/*
		printf(",%s%s%s%s%s", 
			(buf[i]&0x080)?"R":"-",
			(buf[i]&0x040)?"P":".",
			(buf[i]&0x020)?"P":".",
			(buf[i]&0x010)?"P":".",
			(buf[i]&0x008)?"P":".");
		printf("/%x%x%x", 
			(buf[i]>>2)&0x01,
			(buf[i]>>1)&0x01,
			(buf[i]&0x01));
		*/
		printf("/%02x ", buf[i] & 0x0ff);

		if (cmd & 0x8)
			printf("(inactive)");
		switch(cmd) {
			case 0x01: printf("Refresh"); break;
			case 0x02: printf("Precharge"); break;
			case 0x03: printf("Activate"); break;
			case 0x04: printf("Write"); break;
			case 0x05: printf("Read"); break;
			case 0x07: printf("NoOp"); break;
			default: break;
		}

		printf("\n");
	}

	if (m_fpga->poll()) {
		printf("FPGA was interrupted\n");
		m_fpga->clear();
		m_fpga->writeio(R_ICONTROL, SCOPEN);
	}
	delete	m_fpga;
}

