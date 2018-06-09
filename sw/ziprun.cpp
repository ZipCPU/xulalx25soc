////////////////////////////////////////////////////////////////////////////////
//
// Filename:	ziprun.cpp
//
// Project:	XuLA2-LX25 SoC based upon the ZipCPU
//
// Purpose:	To load a program for the ZipCPU into memory, whether flash
//		or SDRAM.  This requires a working/running configuration
//	in order to successfully load.
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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

#include "usbi.h"
#include "port.h"
#include "regdefs.h"
#include "flashdrvr.h"
#include "zipelf.h"
#include "byteswap.h"
#include <design.h>

FPGA	*m_fpga;

void	usage(void) {
	printf("USAGE: ziprun [-hpuv] <zip-program-file>\n");
	printf("\n"
"\t-h\tDisplay this usage statement\n"
"\t-p [PORT]\tConnect to the XuLA device across a network access\n"
"\t\tconnection using port PORT, rather than attempting a USB\n"
"\t\tconnection.  If PORT is not given, %s:%d will be\n"
"\t\tassumed as a default.\n"
"\t-r\tActually start the CPU as well, instead of just loading it\n"
"\t-u\tAccess the XuLA board via the USB connector [DEFAULT]\n"
"\t-v\tVerbose\n",
	FPGAHOST,FPGAPORT);
}

const unsigned
	FLASHBASE = SPIFLASH,
	RESET_ADDRESS = FLASHBASE + FLASHBYTES/4;

int main(int argc, char **argv) {
	int		skp=0, port = FPGAPORT;
	bool		use_usb = true, start_when_finished = false, verbose = false;
	unsigned	entry = RAMBASE;
	FLASHDRVR	*flash = NULL;
	const char	*bitfile = NULL, *altbitfile = NULL, *execfile = NULL;

	if (argc < 2) {
		usage();
		exit(EXIT_SUCCESS);
	}

	skp=1;
	for(int argn=0; argn<argc-skp; argn++) {
		if (argv[argn+skp][0] == '-') {
			switch(argv[argn+skp][1]) {
			case 'h':
				usage();
				exit(EXIT_SUCCESS);
				break;
			case 'p':
				use_usb = false;
				if (isdigit(argv[argn+skp][2])) {
					port = atoi(&argv[argn+skp][2]);
					if (verbose)
						printf("No port number given\n");
				}
				if (verbose)
					printf("Using port %d instead of USB\n", port);
				break;
			case 'r':
				start_when_finished = true;
				if (verbose)
					printf("Will start the CPU upon completion\n");
				break;
			case 'u':
				use_usb = true;
				break;
			case 'v':
				verbose = true;
				break;
			default:
				fprintf(stderr, "Unknown option, -%c\n\n",
					argv[argn+skp][0]);
				usage();
				exit(EXIT_FAILURE);
				break;
			} skp++; argn--;
		} else {
			// Anything here must be either the program to load,
			// or a bit file to load
			printf("%s must be some other arg\n", argv[argn]);
			argv[argn] = argv[argn+skp];
		}
	} argc -= skp;


	for(int argn=0; argn<argc; argn++) {
		if (verbose) printf("Re-examining argument: %s\n", argv[argn]);
		if (iself(argv[argn])) {
			if (verbose)
				printf("%s is an ELF file\n", argv[argn]);
			if (execfile) {
				printf("Too many executable files given, %s and %s\n", execfile, argv[argn]);
				usage();
				exit(EXIT_FAILURE);
			} execfile = argv[argn];
		} else { // if (isbitfile(argv[argn]))
			if (!bitfile)
				bitfile = argv[argn];
			else if (!altbitfile)
				altbitfile = argv[argn];
			else {
				printf("Unknown file name or too many files, %s\n", argv[argn]);
				usage();
				exit(EXIT_FAILURE);
			}
		}
	}

	if ((execfile == NULL)&&(bitfile == NULL)) {
		printf("No executable or bit file(s) given!\n\n");
		usage();
		exit(EXIT_FAILURE);
	}

	if ((bitfile)&&(access(bitfile,R_OK)!=0)) {
		// If there's no code file, or the code file cannot be opened
		fprintf(stderr, "Cannot open bitfile, %s\n", bitfile);
		exit(EXIT_FAILURE);
	}

	if ((altbitfile)&&(access(altbitfile,R_OK)!=0)) {
		// If there's no code file, or the code file cannot be opened
		fprintf(stderr, "Cannot open alternate bitfile, %s\n", altbitfile);
		exit(EXIT_FAILURE);
	}

	if ((execfile)&&(access(execfile,R_OK)!=0)) {
		// If there's no code file, or the code file cannot be opened
		fprintf(stderr, "Cannot open executable, %s\n", execfile);
		exit(EXIT_FAILURE);
	}

	char	*fbuf = new char[FLASHBYTES];

	// Set the flash buffer to all ones
	memset(fbuf, -1, FLASHBYTES);

	if (use_usb)
		m_fpga = new FPGA(new USBI());
	else
		m_fpga = new FPGA(new NETCOMMS(FPGAHOST, port));


	// Make certain we can talk to the FPGA
	try {
		unsigned v  = m_fpga->readio(R_VERSION);
		if (v < 0x20170000) {
			fprintf(stderr, "Could not communicate with board (invalid version)\n");
			exit(EXIT_FAILURE);
		}
	} catch(BUSERR b) {
		fprintf(stderr, "Could not communicate with board (BUSERR when reading VERSION)\n");
		exit(EXIT_FAILURE);
	}

	// Halt the CPU
	try {
		printf("Halting the CPU\n");
		m_fpga->writeio(R_ZIPCTRL, CPU_HALT|CPU_RESET);
	} catch(BUSERR b) {
		fprintf(stderr, "Could not halt the CPU (BUSERR)\n");
		exit(EXIT_FAILURE);
	}

	flash = new FLASHDRVR(m_fpga);

	if ((execfile)||(bitfile)) try {
		ELFSECTION	**secpp = NULL, *secp;

		if(iself(execfile)) {
			// zip-readelf will help with both of these ...
			elfread(execfile, entry, secpp);
		} else {
			fprintf(stderr, "ERR: %s is not in ELF format\n", execfile);
			exit(EXIT_FAILURE);
		}

		printf("Loading: %s\n", execfile);
		// assert(secpp[1]->m_len = 0);
		for(int i=0; secpp[i]->m_len; i++) {
			bool	valid = false;
			secp=  secpp[i];

			// Make sure our section is either within block RAM
			if ((secp->m_start >= RAMBASE)
				&&(secp->m_start+secp->m_len
					<= RAMBASE + MEMBYTES))
				valid = true;

#ifdef	FLASH_ACCESS
			// Flash
			if ((secp->m_start >= RESET_ADDRESS)
				&&(secp->m_start+secp->m_len
						<= FLASHBASE+FLASHBYTES))
				valid = true;
#endif

#ifndef	BYPASS_SDRAM_ACCESS
			// or SDRAM
			if ((secp->m_start >= SDRAMBASE)
				&&(secp->m_start+secp->m_len
						<= SDRAMBASE+SDRAMBYTES))
				valid = true;
#endif
			if (!valid) {
				fprintf(stderr, "No such memory on board: 0x%08x - %08x\n",
					secp->m_start, secp->m_start+secp->m_len);
				exit(EXIT_FAILURE);
			}
		}

		unsigned	startaddr = RESET_ADDRESS, codelen = 0;
		for(int i=0; secpp[i]->m_len; i++) {
			secp = secpp[i];

#ifndef	BYPASS_SDRAM_ACCESS
			if ((secp->m_start >= SDRAMBASE)
				&&(secp->m_start+secp->m_len
						<= SDRAMBASE+SDRAMBYTES)) {
				if (verbose)
					printf("Writing to MEM: %08x-%08x\n",
						secp->m_start,
						secp->m_start+secp->m_len);
				unsigned ln = (secp->m_len+3)&-4;
				uint32_t	*bswapd = new uint32_t[ln>>2];
				if (ln != (secp->m_len&-4))
					memset(bswapd, 0, ln);
				memcpy(bswapd, secp->m_data,  ln);
				byteswapbuf(ln>>2, bswapd);
				m_fpga->writei(secp->m_start, ln>>2, bswapd);

				continue;
			}
#endif

			if ((secp->m_start >= RAMBASE)
				  &&(secp->m_start+secp->m_len
						<= RAMBASE+MEMBYTES)) {
				if (verbose)
					printf("Writing to MEM: %08x-%08x\n",
						secp->m_start,
						secp->m_start+secp->m_len);
				unsigned ln = (secp->m_len+3)&-4;
				uint32_t	*bswapd = new uint32_t[ln>>2];
				if (ln != (secp->m_len&-4))
					memset(bswapd, 0, ln);
				memcpy(bswapd, secp->m_data,  ln);
				byteswapbuf(ln>>2, bswapd);
				m_fpga->writei(secp->m_start, ln>>2, bswapd);
				continue;
			}

#ifdef	FLASH_ACCESS
			if ((secp->m_start >= FLASHBASE)
				  &&(secp->m_start+secp->m_len
						<= FLASHBASE+FLASHBYTES)) {
				// Otherwise writing to flash
				if (secp->m_start < startaddr) {
					// Keep track of the first address in
					// flash, as well as the last address
					// that we will write
					codelen += (startaddr-secp->m_start);
					startaddr = secp->m_start;
				} if (secp->m_start+secp->m_len > startaddr+codelen) {
					codelen = secp->m_start+secp->m_len-startaddr;
				} if (verbose)
					printf("Sending to flash: %08x-%08x\n",
						secp->m_start,
						secp->m_start+secp->m_len);

				// Copy this data into our copy of what we want
				// the flash to look like.
				memcpy(&fbuf[secp->m_start-FLASHBASE],
					secp->m_data, secp->m_len);
			}
#endif
		}
		m_fpga->readio(R_ZIPCTRL); // Check for bus errors

#ifdef	FLASH_ACCESS
		if ((flash)&&(codelen>0)&&(!flash->write(startaddr, codelen, &fbuf[startaddr-FLASHBASE], true))) {
			fprintf(stderr, "ERR: Could not write program to flash\n");
			exit(EXIT_FAILURE);
		} else if ((!flash)&&(codelen > 0)) {
			fprintf(stderr, "ERR: Cannot write to flash: Driver didn\'t load\n");
			// fprintf(stderr, "flash->write(%08x, %d, ... );\n", startaddr,
			//	codelen);
		}
#endif

		if (m_fpga) m_fpga->readio(R_VERSION); // Check for bus errors

		// Now ... how shall we start this CPU?
		printf("Clearing the CPUs registers\n");
		for(int i=0; i<32; i++) {
			m_fpga->writeio(R_ZIPCTRL, CPU_HALT|i);
			m_fpga->writeio(R_ZIPDATA, 0);
		}

		m_fpga->writeio(R_ZIPCTRL, CPU_HALT|CPU_CLRCACHE);
		printf("Setting PC to %08x\n", entry);
		m_fpga->writeio(R_ZIPCTRL, CPU_HALT|CPU_sPC);
		m_fpga->writeio(R_ZIPDATA, entry);

		if (start_when_finished) {
			printf("Starting the CPU\n");
			m_fpga->writeio(R_ZIPCTRL, CPU_GO|CPU_sPC);
		} else {
			printf("The CPU should be fully loaded, you may now\n");
			printf("start it (from reset/reboot) with:\n");
			printf("> wbregs cpu 0x0f\n");
			printf("\n");
		}
	} catch(BUSERR a) {
		fprintf(stderr, "\nXULA-BUS error @0x%08x\n", a.addr);
		m_fpga->writeio(R_ZIPCTRL, CPU_RESET|CPU_HALT|CPU_CLRCACHE);
		exit(-2);
	}

	printf("CPU Status is: %08x\n", m_fpga->readio(R_ZIPCTRL));
	if (m_fpga) delete	m_fpga;

	return EXIT_SUCCESS;
}

