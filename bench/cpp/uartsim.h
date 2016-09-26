////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	uartsim.h
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
#ifndef	UARTSIM_H
#define	UARTSIM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

#define	TXIDLE	0
#define	TXDATA	1
#define	RXIDLE	0
#define	RXDATA	1

class	UARTSIM	{
	int	m_skt, m_con;
	unsigned m_setup;
	int	m_nparity, m_fixdp, m_evenp, m_nbits, m_nstop, m_baud_counts;
	int	m_rx_baudcounter, m_rx_state, m_rx_busy,
		m_rx_changectr, m_last_tx;
	int	m_tx_baudcounter, m_tx_state, m_tx_busy;
	unsigned	m_rx_data, m_tx_data;

	void	setup_listener(const int port);
	int	tick(const int i_tx);

public:
	UARTSIM(const int port);
	void	kill(void);
	void	setup(unsigned isetup);
	int	operator()(int i_tx) {
		return tick(i_tx); }
	int	operator()(int i_tx, unsigned isetup) {
		setup(isetup); return tick(i_tx); }
};

#endif
