////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	cpuscope.cpp
//
// Project:	XuLA2-LX25 SoC based upon the ZipCPU
//
// Purpose:	To read out, and decompose, the results of the wishbone scope
//		as applied to the ICAPE2 interaction.
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
// You should have received a copy of the GNU General Public License along
// with this program.  (It's in the $(ROOT)/doc directory, run make with no
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
#include "scopecls.h"

#define	WBSCOPE		R_CPUSCOPE
#define	WBSCOPEDATA	R_CPUSCOPED

#include "zopcodes.h"

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

const char *opcodestr[] = {
	"SUB","AND","ADD","OR","XOR","LSR","LSL","ASR",
	"MPY","LDILO","MPYUHI","MPYSHI","BREV","POPC","ROL","MOV",
	"CMP","TEST","LOD","STO","DIVU","DIVS","LDI","LDI",
	"NOOP","BREAK","LOCK","(rsrvd)","(rsrvd)","(rsrvd)","(rsrvd)","(rsrvd)"
};
const char *regstr[] = {
	"R0","R1","R2","R3","R4","R5","R6","R7","R8","R9","RA","RB","RC",
	"SP","CC","PC"
};

class	CPUSCOPE : public SCOPE {
public:
	CPUSCOPE(FPGA *fpga, unsigned addr, bool vecread)
		: SCOPE(fpga, addr, false, false) {};
	~CPUSCOPE(void) {}
	virtual	void	decode(DEVBUS::BUSW val) const {
		int	i_wb_err, gie, alu_illegal, newpc, mem_busy, stb, we,
			maddr, ins, pfval, alu_pc;
		int	pfcyc, pfstb, pfaddr;

		i_wb_err    = (val>>31)&1;
		gie         = (val>>30)&1;
		alu_illegal = (val>>29)&1;
		newpc       = (val>>28)&1;
		mem_busy    = (val>>27)&1;
		stb         = (val>>26)&1;
		we          = (val>>25)&1;
		maddr       = (val>>16)&0x01ff;
		ins         = (val>>16)&0x07ff;
		pfval       = (val>>15)&1;
		pfcyc       = (val>>14)&1;
		pfstb       = (val>>13)&1;
		pfaddr      = (val & 0x1fff);
		alu_pc      = (val & 0x7fff);

		printf("%s%s%s%s%s ",
			(i_wb_err)?"E ":"  ",
			(gie)?"GIE":"   ",
			(alu_illegal)?"ILL":"   ",
			(newpc)?"NPC":"   ",
			(mem_busy)?"MBSY":"    ");
		if (mem_busy)
			printf("M:%s%s@..%4x", (stb)?"STB":"   ",(we)?"W":"R",
				maddr);
		else {
			int	inreg = (ins>>6)&0x0f;
			int	opcode = ((ins>>1)&0x1f);
			const char *incode = opcodestr[opcode];
			printf("I:%03x %5s,%s", (ins<<1), incode, regstr[inreg]);
		}

		if (pfval)
			printf(" V: %04x%4s", alu_pc, "");
		else
			printf(" %s%s@%04x",
				(pfcyc)?"CYC":"   ",
				(pfstb)?"STB":"   ",
				pfaddr);
	}
};

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

	CPUSCOPE *scope = new CPUSCOPE(m_fpga, WBSCOPE, false);
	if (!scope->ready()) {
		printf("Scope is not yet ready:\n");
		scope->decode_control();
	} else
		scope->read();
	delete	m_fpga;
}

