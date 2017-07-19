################################################################################
##
## Filename:	Makefile
##
## Project:	XuLA2-LX25 SoC based upon the ZipCPU
##
## Purpose:	
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
PROGRAMS := $(OBJDIR) wbregs netusb wbsettime dumpflash	\
	dumpsdram ziprun ramscope zipstate zipdbg cfgscope loadmem	\
	sdcardscop uartscope
all: $(PROGRAMS)
CXX := g++
LIBUSBINC := -I/usr/include/libusb-1.0/
LIBUSBDIR := -L/usr/lib/x86_64-linux-gnu
OBJDIR := obj-pc
# ZIPD := /home/dan/work/rnd/zipcpu/trunk/sw/zasm
BUSSRCS := ttybus.cpp llcomms.cpp regdefs.cpp usbi.cpp
SOURCES := ziprun.cpp zipdbg.cpp dumpsdram.cpp wbregs.cpp netusb.cpp	\
		flashdrvr.cpp loadmem.cpp $(BUSSRCS)
HEADERS := llcomms.h ttybus.h devbus.h regdefs.h usbi.h flashdrvr.h
OBJECTS := $(addprefix $(OBJDIR)/,$(subst .cpp,.o,$(SOURCES)))
BUSOBJS := $(addprefix $(OBJDIR)/,$(subst .cpp,.o,$(BUSSRCS)))
CFLAGS := -g -Wall $(LIBUSBINC) -I. -I../rtl
LIBS := -lusb-1.0
SUBMAKE := $(MAKE) --no-print-directory -C

%.o: $(OBJDIR)/%.o
$(OBJDIR)/%.o: %.cpp
	$(mk-objdir)
	$(CXX) $(CFLAGS) -c $< -o $@
$(OBJDIR)/cpuscope.o: cpuscope.cpp scopecls.h
	$(CXX) $(CFLAGS) -c $< -o $@
$(OBJDIR)/scopecls.o: scopecls.cpp scopecls.h
	$(CXX) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(OBJDIR)/ $(PROGRAMS)


# wbprogram: $(OBJDIR)/wbprogram.o $(OBJDIR)/flashdrvr.o $(BUSOBJS)
	# $(CXX) -g $^ -o $@
netusb: $(OBJDIR)/netusb.o $(OBJDIR)/usbi.o $(OBJDIR)/llcomms.o
	$(CXX) $(CFLAGS) $^ $(LIBS) -o $@
wbsettime: $(OBJDIR)/wbsettime.o $(BUSOBJS)
	$(CXX) $(CFLAGS) $^ $(LIBS) -o $@
bustest: $(OBJDIR)/bustest.o $(BUSOBJS)
	$(CXX) $(CFLAGS) $^ $(LIBS) -o $@
wbregs: $(OBJDIR)/wbregs.o $(BUSOBJS)
	$(CXX) $(CFLAGS) $^ $(LIBS) -o $@
dumpflash: $(OBJDIR)/dumpflash.o $(BUSOBJS)
	$(CXX) $(CFLAGS) $^ $(LIBS) -o $@
cfgscope: $(OBJDIR)/cfgscope.o $(BUSOBJS)
	$(CXX) $(CFLAGS) $^ $(LIBS) -o $@
sdcardscop: $(OBJDIR)/sdcardscop.o $(BUSOBJS)
	$(CXX) $(CFLAGS) $^ $(LIBS) -o $@
uartscope: $(OBJDIR)/uartscope.o $(BUSOBJS)
	$(CXX) $(CFLAGS) $^ $(LIBS) -o $@
ramscope: $(OBJDIR)/ramscope.o $(BUSOBJS)
	$(CXX) $(CFLAGS) $^ $(LIBS) -o $@
dumpsdram: $(OBJDIR)/dumpsdram.o $(BUSOBJS)
	$(CXX) $(CFLAGS) $^ $(LIBS) -o $@
loadmem: $(OBJDIR)/loadmem.o $(BUSOBJS)
	$(CXX) $(CFLAGS) $^ $(LIBS) -o $@
ziprun: $(OBJDIR)/ziprun.o $(OBJDIR)/flashdrvr.o $(BUSOBJS) $(OBJDIR)/byteswap.o $(OBJDIR)/zipelf.o
	$(CXX) $(CFLAGS) $^ $(LIBS) -lelf -o $@
zipstate: $(OBJDIR)/zipstate.o $(BUSOBJS)
	$(CXX) $(CFLAGS) $^ $(LIBS) -o $@
cpuscope: $(OBJDIR)/cpuscope.o $(OBJDIR)/scopecls.o $(BUSOBJS)
	$(CXX) $(CFLAGS) $^ $(LIBS) -o $@

# ziprun: $(OBJDIR)/ziprun.o $(BUSOBJS)
	# $(CXX) -g $^ -o $@

#
DBGSRCS  := zopcodes.cpp twoc.cpp
DBGOBJS := $(addprefix $(OBJDIR)/,$(subst .cpp,.o,$(DBGSRCS)))
zipdbg: $(OBJDIR)/zipdbg.o $(BUSOBJS) $(DBGOBJS)
	$(CXX) -g $^ $(LIBS) -lcurses -o $@


usbtst: usbtst.cpp
	$(CXX) $(CFLAGS) usbtst.cpp $(LIBS) -o usbtst
txtest: txtest.cpp
	$(CXX) $(CFLAGS) txtest.cpp $(LIBS) -o txtest

define	mk-objdir
	@bash -c "if [ ! -e $(OBJDIR) ]; then mkdir -p $(OBJDIR); fi"
endef

define	build-depends
	@echo "Building dependency file(s)"
	$(mk-objdir)
	@$(CXX) $(CPPFLAGS) -MM -I$(ZIPD) $(SOURCES) > $(OBJDIR)/xdepends.txt
	@sed -e 's/^.*.o: /$(OBJDIR)\/&/' < $(OBJDIR)/xdepends.txt > $(OBJDIR)/depends.txt
	@rm $(OBJDIR)/xdepends.txt
endef

tags: $(SOURCES) $(HEADERS)
	@echo "Generating tags"
	@ctags $(SOURCES) $(HEADERS)

.PHONY: depends
depends: tags
	$(build-depends)

$(OBJDIR)/depends.txt: $(OBJDIR)/ $(SOURCES) $(HEADERS)
	$(build-depends)

# obj-pc/usbi.o: usbi.cpp
	# $(CXX) $(LIBUSBINC) -I../../fpgalib/sw -c usbi.cpp  -o $@

-include $(OBJDIR)/depends.txt
