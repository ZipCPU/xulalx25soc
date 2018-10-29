################################################################################
##
## Filename:	bench/formal/Makefile
##
## Project:	XuLA2-LX25 SoC based upon the ZipCPU
##
## Purpose:	To direct the formal verification of any XuLA2-LX25 components.
##
## Targets:	The default target, all, tests all of the components defined
##		within this module.
##
## Creator:	Dan Gisselquist, Ph.D.
##		Gisselquist Technology, LLC
##
################################################################################
##
## Copyright (C) 2017, Gisselquist Technology, LLC
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
################################################################################
##
##
TESTS := wbsdram
.PHONY: $(TESTS)
all: $(TESTS)
RTL := ../../rtl
SMTBMC  := yosys-smtbmc
# SOLVER  := -s z3
SOLVER  := -s yices
# SOLVER  := -s boolector
BMCARGS := --presat $(SOLVER)
# BMCARGS := $(SOLVER)
INDARGS := $(SOLVER) -i

SDRAM    := wbsdram

$(SDRAM) : $(SDRAM).check
$(SDRAM).check: $(RTL)/$(SDRAM).v fwb_slave.v $(SDRAM).ys
	rm -f $@
	sby -f $(SDRAM).sby
	touch $@


.PHONY: clean
clean:
	rm -f $(SDRAM)/
	rm -f *.check
