////////////////////////////////////////////////////////////////////////////////
//
// Filename:	ziprun.cpp
//
// Project:	XuLA2 board
//
// Purpose:	To load a program for the ZipCPU into memory.
//
// 	Steps:
//		1. Halt and reset the CPU
//		2. Load memory
//		3. Clear the cache
//		4. Clear any registers
//		5. Set the PC to point to the FPGA local memory
//	THIS DOES NOT START THE PROGRAM!!  The CPU is left in the halt state.
//	To actually start the program, execute a ./wbregs cpu 0.  (Actually,
//	any value between 0x0 and 0x1f will work, the difference being what
//	register you will be able to inspect while the CPU is running.)
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

bool	iself(const char *fname) {
	FILE	*fp;
	bool	ret = true;
	fp = fopen(fname, "rb");

	if (!fp)	return false;
	if (0x7f != fgetc(fp))	ret = false;
	if ('E'  != fgetc(fp))	ret = false;
	if ('L'  != fgetc(fp))	ret = false;
	if ('F'  != fgetc(fp))	ret = false;
	fclose(fp);
	return 	ret;
}

long	fgetwords(FILE *fp) {
	// Return the number of words in the current file, and return the 
	// file as though it had never been adjusted
	long	fpos, flen;
	fpos = ftell(fp);
	if (0 != fseek(fp, 0l, SEEK_END)) {
		fprintf(stderr, "ERR: Could not determine file size\n");
		perror("O/S Err:");
		exit(-2);
	} flen = ftell(fp);
	if (0 != fseek(fp, fpos, SEEK_SET)) {
		fprintf(stderr, "ERR: Could not seek on file\n");
		perror("O/S Err:");
		exit(-2);
	} flen /= sizeof(FPGA::BUSW);
	return flen;
}

FPGA	*m_fpga;
class	SECTION {
public:
	unsigned	m_start, m_len;
	FPGA::BUSW	m_data[1];
};

SECTION	**singlesection(int nwords) {
	fprintf(stderr, "NWORDS = %d\n", nwords);
	size_t	sz = (2*(sizeof(SECTION)+sizeof(SECTION *))
		+(nwords-1)*(sizeof(FPGA::BUSW)));
	char	*d = (char *)malloc(sz);
	SECTION **r = (SECTION **)d;
	memset(r, 0, sz);
	r[0] = (SECTION *)(&d[2*sizeof(SECTION *)]);
	r[0]->m_len   = nwords;
	r[1] = (SECTION *)(&r[0]->m_data[r[0]->m_len]);
	r[0]->m_start = 0;
	r[1]->m_start = 0;
	r[1]->m_len   = 0;

	return r;
}

SECTION **rawsection(const char *fname) {
	SECTION		**secpp, *secp;
	unsigned	num_words;
	FILE		*fp;
	int		nr;

	fp = fopen(fname, "r");
	if (fp == NULL) {
		fprintf(stderr, "Could not open: %s\n", fname);
		exit(-1);
	}

	if ((num_words=fgetwords(fp)) > MEMWORDS) {
		fprintf(stderr, "File overruns Block RAM\n");
		exit(-1);
	}
	secpp = singlesection(num_words);
	secp = secpp[0];
	secp->m_start = RAMBASE;
	secp->m_len = num_words;
	nr= fread(secp->m_data, sizeof(FPGA::BUSW), num_words, fp);
	if (nr != (int)num_words) {
		fprintf(stderr, "Could not read entire file\n");
		perror("O/S Err:");
		exit(-2);
	} assert(secpp[1]->m_len == 0);

	return secpp;
}

unsigned	byteswap(unsigned n) {
	unsigned	r;

	r = (n&0x0ff); n>>= 8;
	r = (r<<8) | (n&0x0ff); n>>= 8;
	r = (r<<8) | (n&0x0ff); n>>= 8;
	r = (r<<8) | (n&0x0ff); n>>= 8;

	return r;
}

// #define	CHEAP_AND_EASY
#ifdef	CHEAP_AND_EASY
#else
#include <libelf.h>
#include <gelf.h>

void	elfread(const char *fname, unsigned &entry, SECTION **&sections) {
	Elf	*e;
	int	fd, i;
	size_t	n;
	char	*id;
	Elf_Kind	ek;
	GElf_Ehdr	ehdr;
	GElf_Phdr	phdr;
	const	bool	dbg = false;

	if (elf_version(EV_CURRENT) == EV_NONE) {
		fprintf(stderr, "ELF library initialization err, %s\n", elf_errmsg(-1));
		perror("O/S Err:");
		exit(EXIT_FAILURE);
	} if ((fd = open(fname, O_RDONLY, 0)) < 0) {
		fprintf(stderr, "Could not open %s\n", fname);
		perror("O/S Err:");
		exit(EXIT_FAILURE);
	} if ((e = elf_begin(fd, ELF_C_READ, NULL))==NULL) {
		fprintf(stderr, "Could not run elf_begin, %s\n", elf_errmsg(-1));
		exit(EXIT_FAILURE);
	}

	ek = elf_kind(e);
	if (ek == ELF_K_ELF) {
		; // This is the kind of file we should expect
	} else if (ek == ELF_K_AR) {
		fprintf(stderr, "Cannot run an archive!\n");
		exit(EXIT_FAILURE);
	} else if (ek == ELF_K_NONE) {
		;
	} else {
		fprintf(stderr, "Unexpected ELF file kind!\n");
		exit(EXIT_FAILURE);
	}

	if (gelf_getehdr(e, &ehdr) == NULL) {
		fprintf(stderr, "getehdr() failed: %s\n", elf_errmsg(-1));
		exit(EXIT_FAILURE);
	} if ((i=gelf_getclass(e)) == ELFCLASSNONE) {
		fprintf(stderr, "getclass() failed: %s\n", elf_errmsg(-1));
		exit(EXIT_FAILURE);
	} if ((id = elf_getident(e, NULL)) == NULL) {
		fprintf(stderr, "getident() failed: %s\n", elf_errmsg(-1));
		exit(EXIT_FAILURE);
	} if (i != ELFCLASS32) {
		fprintf(stderr, "This is a 64-bit ELF file, ZipCPU ELF files are all 32-bit\n");
		exit(EXIT_FAILURE);
	}

	if (dbg) {
	printf("    %-20s 0x%jx\n", "e_type", (uintmax_t)ehdr.e_type);
	printf("    %-20s 0x%jx\n", "e_machine", (uintmax_t)ehdr.e_machine);
	printf("    %-20s 0x%jx\n", "e_version", (uintmax_t)ehdr.e_version);
	printf("    %-20s 0x%jx\n", "e_entry", (uintmax_t)ehdr.e_entry);
	printf("    %-20s 0x%jx\n", "e_phoff", (uintmax_t)ehdr.e_phoff);
	printf("    %-20s 0x%jx\n", "e_shoff", (uintmax_t)ehdr.e_shoff);
	printf("    %-20s 0x%jx\n", "e_flags", (uintmax_t)ehdr.e_flags);
	printf("    %-20s 0x%jx\n", "e_ehsize", (uintmax_t)ehdr.e_ehsize);
	printf("    %-20s 0x%jx\n", "e_phentsize", (uintmax_t)ehdr.e_phentsize);
	printf("    %-20s 0x%jx\n", "e_shentsize", (uintmax_t)ehdr.e_shentsize);
	printf("\n");
	}


	// Check whether or not this is an ELF file for the ZipCPU ...
	if (ehdr.e_machine != 0x0dadd) {
		fprintf(stderr, "This is not a ZipCPU ELF file\n");
		exit(EXIT_FAILURE);
	}

	// Get our entry address
	entry = ehdr.e_entry;


	// Now, let's go look at the program header
	if (elf_getphdrnum(e, &n) != 0) {
		fprintf(stderr, "elf_getphdrnum() failed: %s\n", elf_errmsg(-1));
		exit(EXIT_FAILURE);
	}

	unsigned total_octets = 0, current_offset=0, current_section=0;
	for(i=0; i<(int)n; i++) {
		total_octets += sizeof(SECTION *)+sizeof(SECTION);

		if (gelf_getphdr(e, i, &phdr) != &phdr) {
			fprintf(stderr, "getphdr() failed: %s\n", elf_errmsg(-1));
			exit(EXIT_FAILURE);
		}

		if (dbg) {
		printf("    %-20s 0x%x\n", "p_type",   phdr.p_type);
		printf("    %-20s 0x%jx\n", "p_offset", phdr.p_offset);
		printf("    %-20s 0x%jx\n", "p_vaddr",  phdr.p_vaddr);
		printf("    %-20s 0x%jx\n", "p_paddr",  phdr.p_paddr);
		printf("    %-20s 0x%jx\n", "p_filesz", phdr.p_filesz);
		printf("    %-20s 0x%jx\n", "p_memsz",  phdr.p_memsz);
		printf("    %-20s 0x%x [", "p_flags",  phdr.p_flags);

		if (phdr.p_flags & PF_X)	printf(" Execute");
		if (phdr.p_flags & PF_R)	printf(" Read");
		if (phdr.p_flags & PF_W)	printf(" Write");
		printf("]\n");
		printf("    %-20s 0x%jx\n", "p_align", phdr.p_align);
		}

		total_octets += phdr.p_memsz;
	}

	char	*d = (char *)malloc(total_octets + sizeof(SECTION)+sizeof(SECTION *));
	memset(d, 0, total_octets);

	SECTION **r = sections = (SECTION **)d;
	current_offset = (n+1)*sizeof(SECTION *);
	current_section = 0;

	for(i=0; i<(int)n; i++) {
		r[i] = (SECTION *)(&d[current_offset]);

		if (gelf_getphdr(e, i, &phdr) != &phdr) {
			fprintf(stderr, "getphdr() failed: %s\n", elf_errmsg(-1));
			exit(EXIT_FAILURE);
		}

		if (dbg) {
		printf("    %-20s 0x%jx\n", "p_offset", phdr.p_offset);
		printf("    %-20s 0x%jx\n", "p_vaddr",  phdr.p_vaddr);
		printf("    %-20s 0x%jx\n", "p_paddr",  phdr.p_paddr);
		printf("    %-20s 0x%jx\n", "p_filesz", phdr.p_filesz);
		printf("    %-20s 0x%jx\n", "p_memsz",  phdr.p_memsz);
		printf("    %-20s 0x%x [", "p_flags",  phdr.p_flags);

		if (phdr.p_flags & PF_X)	printf(" Execute");
		if (phdr.p_flags & PF_R)	printf(" Read");
		if (phdr.p_flags & PF_W)	printf(" Write");
		printf("]\n");

		printf("    %-20s 0x%jx\n", "p_align", phdr.p_align);
		}

		current_section++;

		r[i]->m_start = phdr.p_vaddr;
		r[i]->m_len   = phdr.p_filesz/ sizeof(FPGA::BUSW);

		current_offset += phdr.p_memsz + sizeof(SECTION);

		// Now, let's read in our section ...
		if (lseek(fd, phdr.p_offset, SEEK_SET) < 0) {
			fprintf(stderr, "Could not seek to file position %08lx\n", phdr.p_offset);
			perror("O/S Err:");
			exit(EXIT_FAILURE);
		} if (phdr.p_filesz > phdr.p_memsz)
			phdr.p_filesz = 0;
		if (read(fd, r[i]->m_data, phdr.p_filesz) != (int)phdr.p_filesz) {
			fprintf(stderr, "Didnt read entire section\n");
			perror("O/S Err:");
			exit(EXIT_FAILURE);
		}

		// Next, we need to byte swap it from big to little endian
		for(unsigned j=0; j<r[i]->m_len; j++)
			r[i]->m_data[j] = byteswap(r[i]->m_data[j]);

		if (dbg) for(unsigned j=0; j<r[i]->m_len; j++)
			fprintf(stderr, "ADR[%04x] = %08x\n", r[i]->m_start+j,
			r[i]->m_data[j]);
	}

	r[i] = (SECTION *)(&d[current_offset]);
	r[current_section]->m_start = 0;
	r[current_section]->m_len   = 0;

	elf_end(e);
	close(fd);
}
#endif

void	usage(void) {
	printf("USAGE: ziprun [-hmprux] <zip-program-file>\n");
	printf("\n"
"\t-h\tDisplay this usage statement\n"
"\t-m\tClear unused memory locations.  Note this only applies to SDRAM\n"
"\t\t(if used) and block ram, not flash.\n"
"\t-p [PORT]\tConnect to the XuLA device across a network access\n"
"\t\tconnection using port PORT, rather than attempting a USB\n"
"\t\tconnection.  If PORT is not given, %s:%d will be\n"
"\t\tassumed as a default.\n"
"\t-u\tAccess the XuLA board via the USB connector [DEFAULT]\n"
"\t-x\tClear all of the ZipCPU registers to a known initial state\n\n",
	FPGAHOST,FPGAPORT);
}

int main(int argc, char **argv) {
	int		skp=0, port = FPGAPORT;
	bool		use_usb = true, permit_raw_files = false;
	unsigned	entry = RAMBASE;
	bool		clear_registers = false, clear_memory = false;
	FLASHDRVR	*flash = NULL;

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
			case 'm':
				clear_memory = true;
				fprintf(stderr, "Clear memory feature not yet implemented\n");
				exit(EXIT_FAILURE);
				break;
			case 'p':
				use_usb = false;
				if (isdigit(argv[argn+skp][2]))
					port = atoi(&argv[argn+skp][2]);
				break;
			case 'r':
				permit_raw_files = true;
				break;
			case 'u':
				use_usb = true;
				break;
			case 'x':
				clear_registers = true;
				break;
			} skp++; argn--;
		} else
			argv[argn] = argv[argn+skp];
	} argc -= skp;

	if (use_usb)
		m_fpga = new FPGA(new USBI());
	else
		m_fpga = new FPGA(new NETCOMMS(FPGAHOST, port));

	if ((argc<=0)||(access(argv[0],R_OK)!=0)) {
		printf("Usage: ziprun obj-file\n");
		printf("\n"
"\tziprun loads the object file into memory, resets the CPU, and leaves it\n"
"\tin a halted state ready to start running the object file.\n");
		exit(-1);
	} const char *codef = argv[0];

	printf("Halting the CPU\n");
	m_fpga->usleep(5);
	m_fpga->writeio(R_ZIPCTRL, CPU_RESET|CPU_HALT);

	try {
		SECTION	**secpp = NULL, *secp;

		if(iself(codef)) {
#ifndef	CHEAP_AND_EASY
			// zip-readelf will help with both of these ...
			elfread(codef, entry, secpp);

			/*
			fprintf(stderr, "Secpp = %08lx\n", (unsigned long)secpp);
			for(int i=0; secpp[i]->m_len; i++) {
				secp = secpp[i];
				fprintf(stderr, "Sec[%2d] - %08x - %08x\n",
					i, secp->m_start,
					secp->m_start+secp->m_len);
			} */
#else
			char	tmpbuf[TMP_MAX], cmdbuf[256];
			int	unused_fd;

			strcpy(tmpbuf, "/var/tmp/ziprunXXXX");

			// Make a temporary file
			unused_fd = mkostemp(tmpbuf, O_CREAT|O_TRUNC|O_RDWR);
			// Close it immediately, since we won't be writing to it
			// ourselves
			close(unused_fd);

			// Now we write to it, as part of calling objcopy
			// 
			sprintf(cmdbuf, "zip-objcopy -S -O binary --reverse-bytes=4 %s %s", codef, tmpbuf);

			if (system(cmdbuf) != 0) {
				unlink(tmpbuf);
				fprintf(stderr, "ZIPRUN: Could not comprehend ELF binary\n");
				exit(-2);
			}

			secpp = rawsection(tmpbuf);
			unlink(tmpbuf);
			entry = RAMBASE;
#endif
		} else if (permit_raw_files) {
			secpp = rawsection(codef);
			entry = RAMBASE;
		} else {
			fprintf(stderr, "ERR: %s is not in ELF format\n", codef);
			exit(EXIT_FAILURE);
		}

		// assert(secpp[1]->m_len = 0);
		for(int i=0; secpp[i]->m_len; i++) {
			bool	valid = false;
			secp=  secpp[i];
			if ((secp->m_start >= RAMBASE)&&(secp->m_start+secp->m_len <= RAMBASE+MEMWORDS))
				valid = true;
			else if ((secp->m_start >= SDRAMBASE)&&(secp->m_start+secp->m_len <= SDRAMBASE+SDRAMWORDS))
				valid = true;
			else if ((secp->m_start >= SPIFLASH)&&(secp->m_start+secp->m_len <= SPIFLASH+FLASHWORDS))
				valid = true;
			if (!valid) {
				fprintf(stderr, "No such memory on board: 0x%08x - %08x\n",
					secp->m_start, secp->m_start+secp->m_len);
				exit(-2);
			}
		}

		if (clear_memory) for(int i=0; secpp[i]->m_len; i++) {
			secp = secpp[i];
			if ((secp->m_start >= RAMBASE)
					&&(secp->m_start+secp->m_len
							<= RAMBASE+MEMWORDS)) {
				printf("Clearing Block ram\n");
				FPGA::BUSW	zbuf[128], a;
				memset(zbuf, 0, 128*sizeof(FPGA::BUSW));
				for(a=RAMBASE; a<RAMBASE+MEMWORDS; a+=128)
					m_fpga->writei(a, 128, zbuf);
				break;
			}
		} m_fpga->readio(R_VERSION); // Check for buserrors

		if (clear_memory) for(int i=0; secpp[i]->m_len; i++) {
			secp = secpp[i];
			if ((secp->m_start >= SDRAMBASE)
					&&(secp->m_start+secp->m_len
						<= SDRAMBASE+SDRAMWORDS)) {
				FPGA::BUSW	zbuf[128], a;
				printf("Clearing SDRam\n");
				memset(zbuf, 0, 128*sizeof(FPGA::BUSW));
				for(a=SDRAMBASE; a<SDRAMBASE+SDRAMWORDS; a+=128)
					m_fpga->writei(a, 128, zbuf);
				break;
			}
		} m_fpga->readio(R_VERSION); // Check for buserrors

		printf("Loading memory\n");
		for(int i=0; secpp[i]->m_len; i++) {
			bool	inflash=false;

			secp = secpp[i];
			if ((secp->m_start >= SPIFLASH)
					&&(secp->m_start+secp->m_len
							<= SPIFLASH+FLASHWORDS))
				inflash = true;
			if (inflash) {
				if (!flash)
					flash = new FLASHDRVR(m_fpga);
				flash->write(secp->m_start, secp->m_len, secp->m_data, true);
			} else if (secp->m_len < (1<<16)) {
				m_fpga->writei(secp->m_start, secp->m_len, secp->m_data);
			} else {
				// The load amount is so big, we'd like to let
				// the user know where we're at along the way.
				for(unsigned k=0; k<secp->m_len; k+=(1<<16)) {
					unsigned ln = (1<<16),
						st = secp->m_start+k;
					if (st+ln > secp->m_start+secp->m_len)
						ln = (secp->m_start+secp->m_len-st);
					if (ln <= 0) break;
					printf("Loading MEM[%08x]-MEM[%08x] ...\r",
						st,st+ln-1); fflush(stdout);
					m_fpga->writei(st, ln, &secp->m_data[k]);
					m_fpga->readio(R_VERSION); // Check for buserrors
				} printf("Loaded  MEM[%08x]-MEM[%08x]      \n",
					secp->m_start,
					secp->m_start+secp->m_len-1);
				fflush(stdout);
			}
			printf("%08x - %08x\n", secp->m_start, secp->m_start+secp->m_len);
		}
		m_fpga->readio(R_ZIPCTRL); // Check for bus errors

		// Clear any buffers
		printf("Clearing the cache\n");
		m_fpga->writeio(R_ZIPCTRL, CPU_RESET|CPU_HALT|CPU_CLRCACHE);
		m_fpga->readio(R_VERSION);

		if (clear_registers) {
			printf("Clearing all registers to zero\n");
			// Clear all registers to zero
			for(int i=0; i<32; i++) {
				m_fpga->writeio(R_ZIPCTRL, CPU_HALT|i);
				m_fpga->writeio(R_ZIPDATA, 0);
			}
		} m_fpga->readio(R_VERSION); // Check for bus errors

		// Start in interrupt mode
		m_fpga->writeio(R_ZIPCTRL, CPU_HALT|CPU_sCC);
		m_fpga->writeio(R_ZIPDATA, 0x000);

		// Set our entry point into our code
		m_fpga->writeio(R_ZIPCTRL, CPU_HALT|CPU_sPC);
		m_fpga->writeio(R_ZIPDATA, entry);

		printf("The CPU should be fully loaded, you may now start\n");
		printf("it.  To start the CPU, type wbregs cpu 0\n");
	} catch(BUSERR a) {
		fprintf(stderr, "\nXULA-BUS error @0x%08x\n", a.addr);
		m_fpga->writeio(R_ZIPCTRL, CPU_RESET|CPU_HALT|CPU_CLRCACHE);
		exit(-2);
	}

	delete	m_fpga;
}

