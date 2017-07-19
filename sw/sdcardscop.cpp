////////////////////////////////////////////////////////////////////////////////
//
// Filename:	sdcardscop.cpp
//
// Project:	XuLA2-LX25 SoC based upon the ZipCPU
//
// Purpose:	To read out, and decompose, the results of the wishbone scope
//		as applied to the SD-Card interaction over the SPI port.
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

void	decode(DEVBUS::BUSW val) {
	int	csn, sck, mosi, miso;
	int	iwi, iws, ird, idat, odat;
	int	grant;
	int	cmd, rsp;

	csn  = ((val>>24)>>3)&1;
	sck  = ((val>>24)>>2)&1;
	mosi = ((val>>24)>>1)&1;
	miso = ((val>>24)   )&1;

	iws = (val>>30)&1;
	iwi = (val>>29)&1;	// ll_idle
	ird =  (val>>28)&1;	// ll_out_stb

	idat = (val>>8)&0x0ff;
	odat = (val&0x0ff);

	cmd = (val>>21)&0x07;
	rsp = (val>>17)&0x07;

	grant=(val>>20)&1;

	printf("%sSPI[%d,%d,%d,%d] (%d,%d) %02x %s%s [LL] %s %02x",
		(grant)?"   ":"!G-",
		csn, sck, mosi, miso,
		cmd, rsp,
		idat,
		(iws)?"-":" ",
		(iwi)?">":" ",
		(ird)?"->":"  ", odat);
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

	bool	compressed = false, vector_read = true;
	DEVBUS::BUSW	addrv = 0;

	if (vector_read) {
		m_fpga->readz(WBSCOPEDATA, scoplen, buf);
	} else {
		for(unsigned int i=0; i<scoplen; i++)
			buf[i] = m_fpga->readio(WBSCOPEDATA);
	}

	if(compressed) {
		for(int i=0; i<(int)scoplen; i++) {
			if ((buf[i]>>31)&1) {
				addrv += (buf[i]&0x7fffffff);
				printf(" ** \n");
				continue;
			}
			printf("%10d %08x: ", addrv++, buf[i]);
			decode(buf[i]);
			printf("\n");
		}
	} else {
		for(int i=0; i<(int)scoplen; i++) {
			if ((i>0)&&(buf[i] == buf[i-1])&&(i<(int)(scoplen-1))) {
				if ((i>2)&&(buf[i] != buf[i-2]))
					printf(" **** ****\n");
				continue;
			} printf("%9d %08x: ", i, buf[i]);
			decode(buf[i]);
			printf("\n");
		}
	}

	if (m_fpga->poll()) {
		printf("FPGA was interrupted\n");
		m_fpga->clear();
	}

	delete[] buf;
	delete	m_fpga;
}

