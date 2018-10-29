////////////////////////////////////////////////////////////////////////////////
//
// Filename:	cfgscope.cpp
//
// Project:	XuLA2-LX25 SoC based upon the ZipCPU
//
// Purpose:	To read out, and decompose, the results of the wishbone scope
//		as applied to the ICAPE interaction on a SPARTAN6.
//
//	This is provided together with the wbscope project as an example of
//	what might be done with the wishbone scope.  The intermediate details,
//	though, between this and the wishbone scope are not part of the
//	wishbone scope project.
//
//	Using this particular scope made it a *lot* easier to get the ICAPE2
//	interface up and running, since I was able to see what was going right
//	(or wrong) with the interface as I was developing it.  Sure, it
//	would've been better to get it to work under a simulator instead of
//	with the scope, but not being certain of how the interface was
//	supposed to work made building a simulator difficult.
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

#include "llcomms.h"
#include "usbi.h"
#include "port.h"
#include "regdefs.h"

#define	WBSCOPE		R_CFGSCOPE
#define	WBSCOPEDATA	R_CFGSCOPED

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
	int	skp=0, port = FPGAPORT;
	bool	use_usb = true;

	skp=1;
	for(int argn=0; argn<argc-skp; argn++) {
		if (argv[argn+skp][0] == '-') {
			if (argv[argn+skp][1] == 'u')
				use_usb = true;
			else if (argv[argn+skp][1] == 'p') {
				use_usb = false;
				if (isdigit(argv[argn+skp][2]))
					port = atoi(&argv[argn+skp][2]);
			}
			skp++; argn--;
		} else
			argv[argn] = argv[argn+skp];
	} argc -= skp;

	if (use_usb)
		m_fpga = new FPGA(new USBI());
	else
		m_fpga = new FPGA(new NETCOMMS(FPGAHOST, port));

	signal(SIGSTOP, closeup);
	signal(SIGHUP, closeup);

	unsigned	v, lgln, scoplen;
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

	DEVBUS::BUSW	*buf;
	buf = new DEVBUS::BUSW[scoplen];

	if (true) {
		m_fpga->readz(WBSCOPEDATA, scoplen, buf);
	} else {
		for(unsigned int i=0; i<scoplen; i++)
			buf[i] = m_fpga->readio(WBSCOPEDATA);
	}

	for(unsigned int i=0; i<scoplen; i++) {
		if ((i>0)&&(buf[i] == buf[i-1])&&
				(i<scoplen-1)&&(buf[i] == buf[i+1]))
			continue;
		printf("%6d %08x:", i, buf[i]);
		printf("S(%x) ", (buf[i]>>27)&0x0f);
		if (buf[i] & 0x40000000)
			printf("W "); else printf("R ");
		printf("WB(%s%s%s%s%s)-%s%s%s%s%s",
			(buf[i]&0x2000000)?"CYC":"   ",
			(buf[i]&0x1000000)?"STB":"   ",
			(buf[i]&0x0800000)?"WE":"  ",
			(buf[i]&0x0400000)?"ACK":"   ",
			(buf[i]&0x0200000)?"STL":"   ",
			(buf[i]&0x0100000)?"EDG":"   ",
			(buf[i]&0x0080000)?"CLK":"   ",
			(buf[i]&0x0040000)?"   ":"CEn",
			(buf[i]&0x0020000)?"BSY":"   ",
			(buf[i]&0x0010000)?"  ":"WE");
		if (buf[i]&0x10000)
			printf("->"); // Read
		else	printf("<-");
		printf(" %04x\n", wrev(buf[i] & 0x0ffff));
	}

	if (m_fpga->poll()) {
		printf("FPGA was interrupted\n");
		m_fpga->clear();
	}
	delete	m_fpga;
}

