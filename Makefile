################################################################################
##
## Filename:	Makefile
##
## Project:	XuLA2 board
##
## Purpose:	A master project makefile.  It tries to build all targets
##		within the project, mostly by directing subdirectory makes.
##
##
## Creator:	Dan Gisselquist, Ph.D.
##		Gisselquist Technology, LLC
##
################################################################################
##
## Copyright (C) 2015, Gisselquist Technology, LLC
##
## This program is free software (firmware): you can redistribute it and/or
## modify it under the terms of  the GNU General Public License as published
## by the Free Software Foundation, either version 3 of the License, or (at
## your option) any later version.
##
## This program is distributed in the hope that it will be useful, but WITHOUT
## ANY WARRANTY; without even the implied warranty of MERCHANTIBILITY or
## FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
## for more details.
##
## License:	GPL, v3, as defined and found on www.gnu.org,
##		http:##www.gnu.org/licenses/gpl.html
##
##
################################################################################
##
##
.PHONY: all
all:	datestamp verilated sw bench
# Could also depend upon load, if desired, but not necessary
BENCH := `find bench -name Makefile` `find bench -name "*.cpp"` `find bench -name "*.h"`
RTL   := `find rtl -name "*.v"` `find rtl -name Makefile`
NOTES := `find . -name "*.txt"` `find . -name "*.html"`
SW    := `find sw -name "*.cpp"` `find sw -name "*.h"`	\
	`find sw -name "*.sh"` `find sw -name "*.py"`	\
	`find sw -name "*.pl"` `find sw -name Makefile`
DEVSW := `find sw-board -name "*.cpp"` `find sw-board -name "*.h" \
	`find sw-board -name Makefile`
PROJ  := xilinx/xula.prj xilinx/xula.xise xilinx/xula.xst	\
	xilinx/xula.ut xilinx/Makefile
BIN  := `find xilinx -name "*.bit"`
CONSTRAINTS := xula.ucf
YYMMDD:=`date +%Y%m%d`

.PHONY: datestamp
datestamp:
	@bash -c 'if [ ! -e $(YYMMDD)-build.v ]; then rm 20??????-build.v; perl xilinx/mkdatev.pl > $(YYMMDD)-build.v; rm -f rtl/builddate.v; fi'
	@bash -c 'if [ ! -e rtl/builddate.v ]; then cd rtl; cp ../$(YYMMDD)-build.v builddate.v; fi'

.PHONY: archive
archive:
	tar --transform s,^,$(YYMMDD)-xula/, -chjf $(YYMMDD)-xula.tjz $(BENCH) $(SW) $(RTL) $(NOTES) $(PROJ) $(BIN) $(CONSTRAINTS)

.PHONY: rtl
rtl: verilated
.PHONY: verilated
verilated:
	cd rtl ; $(MAKE) --no-print-directory

.PHONY: bench
bench: rtl
	cd bench/cpp ; $(MAKE) --no-print-directory

.PHONY: sw
sw:
	cd sw ; $(MAKE) --no-print-directory

.PHONY: bit
bit:
	cd xilinx ; $(MAKE) --no-print-directory xula.bit

.PHONY: load	
load:	bit
	xsload -b xula2-lx25 --fpga xilinx/xula.bit
	
.PHONY: xload	
xload:	
	xsload -b xula2-lx25 --fpga xilinx/toplevel.bit

.PHONY: timing
timing:
	cd xilinx ; $(MAKE) --no-print-directory timing


