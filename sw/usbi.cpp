////////////////////////////////////////////////////////////////////////////////
//
// Filename:	usbi.cpp
//
// Project:	XuLA2-LX25 SoC based upon the ZipCPU
//
// Purpose:	Creates a USB port, similar to a serial port, out of the
//		XuLA2 JTAG interface S/W.
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
#include <unistd.h>
#include <string.h>
#include <ctype.h>
// #include <usb.h>

#include <libusb.h>

#include "llcomms.h"
#include "usbi.h"

const	bool	DEBUG = false;

// Walk us through the JTAG Chain:
//	5-1's to go to test/reset
//	0	to go to Run-Test/Idle
//	1	to go to select-dr-scan
//	1	to go to select-ir-scan
//	0	to go to capture-ir
//	0	to go to shift-ir
//	(6-bit code 0x02 through TDI to IR, while sending 0-bits to TMS)
//	1	to leave shift IR and go to exit1-ir
//	1	to go to update-ir
//	1	to go to select-dr-scan
//	0	to go to capture-dr
//	0	to go to shift-dr
#define	RESET_JTAG_LEN	12
static const char	RESET_TO_USER_DR[RESET_JTAG_LEN] = {
	JTAG_CMD,
	21, // clocks
	0,0,0,	// Also clocks, higher order bits
	PUT_TMS_MASK | PUT_TDI_MASK, // flags
	(char)(0x0df),	// TMS: Five ones, then one zero, and two ones -- low bits first
	0x00,	// TDI: irrelevant here
	(char)0x80,	// TMS: two zeros, then six zeros
	0x08,	// TDI: user command #1, bit reversed
	0x03,	// TMS: three ones, then two zeros
	0x00	// TDI byte -- irrelevant here
	//
	// 0xc0, // TDI byte -- user command #2
	// 0x40,	// TDI: user command #1
	// 0x0c, // TDI byte -- user command #2, bit reversed
};

//
//	TMS: 
#define	TX_DR_LEN	12
static const	char	TX_DR_BITS[TX_DR_LEN] = {
	JTAG_CMD,
	48, // clocks
	0,0,0,	// Also clocks, higher order bits
	PUT_TDI_MASK, // flags
	(char)0x0ff, 0, 0, 0, 0, 0	// Six data bytes
	// module_id = 255
	// 32'h(payload.length)
	// payload
	// module_id + payload.len + num_result_bits, length=32 + payload ???
};

//
//	TMS: 
//	
#define	REQ_RX_LEN	6
static const	char	REQ_RX_BITS[REQ_RX_LEN] = {
	JTAG_CMD,
	(const char)((USB_PKTLEN-REQ_RX_LEN)*8), // bits-requested
	0,0,0,	// Also clocks, higher order bits
	GET_TDO_MASK|TDI_VAL_MASK, // flags:TDI is kept low here, so no TDI flag
	// No data given, since there's no info to send or receive
	// Leave the result in shift-DR mode
};

/*
#define	RETURN_TO_RESET_LEN	7
static const char	RETURN_TO_RESET[RETURN_TO_RESET_LEN] = {
	JTAG_CMD,
	5, // clocks
	0,0,0,	// Also clocks, higher order bits
	PUT_TMS_MASK, // flags
	(char)0x0ff, // Five ones
};
*/

USBI::USBI(void) {
	int	config;

	m_total_nread = 0;

	if (0 != libusb_init(&m_usb_context)) {
		fprintf(stderr, "Error initializing the USB library\n");
		perror("O/S Err:");
		exit(-1);
	}

	m_xula_usb_device = libusb_open_device_with_vid_pid(m_usb_context,
		VENDOR_ID, PRODUCT_ID);
	if (!m_xula_usb_device) {
		fprintf(stderr, "Could not open XuLA device\n");
		perror("O/S Err:");

		libusb_exit(m_usb_context);
		exit(-1);
	}

	if (0 != libusb_get_configuration(m_xula_usb_device, &config)) {
		fprintf(stderr, "Could not get configuration\n");
		perror("O/S Err:");

		libusb_close(m_xula_usb_device);
		libusb_exit(m_usb_context);
		exit(-1);
	}

	if (0 != libusb_claim_interface(m_xula_usb_device, XESS_INTERFACE)) {
		fprintf(stderr, "Could not claim interface\n");
		perror("O/S Err:");

		libusb_close(m_xula_usb_device);
		libusb_exit(m_usb_context);
		exit(-1);
	}

	unsigned char	abuf[RESET_JTAG_LEN];
	int	actual_length = RESET_JTAG_LEN, r;

	memcpy(abuf, RESET_TO_USER_DR, RESET_JTAG_LEN);
	r = libusb_bulk_transfer(m_xula_usb_device, XESS_ENDPOINT_OUT,
		abuf, RESET_JTAG_LEN, &actual_length, 4);
	if ((r!=0)||(actual_length != RESET_JTAG_LEN)) {
		// Try clearing the queue twice, then doing this
		do {
			r = libusb_bulk_transfer(m_xula_usb_device, XESS_ENDPOINT_IN,
				(unsigned char *)m_rxbuf, USB_PKTLEN, &actual_length, 20);
		} while((r==0)&&(actual_length > 0));

		r = libusb_bulk_transfer(m_xula_usb_device, XESS_ENDPOINT_OUT,
			abuf, RESET_JTAG_LEN, &actual_length, 4);
		if ((r != 0)||(actual_length != RESET_JTAG_LEN)) {
			printf("Some error took place requesting RESET_TO_USER_DR\n");
			printf("r = %d, actual_length = %d (vs %d requested)\n",
				r, actual_length, RESET_JTAG_LEN);
			perror("O/S Err");
			exit(-2);
		}
	}

	// Initialize our read FIFO
	m_rbeg = m_rend = 0;

	flush_read();
}

void	USBI::close(void) {
	// Release our interface
	if (0 != libusb_release_interface(m_xula_usb_device, XESS_INTERFACE)) {
		fprintf(stderr, "Could not release interface\n");
		perror("O/S Err:");

		libusb_close(m_xula_usb_device);
		libusb_exit(m_usb_context);
		exit(-1);
	}

	// And then close our device with
	libusb_close(m_xula_usb_device);

	// And just before exiting, we free our USB context
	libusb_exit(m_usb_context);
}

void	USBI::write(char *buf, int len) {
	int	r, actual_length;

	if (len >= USB_PKTLEN) {
		const int	nv = USB_PKTLEN/2-1;
		for(int pos=0; pos<len; pos+=nv)
			write(&buf[pos], (len-pos>nv)?(nv):len-pos);
	} else {
		memset(m_txbuf, 0, len+6);
		m_txbuf[0] = JTAG_CMD;
		m_txbuf[1] = len * 8;
		m_txbuf[2] = 0;
		m_txbuf[3] = 0;
		m_txbuf[4] = 0;
		m_txbuf[5] = PUT_TDI_MASK | GET_TDO_MASK;

		for(int i=0; i<len; i++)
			m_txbuf[6+i] = buf[i];
		// printf("WRITE::(buf=%*s, %d)\n", len, buf, len);
		r = libusb_bulk_transfer(m_xula_usb_device, XESS_ENDPOINT_OUT,
			(unsigned char*)m_txbuf, len+6, &actual_length, 0);
		if ((r!=0)||(actual_length != len+6)) {
			printf("WRITE::(buf, %d) -- ERR\n", len+6);
			printf("r = %d, actual_length = %d (!= %d requested)\n", r,
				actual_length, len+6);

			if (r == -7) {
				r = libusb_bulk_transfer(m_xula_usb_device,
					XESS_ENDPOINT_OUT,
					(unsigned char*)m_txbuf, len+6,
					&actual_length, 2);
				if ((r!=0)||(actual_length != len+6)) {
					printf("WRITE::(buf, %d) -- ERR\n", len+6);
					printf("r = %d, actual_length = %d (!= %d requested)\n", r,
						actual_length, len+6);
					perror("O/S Err");

					exit(-2);
				}
			} else {
				perror("O/S Err");
				exit(-2);
			}
		}

		// Try to read back however many bytes we can
		r = libusb_bulk_transfer(m_xula_usb_device, XESS_ENDPOINT_IN,
			(unsigned char*)m_rxbuf, USB_PKTLEN, &actual_length, 20);
		if ((r==0)&&(actual_length > 0)) {
			push_fifo(m_rxbuf, actual_length);
		} else {
			printf("Some error took place in receiving\n");
			perror("O/S Err");
		}
	}
}

int	USBI::read(char *buf, int len) {
	return read(buf, len, 4);
}

int	USBI::read(char *buf, int len, int timeout_ms) {
	int	left = len, nr=0;

	// printf("USBI::read(%d) (FIFO is %d-%d)\n", len, m_rend, m_rbeg);
	nr = pop_fifo(buf, left);
	left -= nr;
	
	while(left > 0) {
		raw_read(left, timeout_ms);
		nr = pop_fifo(&buf[len-left], left);
		left -= nr;

		// printf("\tWHILE (nr = %d, LEFT = %d, len=%d)\n", nr, left, len);
		if (nr == 0)
			break;
	}

	// printf("READ %d characters (%d req, %d left)\n", len-left, len, left);
	return len-left;
}

void	USBI::raw_read(const int clen, int timeout_ms) {
	int	avail = (m_rbeg - m_rend)&(RCV_BUFMASK), actual_length;
	int	len = clen;
	if (len > RCV_BUFMASK-avail)
		len = RCV_BUFMASK-avail;
	if (len > 26)
		len = 26;

	if (DEBUG) printf("USBI::RAW-READ(%d, was %d)\n", len, clen);
	memcpy(m_txbuf, REQ_RX_BITS, REQ_RX_LEN);

	// I have chased process hangs to this line, but have no more
	// information than that somewhere within libusb_bulk_transfer,
	// libusb_handle_events_completed,
	// libusb_handle_events_timeout_completed, ?, poll(), there seems
	// to be a bug.
	//
	// I'm not certain if the bug is in the Linux kernel, or in the 
	// Xess tools.  I will note that, when a followup attempt is made
	// to read from the device, previously unread data may still get dumped
	// to it.  Therefore, be careful to clear the device upon starting
	// any process.
	//
	if(DEBUG) { printf("["); fflush(stdout); }
	int r = libusb_bulk_transfer(m_xula_usb_device, XESS_ENDPOINT_OUT,
		(unsigned char *)m_txbuf, REQ_RX_LEN, &actual_length,
		timeout_ms);
	if(DEBUG) { printf("]"); fflush(stdout); }

	if ((r==0)&&(actual_length == REQ_RX_LEN)) {
	} else if (r == -7) {
		// Nothing to read in the timeout provided
		// We'll have to request this data again ... later
		return;
	} else {
		printf("READ(WRITE,READ-REQ) -- ERR\n");
		printf("r = %d, actual_length = %d (!= %d requested)\n", r,
			actual_length, len+6);
		perror("O/S Err");
		exit(-2);
	}

	// Try to read back however many bytes we can
	r = libusb_bulk_transfer(m_xula_usb_device, XESS_ENDPOINT_IN,
		(unsigned char *)m_rxbuf, USB_PKTLEN, &actual_length, 0);
	if ((r==0)&&(actual_length > 0)) {
		if (DEBUG) {
			printf("RAW-READ() -> %d Read\n", actual_length);
			for(int i=0; i<actual_length; i++)
				printf("%02x ", m_rxbuf[i] & 0x0ff);
			printf("\n");
		}
		push_fifo(m_rxbuf, actual_length);
	} else if (r == -7) {
		// Nothing to read in the timeout provided
		// Return, adding nothing to our FIFO
	} else {
		printf("Some error took place in receiving\n");
		perror("O/S Err");
	}

	// fprintf(stderr, "\tUSBI::RAW-READ() -- COMPLETE (%d avail)\n",
		// (m_rbeg-m_rend)&(RCV_BUFMASK));
}

void	USBI::flush_read(void) {
	while(poll(4)) {
		m_rbeg = m_rend = 0;
	}
}

void	USBI::push_fifo(char *buf, int len) {
	char	last = 0;
	char	*sptr = buf;

	// printf("Pushing %d items onto FIFO (%d - %d)\n", len, m_rend, m_rbeg);
	if (m_rbeg != m_rend)
		last = m_rbuf[m_rend];
	if (DEBUG)
		printf("\tPushing:");
	for(int i=0; i<len; i++) {
		char v = *sptr++;
		if (((v & 0x80)||((unsigned char)v < 0x10))&&(v == last)) {
			// printf("\tSkipping: %02x\n", v & 0x0ff);
		} else if ((unsigned char)v == 0x0ff) {
		} else {
			m_rbuf[m_rbeg] = v;
			m_rbeg = (m_rbeg+1)&(RCV_BUFMASK);
			if (DEBUG)
				printf(" %02x", v & 0x0ff);
		} last = v;
	} if (DEBUG) printf("\n");
}

int	USBI::pop_fifo(char *buf, int len) {
	int	avail = (m_rbeg - m_rend)&(RCV_BUFMASK);
	int	left = len;
	int	nr = 0;

	// printf("Attempting to pop %d items from FIFO (%d - %d)\n",
	//		len, m_rend, m_rbeg);
	while((avail > 0)&&(left > 0)) {
		int ln = RCV_BUFLEN-m_rend;
		if (ln > left)
			ln = left;
		if (ln > avail)
			ln = avail;
		memcpy(&buf[len-left], &m_rbuf[m_rend], ln);
		left   -= ln;
		avail  -= ln;
		m_rend  = (m_rend + ln)&(RCV_BUFMASK);
		nr     += ln;

		if (DEBUG) {
			printf("P:");
			for(int i=0; i<ln; i++)
				printf("%02x ", buf[len-left-ln+i]);
		}
	} if (DEBUG) printf("\n");

	/*
	if (nr > 0)
		printf("\tPopped %d items, buf[0] = %02x (%d - %d)\n",
			nr, buf[0], m_rend, m_rbeg);
	else
		printf("\tPopped nothing, %d - %d\n", m_rend, m_rbeg);
	*/

	return nr;
}

bool	USBI::poll(unsigned ms) {
	int	avail = (m_rbeg-m_rend)&(RCV_BUFMASK);
	bool	r = true;

	// printf("POLL request\n");

	if ((avail < 2)&&((avail<1)||(m_rbuf[m_rend]&0x80)||(m_rbuf[m_rend]<0x10))) {
		raw_read(4,ms);
		avail = (m_rbeg-m_rend)&(RCV_BUFMASK);
		// printf("%d availabe\n", avail);

		char	v = (m_rbuf[(m_rbeg-1)&(RCV_BUFMASK)]);
		while(((v&0x80)==0)&&((unsigned)v>=0x10)&&(avail < RCV_BUFMASK-32)) {
			raw_read(26,ms);
			avail = (m_rbeg-m_rend)&(RCV_BUFMASK);
		}
		if (avail < 1)
			r = false;
		else if ((avail==1)&&((m_rbuf[m_rend]&0x80)||(m_rbuf[m_rend]<0x10)))
			r = false;
	}

	// printf("USBI::poll() -> %s (%d avail)\n", (r)?"true":"false", avail);
	return r;
}

int	USBI::available(void) {
	int	avail = (m_rbeg-m_rend)&(RCV_BUFMASK);

	if (avail > 1)
		return avail;
	else if ((avail == 1)&&((m_rbuf[m_rend]&0x80)||(m_rbuf[m_rend]<0x10)))
		return 1;
	else
		return 0;
}
