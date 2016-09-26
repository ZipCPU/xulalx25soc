////////////////////////////////////////////////////////////////////////////////
//
// Filename:	netusb.cpp
//
// Project:	XuLA2 board
//
// Purpose:	Forwards a XuLA2 board USB connection over a TCP socket, so
//		a non-local computer can control the board.
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <poll.h>
#include <signal.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>

#include "port.h"
#include "usbi.h"

void	sigstop(int v) {
	fprintf(stderr, "SIGSTOP!!\n");
	exit(0);
}
void	sighup(int v) {
	fprintf(stderr, "SIGHUP!!\n");
	exit(0);
}
void	sigint(int v) {
	fprintf(stderr, "SIGINT!!\n");
	exit(0);
}
void	sigsegv(int v) {
	fprintf(stderr, "SIGSEGV!!\n");
	exit(0);
}
void	sigbus(int v) {
	fprintf(stderr, "SIGBUS!!\n");
	exit(0);
}
void	sigpipe(int v) {
	fprintf(stderr, "SIGPIPE!!\n");
	exit(0);
}

int	setup_listener(const int port) {
	int	skt;
	struct  sockaddr_in     my_addr;

	printf("Listening on port %d\n", port);

	skt = socket(AF_INET, SOCK_STREAM, 0);
	if (skt < 0) {
		perror("Could not allocate socket: ");
		exit(-1);
	}

	// Set the reuse address option
	{
		int optv = 1, er;
		er = setsockopt(skt, SOL_SOCKET, SO_REUSEADDR, &optv, sizeof(optv));
		if (er != 0) {
			perror("SockOpt Err:");
			exit(-1);
		}
	}

	memset(&my_addr, 0, sizeof(struct sockaddr_in)); // clear structure
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	my_addr.sin_port = htons(port);

	if (bind(skt, (struct sockaddr *)&my_addr, sizeof(my_addr))!=0) {
		perror("BIND FAILED:");
		exit(-1);
	}

	if (listen(skt, 1) != 0) {
		perror("Listen failed:");
		exit(-1);
	}

	return skt;
}

class	LINBUFS {
public:
	char	m_iline[512], m_oline[512];
	char	m_buf[256];
	int	m_ilen, m_olen;
	bool	m_connected;

	LINBUFS(void) {
		m_ilen = 0; m_olen = 0; m_connected = false;
	}
};

bool	check_incoming(LINBUFS &lb, USBI *usbp, int confd, int timeout) {
	struct	pollfd	p[2];
	int	pv, nfds;

	if (confd >= 0) {
		// Only do this if we currently have a network connection
		p[0].fd = confd;
		p[0].events = POLLIN | POLLRDHUP | POLLERR;
		nfds = 1;

		if ((pv=poll(p, nfds, timeout)) < 0) {
			perror("Poll Failed!  O/S Err:");
			exit(-1);
		}
		if (p[0].revents & POLLIN) {
			/*
			 else if (p[0].revents)
			printf("UNKNOWN TTY EVENT: %d\n", p[0].revents);
		*/
			int nr = read(confd, lb.m_buf, 256);
			if (nr == 0) {
				lb.m_connected = false;
				if (lb.m_olen > 0) {
					lb.m_oline[lb.m_olen] = '\0';
					printf("< %s\n", lb.m_oline);
				} lb.m_olen = 0;
				close(confd);
			} else if (nr > 0) {
				usbp->write(lb.m_buf, nr);
			} for(int i=0; i<nr; i++) {
				lb.m_oline[lb.m_olen++] = lb.m_buf[i];
				assert(lb.m_buf[i] != '\0');
				if ((lb.m_oline[lb.m_olen-1]=='\n')
					||(lb.m_oline[lb.m_olen-1]=='\r')
					||(lb.m_olen >= (int)sizeof(lb.m_oline)-1)) {
					if (lb.m_olen >= (int)sizeof(lb.m_oline)-1)
						lb.m_oline[lb.m_olen] = '\0';
					else
						lb.m_oline[lb.m_olen-1] = '\0';
					if (lb.m_olen > 1)
						printf("< %s\n", lb.m_oline);
					lb.m_olen = 0;
				}
			}
		} else if ((nfds>1)&&(p[1].revents)) {
			printf("UNKNOWN SKT EVENT: %d\n", p[1].revents);
		}
	}

	if (usbp->poll(2)) {
		int	nr, nrn;

		nr = nrn = usbp->read(lb.m_buf,1,2);
		while((nrn>0)&&(nr<255)&&((lb.m_buf[0]&0x80)==0)&&(lb.m_buf[0]>0x10)) {
			nrn = usbp->read(&lb.m_buf[nr],1,2);
			nr += nrn;
		} if ((nr > 0)&&(confd >= 0)) {
			// Return our result if we have a network
			// connection
			write(confd, lb.m_buf, nr);
		} for(int i=0; i<nr; i++) {
			lb.m_iline[lb.m_ilen++] = lb.m_buf[i];
			if ((lb.m_iline[lb.m_ilen-1]=='\n')
				||(lb.m_iline[lb.m_ilen-1]=='\r')
				||(lb.m_ilen >= (int)sizeof(lb.m_iline)-1)) {
				if (lb.m_ilen >= (int)sizeof(lb.m_iline)-1)
					lb.m_iline[lb.m_ilen] = '\0';
				else
					lb.m_iline[lb.m_ilen-1] = '\0';
				if (lb.m_ilen > 1)
					printf("%c %s\n",
						(confd>=0)?'>':'#', lb.m_iline);
				lb.m_ilen = 0;
			}
		}
	}

	return (pv > 0);
}

int	myaccept(int skt, int timeout) {
	int	con = -1;
	struct	pollfd	p[1];
	int	pv;

	p[0].fd = skt;
	p[0].events = POLLIN | POLLERR;
	if ((pv=poll(p, 1, timeout)) < 0) {
		perror("Poll Failed!  O/S Err:");
		exit(-1);
	} if (p[0].revents & POLLIN) {
		con = accept(skt, 0, 0);
		if (con < 0) {
			perror("Accept failed!  O/S Err:");
			exit(-1);
		}
	} return con;
}

int	main(int argc, char **argv) {
	// First, accept a network connection
	int	skt = setup_listener(FPGAPORT);
	USBI	*usbp;
	bool	done = false;

	signal(SIGSTOP, sigstop);
	signal(SIGBUS, sigbus);
	signal(SIGSEGV, sigsegv);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, sigint);
	signal(SIGHUP, sighup);

	usbp = new USBI();

	LINBUFS	lb;
	while(!done) {
		int	con;

		// Accept a connection before going on
		// Let's call poll(), so we can still read any
		// tty messages even when not accepted
		con = myaccept(skt, 50);
		if (con >= 0) {
			lb.m_connected = true;

			/*
			// Set our new socket as non-blocking
			int flags = fcntl(fd, F_GETFL, 0);
			flags |= O_NONBLOCK;
			fcntl(fd, F_SETFL, flags);
			*/

			// printf("Received a new connection\n");
		}

		// Flush any buffer within the TTY
		while(check_incoming(lb, usbp, -1, 0))
			;

		// Now, process that connection until it's gone
		while(lb.m_connected) {
			check_incoming(lb, usbp, con, -1);
		}
	}

	printf("Closing our socket\n");
	close(skt);
}

