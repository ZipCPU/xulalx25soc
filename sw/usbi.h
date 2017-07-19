////////////////////////////////////////////////////////////////////////////////
//
// Filename:	usbi.h
//
// Project:	XuLA2-LX25 SoC based upon the ZipCPU
//
// Purpose:	This package attempts to convet a JTAG over USB based
//		communication system into something similar to a serial port
//	based communication system.  Some differences include the fact that,
//	if the USB port isn't polled, nothing comes out of the port.  Hence,
//	on connecting (or polling for the first time) ... there might be a 
//	bunch of stuff to (initially) ignore.
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
#ifndef	USBI_H
#define	USBI_H

#include <libusb.h>

#define	VENDOR_ID		0x04d8
#define	PRODUCT_ID		0x0ff8c
#define	XESS_INTERFACE		0
#define	XESS_ENDPOINT_OUT	0x01
#define	XESS_ENDPOINT_IN	0x81
#define	XESS_ENDPOINT_IN	0x81
//
#define	JTAG_CMD	0x4f
#define	GET_TDO_MASK	0x01
#define	PUT_TMS_MASK	0x02
#define	TMS_VAL_MASK	0x04
#define	PUT_TDI_MASK	0x08
#define	TDI_VAL_MASK	0x10
//
// #define	USER1_INSTR	0x02	// a SIX bit two
#define	USB_PKTLEN	32
#define	RCV_BUFLEN	512
#define	RCV_BUFMASK	(RCV_BUFLEN-1)

#include "llcomms.h"

class	USBI : public LLCOMMSI { // USB Interface
private:
	char	m_rbuf[RCV_BUFLEN];
	char	m_txbuf[2*USB_PKTLEN], m_rxbuf[2*USB_PKTLEN];
	int	m_rbeg, m_rend;

	libusb_context		*m_usb_context;
	libusb_device		**m_usb_dev_list;
	libusb_device_handle	*m_xula_usb_device;

	virtual	int	pop_fifo(char *buf, int len);
	virtual	void	push_fifo(char *buf, int len);
	virtual	void	raw_read(int len, int timeout_ms);
	virtual	void	flush_read(void);

public:
	USBI(void);

	virtual	void	close(void);
	virtual	int	read(char *buf, int len);
	virtual	int	read(char *buf, int len, int timeout_ms);
	virtual	void	write(char *buf, int len);
	virtual	bool	poll(unsigned ms);
	virtual	int	available(void);
};

#endif // USBI_H

