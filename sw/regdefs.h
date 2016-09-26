////////////////////////////////////////////////////////////////////////////////
//
// Filename:	regdefs.h
//
// Project:	XuLA2 board
//
// Purpose:	
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
#ifndef	REGDEFS_H
#define	REGDEFS_H

// #define	R_RESET		0x00000100
// #define	R_STATUS	0x00000101
// #define	R_CONTROL	0x00000101
#define	R_VERSION	0x00000101
#define	R_ICONTROL	0x00000102
#define	R_BUSERR	0x00000103
#define	R_ITIMER	0x00000104
#define	R_DATE		0x00000105
#define	R_GPIO		0x00000106
#define	R_UART_CTRL	0x00000107
#define	R_PWM_DATA	0x00000108
#define	R_PWM_INTERVAL	0x00000109
#define	R_UART_RX	0x0000010a
#define	R_UART_TX	0x0000010b
#define	R_SPIF_EREG	0x0000010c
#define	R_SPIF_CREG	0x0000010d
#define	R_SPIF_SREG	0x0000010e
#define	R_SPIF_IDREG	0x0000010f
#define	R_CLOCK		0x00000110
#define	R_TIMER		0x00000111
#define	R_STOPWATCH	0x00000112
#define	R_CKALARM	0x00000113
#define	R_CKSPEED	0x00000114

// And because the flash driver needs these constants defined ...
#define	R_QSPI_EREG	0x0000010c
#define	R_QSPI_CREG	0x0000010d
#define	R_QSPI_SREG	0x0000010e
#define	R_QSPI_IDREG	0x0000010f

// GPS registers
//			0x00000114
//			0x00000115
//			0x00000116
//			0x00000117

// WB Scope registers wb_addr[31:3]==30'h23, i.e. 46, 8c, 118
#define	R_QSCOPE	0x00000118	// Quad SPI scope ctrl
#define	R_QSCOPED	0x00000119	//	and data
#define	R_CFGSCOPE	0x0000011a	// Configuration/ICAPE scope control
#define	R_CFGSCOPED	0x0000011b	//	and data
#define	R_RAMSCOPE	0x0000011c	// SDRAM scope control
#define	R_RAMSCOPED	0x0000011d	//	and data
#define	R_CPUSCOPE	0x0000011e	// SDRAM scope control
#define	R_CPUSCOPED	0x0000011f	//	and data
//
// SD Card
#define	R_SDCARD_CTRL	0x00000120
#define	R_SDCARD_DATA	0x00000121
#define	R_SDCARD_FIFOA	0x00000122
#define	R_SDCARD_FIFOB	0x00000123
//
// Unused/open
// #define SOMETHING	0x00000124 -- 0x013f	(28 spaces)
//
// FPGA CONFIG/ICAP REGISTERS
#define	R_CFG_CRC	0x00000140
#define	R_CFG_FAR_MAJ	0x00000141
#define	R_CFG_FAR_MIN	0x00000142
#define	R_CFG_FDRI	0x00000143
#define	R_CFG_FDRO	0x00000144
#define	R_CFG_CMD	0x00000145
#define	R_CFG_CTL	0x00000146
#define	R_CFG_MASK	0x00000147
#define	R_CFG_STAT	0x00000148
#define	R_CFG_LOUT	0x00000149
#define	R_CFG_COR1	0x0000014a
#define	R_CFG_COR2	0x0000014b
#define	R_CFG_PWRDN	0x0000014c
#define	R_CFG_FLR	0x0000014d
#define	R_CFG_IDCODE	0x0000014e
#define	R_CFG_CWDT	0x0000014f
#define	R_CFG_HCOPT	0x00000150
#define	R_CFG_CSBO	0x00000152
#define	R_CFG_GEN1	0x00000153
#define	R_CFG_GEN2	0x00000154
#define	R_CFG_GEN3	0x00000155
#define	R_CFG_GEN4	0x00000156
#define	R_CFG_GEN5	0x00000157
#define	R_CFG_MODE	0x00000158
#define	R_CFG_GWE	0x00000159
#define	R_CFG_GTS	0x0000015a
#define	R_CFG_MFWR	0x0000015b
#define	R_CFG_CCLK	0x0000015c
#define	R_CFG_SEU	0x0000015d
#define	R_CFG_EXP	0x0000015e
#define	R_CFG_RDBK	0x0000015f
#define	R_CFG_BOOTSTS	0x00000160
#define	R_CFG_EYE	0x00000161
#define	R_CFG_CBC	0x00000162

// RAM memory space
#define	RAMBASE		0x00002000
#define	MEMWORDS	(1<<13)
// Flash memory space
#define	SPIFLASH	0x00040000
#define	FLASHWORDS	(1<<18)
// SDRAM memory space
#define	SDRAMBASE	0x00800000
#define	SDRAMWORDS	(1<<25)
// Zip CPU Control and Debug registers
#define	R_ZIPCTRL	0x01000000
#define	R_ZIPDATA	0x01000001


// Interrupt control constants
#define	GIE		0x80000000	// Enable all interrupts
#define	SCOPEN		0x80080008	// Enable WBSCOPE interrupts
#define	ISPIF_EN	0x80040004	// Enable SPI Flash interrupts
#define	ISPIF_DIS	0x00040000	// Disable SPI Flash interrupts
#define	ISPIF_CLR	0x00000004	// Clear pending SPI Flash interrupt

// Flash control constants
#define	ERASEFLAG	0x80000000
#define	DISABLEWP	0x10000000

#define	SZPAGE		64
#define	PGLEN		64
#define	NPAGES		32
#define	SECTORSZ	(NPAGES * SZPAGE)
#define	NSECTORS	256
#define	SECTOROF(A)	((A) & (-1<<10))
#define	PAGEOF(A)	((A) & (-1<<6))

#define	RAMLEN		0x02000

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
