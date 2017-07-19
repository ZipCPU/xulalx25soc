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
## Copyright (C) 2015-2017, Gisselquist Technology, LLC
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
## You should have received a copy of the GNU General Public License along
## with this program.  (It's in the $(ROOT)/doc directory.  Run make with no
## target there if the PDF file isn't present.)  If not, see
## <http://www.gnu.org/licenses/> for a copy.
##
## License:	GPL, v3, as defined and found on www.gnu.org,
##		http://www.gnu.org/licenses/gpl.html
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
SUBMAKE:= $(MAKE) --no-print-directory -C

#
#
#
# Now that we know that all of our required components exist, we can build
# things
#
#
#
# Create a datestamp file, so that we can check for the build-date when the
# project was put together.
#
.PHONY: datestamp
datestamp:
	@bash -c 'if [ ! -e $(YYMMDD)-build.v ]; then rm -f 20??????-build.v; perl xilinx/mkdatev.pl > $(YYMMDD)-build.v; rm -f rtl/builddate.v; fi'
	@bash -c 'if [ ! -e rtl/builddate.v ]; then cd rtl; cp ../$(YYMMDD)-build.v builddate.v; fi'

#
#
# Make a tar archive of this file, as a poor mans version of source code control
# (Sorry ... I've been burned too many times by files I've wiped away ...)
#
ARCHIVEFILES := $(BENCH) $(SW) $(RTL) $(SIM) $(NOTES) $(PROJ) $(BIN) $(CONSTRAINTS) $(AUTODATA) README.md
.PHONY: archive
archive:
	tar --transform s,^,$(YYMMDD)-xula/, -chjf $(YYMMDD)-xula.tjz $(BENCH) $(SW) $(RTL) $(NOTES) $(PROJ) $(BIN) $(CONSTRAINTS)

#
#
# Verify that the rtl has no bugs in it, while also creating a Verilator
# simulation class library that we can then use for simulation
#
.PHONY: rtl
rtl: verilated
.PHONY: verilated
verilated:
	+@$(SUBMAKE) rtl
#
#
# Build a simulation of this entire design
#
.PHONY: bench
bench: rtl
	$(SUBMAKE) rtl

#
#
# A master target to build all of the support software
#
.PHONY: sw
sw:
	$(SUBMAKE) sw

.PHONY: bit
bit:
	$(SUBMAKE) xilinx xula.bit

.PHONY: load	
load:	bit
	xsload -b xula2-lx25 --fpga xilinx/xula.bit
	
.PHONY: xload
xload:	
	xsload -b xula2-lx25 --fpga xilinx/toplevel.bit

.PHONY: timing
timing:
	$(SUBMAKE) xilinx timing



.PHONY: clean
clean:
	+$(SUBMAKE) auto-data     clean
	+$(SUBMAKE) sim/verilated clean
	+$(SUBMAKE) rtl           clean
	+$(SUBMAKE) sw/zlib       clean
	+$(SUBMAKE) sw/board      clean
	+$(SUBMAKE) sw/host       clean
