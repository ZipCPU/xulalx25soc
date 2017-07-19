////////////////////////////////////////////////////////////////////////////////
//
// Filename:	regdefs.h
//
// Project:	XuLA2-LX25 SoC based upon the ZipCPU
//
// Purpose:	
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
#ifndef	REGDEFS_H
#define	REGDEFS_H

// #define	R_RESET		0x00000400
// #define	R_STATUS	0x00000404
// #define	R_CONTROL	0x00000404
#define	R_VERSION	0x00000404
#define	R_ICONTROL	0x00000408
#define	R_BUSERR	0x0000040c
#define	R_ITIMER	0x00000410
#define	R_DATE		0x00000414
#define	R_GPIO		0x00000418
#define	R_UART_CTRL	0x0000041c
#define	R_PWM_DATA	0x00000420
#define	R_PWM_INTERVAL	0x00000424
#define	R_UART_RX	0x00000428
#define	R_UART_TX	0x0000042c
#define	R_SPIF_EREG	0x00000430
#define	R_SPIF_CREG	0x00000434
#define	R_SPIF_SREG	0x00000438
#define	R_SPIF_IDREG	0x0000043c
#define	R_CLOCK		0x00000440
#define	R_TIMER		0x00000444
#define	R_STOPWATCH	0x00000448
#define	R_CKALARM	0x0000044c
#define	R_CKSPEED	0x00000450

// And because the flash driver needs these constants defined ...
#define	R_QSPI_EREG	0x00000430
#define	R_QSPI_CREG	0x00000434
#define	R_QSPI_SREG	0x00000438
#define	R_QSPI_IDREG	0x0000043c

// GPS registers
//			0x00000450
//			0x00000454
//			0x00000458
//			0x0000045c

// WB Scope registers wb_addr[31:3]==30'h23, i.e. 46, 8c, 118
#define	R_QSCOPE	0x00000460	// Quad SPI scope ctrl
#define	R_QSCOPED	0x00000464	//	and data
#define	R_CFGSCOPE	0x00000468	// Configuration/ICAPE scope control
#define	R_CFGSCOPED	0x0000046c	//	and data
#define	R_RAMSCOPE	0x00000470	// SDRAM scope control
#define	R_RAMSCOPED	0x00000474	//	and data
#define	R_CPUSCOPE	0x00000478	// SDRAM scope control
#define	R_CPUSCOPED	0x0000047c	//	and data
//
// SD Card
#define	R_SDCARD_CTRL	0x00000480
#define	R_SDCARD_DATA	0x00000484
#define	R_SDCARD_FIFOA	0x00000488
#define	R_SDCARD_FIFOB	0x0000048c
//
// Unused/open
// #define SOMETHING	0x00000124 -- 0x013f	(28 spaces)
//
// FPGA CONFIG/ICAP REGISTERS
#define	R_CFG_CRC	0x00000500
#define	R_CFG_FAR_MAJ	0x00000504
#define	R_CFG_FAR_MIN	0x00000508
#define	R_CFG_FDRI	0x0000050c
#define	R_CFG_FDRO	0x00000510
#define	R_CFG_CMD	0x00000514
#define	R_CFG_CTL	0x00000518
#define	R_CFG_MASK	0x0000051c
#define	R_CFG_STAT	0x00000520
#define	R_CFG_LOUT	0x00000524
#define	R_CFG_COR1	0x00000528
#define	R_CFG_COR2	0x0000052c
#define	R_CFG_PWRDN	0x00000530
#define	R_CFG_FLR	0x00000534
#define	R_CFG_IDCODE	0x00000538
#define	R_CFG_CWDT	0x0000053c
#define	R_CFG_HCOPT	0x00000540
#define	R_CFG_CSBO	0x00000548
#define	R_CFG_GEN1	0x0000054c
#define	R_CFG_GEN2	0x00000550
#define	R_CFG_GEN3	0x00000554
#define	R_CFG_GEN4	0x00000558
#define	R_CFG_GEN5	0x0000055c
#define	R_CFG_MODE	0x00000560
#define	R_CFG_GWE	0x00000564
#define	R_CFG_GTS	0x00000568
#define	R_CFG_MFWR	0x0000056c
#define	R_CFG_CCLK	0x00000570
#define	R_CFG_SEU	0x00000574
#define	R_CFG_EXP	0x00000578
#define	R_CFG_RDBK	0x0000057c
#define	R_CFG_BOOTSTS	0x00000580
#define	R_CFG_EYE	0x00000584
#define	R_CFG_CBC	0x00000588

// RAM memory space
#define	RAMBASE		0x00008000
#define	MEMWORDS	(1<<13)
#define	MEMBYTES	(MEMWORDS<<2)
// Flash memory space
#define	SPIFLASH	0x000100000
#define	FLASHWORDS	(1<<18)
#define	FLASHBYTES	(FLASHWORDS<<2)
// SDRAM memory space
#define	SDRAMBASE	0x002000000
#define	SDRAMWORDS	(1<<25)
#define	SDRAMBYTES	(SDRAMWORDS<<2)
// Zip CPU Control and Debug registers
#define	R_ZIPCTRL	0x04000000
#define	R_ZIPDATA	0x04000004


// Interrupt control constants
#define	GIE		0x80000000	// Enable all interrupts
#define	SCOPEN		0x80080008	// Enable WBSCOPE interrupts
#define	ISPIF_EN	0x80040004	// Enable SPI Flash interrupts
#define	ISPIF_DIS	0x00040000	// Disable SPI Flash interrupts
#define	ISPIF_CLR	0x00000004	// Clear pending SPI Flash interrupt

// Flash control constants
#define	ERASEFLAG	0x80000000
#define	DISABLEWP	0x10000000
#define	ENABLEWP	0x00000000

#define	SZPAGEB		256
#define	PGLENB		256
#define	SZPAGEW		64
#define	PGLENW		64
#define	NPAGES		32
#define	SECTORSZB	(NPAGES * SZPAGEB)	// In bytes, not words!!
#define	SECTORSZW	(NPAGES * SZPAGEW)	// In words
#define	NSECTORS	64

#define	SECTOROF(A)	((A) & (-1<<12))
#define	PAGEOF(A)	((A) & (-1<<8))


// ZIP Control sequences
#define	CPU_GO		0x0000
#define	CPU_RESET	0x0040
#define	CPU_INT		0x0080
#define	CPU_STEP	0x0100
#define	CPU_STALL	0x0200
#define	CPU_HALT	0x0400
#define	CPU_CLRCACHE	0x0800
#define	CPU_sR0		(0x0000|CPU_HALT)
#define	CPU_sSP		(0x000d|CPU_HALT)
#define	CPU_sCC		(0x000e|CPU_HALT)
#define	CPU_sPC		(0x000f|CPU_HALT)
#define	CPU_uR0		(0x0010|CPU_HALT)
#define	CPU_uSP		(0x001d|CPU_HALT)
#define	CPU_uCC		(0x001e|CPU_HALT)
#define	CPU_uPC		(0x001f|CPU_HALT)

// Scop definition/sequences
#define	SCOPE_NO_RESET	0x80000000
#define	SCOPE_TRIGGER	(0x08000000|SCOPE_NO_RESET)
#define	SCOPE_DISABLE	(0x04000000)

typedef	struct {
	unsigned	m_addr;
	const char	*m_name;
} REGNAME;

extern	const	REGNAME	*bregs;
extern	const	int	NREGS;
// #define	NREGS	(sizeof(bregs)/sizeof(bregs[0]))

extern	unsigned	addrdecode(const char *v);
extern	const	char *addrname(const unsigned v);

#include "ttybus.h"
// #include "portbus.h"

typedef	TTYBUS	FPGA;

#endif
