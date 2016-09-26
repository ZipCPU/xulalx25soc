////////////////////////////////////////////////////////////////////////////////
//
// Filename:	regdefs.cpp
//
// Project:	XuLA2 board
//
// Purpose:	To give human readable names to the various registers available
//		internal to the processor on the wishbone bus.  This file is
//	primarily used for name to number translation within wbregs.cpp.
//	All names for a given register are equivalent, save only that the
//	register will always be identified by its first name in any output.
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
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <ctype.h>
#include "regdefs.h"

const	REGNAME	raw_bregs[] = {
	// { R_RESET,	"RESET"			},
	// { R_STATUS,	"STATUS"		},
	// { R_CONTROL,	"CONTROL"		},
	{ R_VERSION,	"VERSION"		},
	{ R_ICONTROL,	"ICONTROL"		},
	{ R_ICONTROL,	"INT"			},
	{ R_ICONTROL,	"PIC"			},
	{ R_ICONTROL,	"INTC"			},
	{ R_BUSERR,	"BUSERR"		},
	{ R_BUSERR,	"BUS"			},
	{ R_DATE,	"DATE"			},
	{ R_GPIO,	"GPIO"			},
	{ R_UART_CTRL,	"UARTCTRL"		},
	{ R_UART_CTRL,	"UART"			},
	{ R_PWM_INTERVAL,"PWMCTRL"		},
	{ R_PWM_INTERVAL,"PWMI"			},
	{ R_PWM_DATA,	"PWMDATA"		},
	{ R_PWM_DATA,	"PWM"			},
	{ R_PWM_DATA,	"PWMAUDIO"		},
	{ R_PWM_DATA,	"AUDIO"			},
	{ R_UART_RX,	"UART-RX"		},
	{ R_UART_RX,	"UARTRX"		},
	{ R_UART_RX,	"RX"			},
	{ R_UART_TX,	"UART-TX"		},
	{ R_UART_TX,	"UARTTX"		},
	{ R_UART_TX,	"TX"			},
	//
	{ R_SPIF_EREG,	"SPIFEREG"		},
	{ R_SPIF_EREG,	"SPIFE"			},
	{ R_SPIF_CREG,	"SPIFCONF"		},
	{ R_SPIF_CREG,	"SPIFC"			},
	{ R_SPIF_SREG,	"SPIFSTAT"		},
	{ R_SPIF_SREG,	"SPIFS"			},
	{ R_SPIF_IDREG, "SPIFID"		},
	{ R_SPIF_IDREG, "SPIFI"			},
	//
	{ R_CLOCK,	"CLOCK"			},
	{ R_CLOCK,	"TIME"			},
	{ R_TIMER,	"TIMER"			},
	{ R_STOPWATCH,	"STOPWACH"		},
	{ R_STOPWATCH,	"STOPWATCH"		},
	{ R_CKALARM,	"CKALARM"		},
	{ R_CKALARM,	"ALARM"			},
	{ R_CKSPEED,	"CKSPEED"		},
	// Scopes are defined and come and go.  Be aware, therefore, not all
	// of these scopes may be defined at the same time.
	{ R_QSCOPE,	"SCOPE"			},
	{ R_QSCOPE,	"SCOP"			},
	{ R_QSCOPED,	"SCOPDATA"		},
	{ R_QSCOPED,	"SCDATA"		},
	{ R_CFGSCOPE,	"CFGSCOPE"		},
	{ R_CFGSCOPE,	"CFGSCOP"		},
	{ R_CFGSCOPED,	"CFGSCOPD"		},
	{ R_CFGSCOPE,	"SDSCOPE"		},
	{ R_CFGSCOPE,	"SDSCOP"		},
	{ R_CFGSCOPED,	"SDSCOPD"		},
	{ R_CPUSCOPED,	"CPUSCOPD"		},
	{ R_RAMSCOPE,	"MEMSCOPE"		},
	{ R_RAMSCOPE,	"MEMSCOP"		},
	{ R_RAMSCOPED,	"MEMSCOPD"		},
	{ R_RAMSCOPE,	"RAMSCOPE"		},
	{ R_RAMSCOPE,	"RAMSCOP"		},
	{ R_RAMSCOPED,	"RAMSCOPD"		},
	{ R_CPUSCOPE,	"CPUSCOPE"		},
	{ R_CPUSCOPE,	"CPUSCOP"		},
	//
	// And now for the SDCard
	//
	{ R_SDCARD_CTRL, "SDCARD"		},
	{ R_SDCARD_DATA, "SDDATA"		},
	{ R_SDCARD_FIFOA, "SDFIF0"		},
	{ R_SDCARD_FIFOA, "SDFIFO"		},
	{ R_SDCARD_FIFOA, "SDFIFA"		},
	{ R_SDCARD_FIFOA, "SDFIFO0"		},
	{ R_SDCARD_FIFOA, "SDFIFOA"		},
	{ R_SDCARD_FIFOB, "SDFIF1"		},
	{ R_SDCARD_FIFOB, "SDFIFB"		},
	{ R_SDCARD_FIFOB, "SDFIFO1"		},
	{ R_SDCARD_FIFOB, "SDFIFOB"		},
	//
	// For working with the ICAPE interface ... if I can ever get a
	// testing environment suitable to prove that it works.
	//
	{ R_CFG_CRC,	"FPGACRC"		},
	{ R_CFG_FAR_MAJ, "FPGAFARH"		},
	{ R_CFG_FAR_MIN, "FPGAFARL"		},
	{ R_CFG_FDRI,	"FPGAFDRI"		},
	{ R_CFG_FDRO,	"FPGAFDRO"		},
	{ R_CFG_CMD,	"FPGACMD"		},
	{ R_CFG_CTL,	"FPGACTL"		},
	{ R_CFG_MASK,	"FPGAMASK"		},
	{ R_CFG_STAT,	"FPGASTAT"		},
	{ R_CFG_LOUT,	"FPGALOUT"		},
	{ R_CFG_COR1,	"FPGACOR1"		},
	{ R_CFG_COR2,	"FPGACOR2"		},
	{ R_CFG_PWRDN,	"FPGAPWRDN"		},
	{ R_CFG_FLR,	"FPGAFLR"		},
	{ R_CFG_IDCODE,	"FPGAIDCODE"		},
	{ R_CFG_CWDT,	"FPGACWDT"		},
	{ R_CFG_HCOPT,	"FPGAHCOPT"		},
	{ R_CFG_CSBO,	"FPGACSBO"		},
	{ R_CFG_GEN1,	"FPGAGEN1"		},
	{ R_CFG_GEN2,	"FPGAGEN2"		},
	{ R_CFG_GEN3,	"FPGAGEN3"		},
	{ R_CFG_GEN4,	"FPGAGEN4"		},
	{ R_CFG_GEN5,	"FPGAGEN5"		},
	{ R_CFG_MODE,	"FPGAMODE"		},
	{ R_CFG_GWE,	"FPGAGWE"		},
	{ R_CFG_GTS,	"FPGAGTS"		},
	{ R_CFG_MFWR,	"FPGAMFWR"		},
	{ R_CFG_CCLK,	"FPGACCLK"		},
	{ R_CFG_SEU,	"FPGASEU"		},
	{ R_CFG_EXP,	"FPGAEXP"		},
	{ R_CFG_RDBK,	"FPGARDBK"		},
	{ R_CFG_BOOTSTS, "BOOTSTS"		},
	{ R_CFG_EYE,	"FPGAEYE"		},
	{ R_CFG_CBC,	"FPGACBC"		},
	//
	//
	{ R_ZIPCTRL,	"ZIPCTRL"		},
	{ R_ZIPCTRL,	"ZIPC"			},
	{ R_ZIPCTRL,	"CPU"			},
	{ R_ZIPCTRL,	"CPUC"			},
	{ R_ZIPDATA,	"ZIPDATA"		},
	{ R_ZIPDATA,	"ZIPD"			},
	{ R_ZIPDATA,	"CPUD"			},
	//
	{ RAMBASE,	"MEM"			},
	{ SPIFLASH,	"FLASH"			},
	{ SDRAMBASE,	"SDRAM"			},
	{ SDRAMBASE,	"RAM"			}
};

#define	RAW_NREGS	(sizeof(raw_bregs)/sizeof(bregs[0]))

const	REGNAME	*bregs = raw_bregs;
const	int	NREGS = RAW_NREGS;

unsigned	addrdecode(const char *v) {
	if (isalpha(v[0])) {
		for(int i=0; i<NREGS; i++)
			if (strcasecmp(v, bregs[i].m_name)==0)
				return bregs[i].m_addr;
		fprintf(stderr, "Unknown register: %s\n", v);
		exit(-2);
	} else
		return strtoul(v, NULL, 0); 
}

const	char *addrname(const unsigned v) {
	for(int i=0; i<NREGS; i++)
		if (bregs[i].m_addr == v)
			return bregs[i].m_name;
	return NULL;
}

