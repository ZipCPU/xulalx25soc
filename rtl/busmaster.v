`define	XULA25
///////////////////////////////////////////////////////////////////////////
//
// Filename:	busmaster.v
//
// Project:	XuLA2 board
//
// Purpose:	This is the highest level, Verilator simulatable, portion of
//		the XuLA2 core.  You should be able to successfully Verilate 
//	this file, and then build a test bench that tests and proves the
//	capability of anything within here.
//
//	In general, this means the file is little more than a wishbone
//	interconnect that connects multiple devices together.  User-JTAG
//	commands come in via i_rx_stb and i_rx_data.  These are converted into
//	wishbone bus interactions, the results of which come back out via
//	o_tx_data and o_tx_stb.
//
//
// Creator:	Dan Gisselquist, Ph.D.
//		Gisselquist Technology, LLC
//
///////////////////////////////////////////////////////////////////////////
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
///////////////////////////////////////////////////////////////////////////
//

//
// Configuration question #1
//
//	What innate capabilities are built into the board?
//
`define	INCLUDE_ZIPCPU

// Without the ZipCPU competing for the bus, we don't need to delay it by a
// cycle.
`ifndef	INCLUDE_ZIPCPU
`define	NO_ZIP_WBU_DELAY
`endif
`ifdef	VERILATOR
`define	NO_ZIP_WBU_DELAY
`endif

`define	IMPLEMENT_ONCHIP_RAM
`ifndef	VERILATOR
`ifndef	XULA25
// `define	FANCY_ICAP_ACCESS
`endif
`endif
`define	FLASH_ACCESS
`ifdef	XULA25
`define SDCARD_ACCESS
`endif
//


//
// Configuration question #2
//
//	Are any scopes built in to the board?
//

//
// Position #1: The flash scope, or perhaps the wishbone bus/uart/jtag scope
//
// `define	FLASH_SCOPE
`ifndef	FLASH_SCOPE
// `define	WBUS_SCOPE // Occupies the FLASH_SCOPE location, so both cannot be active
`endif
//
// Position #2: The ICAP configuration scope, could also be the SDCard scope
// depending on how we configure ourselves here
//
`ifdef	XULA25
`ifdef	FANCY_ICAP_ACCESS
// `define	CFG_SCOPE // Only defined if we have the access ...
`else
`ifdef	SDCARD_ACCESS
// `define	SDCARD_SCOPE
`endif
`endif
`endif
//
// Position #3: The SDRAM scope / UART scope (never both)
//
// `define	SDRAM_SCOPE
// `define	UART_SCOPE
//
// Position #4: The Zip CPU scope
//
`ifdef	INCLUDE_ZIPCPU
`ifdef	VERILATOR
`define	ZIP_SCOPE
`else // VERILATOR
`ifdef	XULA25
// `define	ZIP_SCOPE
`endif // XULA25
`endif // VERILATOR
`endif // INCLUDE_ZIPCPU

module	busmaster(i_clk, i_rst,
		i_rx_stb, i_rx_data, o_tx_stb, o_tx_data, i_tx_busy,
		// The SPI Flash lines
		o_sf_cs_n, o_sd_cs_n, o_spi_sck, o_spi_mosi, i_spi_miso,
		// The SDRAM lines
		o_ram_cs_n, o_ram_cke, o_ram_ras_n, o_ram_cas_n,
			o_ram_we_n, o_ram_bs, o_ram_addr,
			o_ram_drive_data, i_ram_data, o_ram_data,
			o_ram_dqm,
		// Generic GPIO
		i_gpio, o_gpio, o_pwm,
		i_rx_uart, o_tx_uart);
	parameter	ZIP_ADDRESS_WIDTH=24, NGPO=15, NGPI=15,
			ZA=ZIP_ADDRESS_WIDTH;
	input			i_clk, i_rst;
	// The bus commander, via an external JTAG port
	input			i_rx_stb;
	input		[7:0]	i_rx_data;
	output	wire		o_tx_stb;
	output	wire	[7:0]	o_tx_data;
	input			i_tx_busy;
	// SPI flash control
	output	wire		o_sf_cs_n, o_sd_cs_n;
	output	wire		o_spi_sck, o_spi_mosi;
	input			i_spi_miso;
	// SDRAM control
	output	wire		o_ram_cs_n, o_ram_cke;
	output	wire		o_ram_ras_n, o_ram_cas_n, o_ram_we_n;
	output	wire	[12:0]	o_ram_addr;
	output	wire	[1:0]	o_ram_bs;
	output	wire		o_ram_drive_data;
	input		[15:0]	i_ram_data;
	output	wire	[15:0]	o_ram_data;
	output	wire	[1:0]	o_ram_dqm;
	input 	[(NGPI-1):0]	i_gpio;
	output wire [(NGPO-1):0] o_gpio;
	output	wire		o_pwm;
	input			i_rx_uart;
	output	wire		o_tx_uart;


	//
	//
	// Master wishbone wires
	//
	//
	wire		wb_cyc, wb_stb, wb_we, wb_stall, wb_ack, wb_err;
	wire	[31:0]	wb_data, wb_idata, wb_addr;

	//
	//
	// First BUS master source: The JTAG
	//
	//
	wire	[31:0]	dwb_idata;

	// Wires going to devices
	wire		wbu_cyc, wbu_stb, wbu_we;
	wire	[31:0]	wbu_addr, wbu_data;
	// and then coming from devices
	wire		wbu_ack, wbu_stall, wbu_err;
	wire	[31:0]	wbu_idata;
	// And then headed back home
	wire	w_interrupt;
	// Oh, and the debug control for the ZIP CPU
	wire		wbu_zip_sel, zip_dbg_ack, zip_dbg_stall;
	assign	wbu_zip_sel =((wbu_cyc)&&(wbu_addr[24]));
	wire	[31:0]	zip_dbg_data;
	wire		wbu_dbg;
	wbubus	genbus(i_clk, i_rx_stb, i_rx_data,
			wbu_cyc, wbu_stb, wbu_we, wbu_addr, wbu_data,
`ifdef	INCLUDE_ZIPCPU
			((~wbu_zip_sel)&&(wbu_ack))
				||((wbu_zip_sel)&&(zip_dbg_ack)),
			((~wbu_zip_sel)&&(wbu_stall))
				||((wbu_zip_sel)&&(zip_dbg_stall)),
				wbu_err, (wbu_zip_sel)?zip_dbg_data:dwb_idata,
`else
			wbu_ack, wbu_stall,
				wbu_err, dwb_idata,
`endif
			w_interrupt,
			o_tx_stb, o_tx_data, i_tx_busy,
			wbu_dbg);


	//
	//
	// Second BUS master source: The ZipCPU
	//
	//
	wire		zip_cyc, zip_stb, zip_we, zip_cpu_int;
	wire	[(ZA-1):0]	w_zip_addr;
	wire	[31:0]	zip_addr, zip_data;
	// and then coming from devices
	wire		zip_ack, zip_stall, zip_err;
	wire	dwb_we, dwb_stb, dwb_cyc, dwb_ack, dwb_stall, dwb_err;
	wire	[31:0]	dwb_addr, dwb_odata;
	wire	[8:0]	w_ints_to_zip_cpu;
`ifdef	INCLUDE_ZIPCPU
`ifdef	ZIP_SCOPE
	wire	[31:0]	zip_debug;
`endif
`ifdef	XULA25
	zipsystem #(24'h2000,ZA,10,1,9)
		zippy(i_clk, 1'b0,
			// Zippys wishbone interface
			zip_cyc, zip_stb, zip_we, w_zip_addr, zip_data,
				zip_ack, zip_stall, dwb_idata, zip_err,
			w_ints_to_zip_cpu, zip_cpu_int,
			// Debug wishbone interface
			((wbu_cyc)&&(wbu_zip_sel)),
				((wbu_stb)&&(wbu_zip_sel)),wbu_we, wbu_addr[0],
				wbu_data,
				zip_dbg_ack, zip_dbg_stall, zip_dbg_data
`ifdef	ZIP_SCOPE
			, zip_debug
`endif
			);
`else
	zipbones #(24'h2000,ZA,10,1)
		zippy(i_clk, 1'b0,
			// Zippys wishbone interface
			zip_cyc, zip_stb, zip_we, w_zip_addr, zip_data,
				zip_ack, zip_stall, dwb_idata, zip_err,
			w_interrupt, zip_cpu_int,
			// Debug wishbone interface
			((wbu_cyc)&&(wbu_zip_sel)),
				((wbu_stb)&&(wbu_zip_sel)),wbu_we, wbu_addr[0],
				wbu_data,
				zip_dbg_ack, zip_dbg_stall, zip_dbg_data
`ifdef	ZIP_SCOPE
				, zip_debug
`endif
		);
`endif
	generate
	if (ZA < 32)
		assign	zip_addr = { {(32-ZA){1'b0}}, w_zip_addr };
	else
		assign	zip_addr = w_zip_addr;
	endgenerate


	//
	//
	// And an arbiter to decide who gets to access the bus
	//
	//
	/*
	wbarbiter #(32,32) wbu_zip_arbiter(i_clk, i_rst,
		// The UART interface Master
		wbu_addr, wbu_data, wbu_we, (wbu_stb)&&(~wbu_zip_sel),
			(wbu_cyc)&&(~wbu_zip_sel), wbu_ack, wbu_stall, wbu_err,
		// The ZIP CPU Master
		zip_addr, zip_data, zip_we, zip_stb,
			zip_cyc, zip_ack, zip_stall, zip_err,
		// Common bus returns
		dwb_addr,dwb_odata,dwb_we,dwb_stb, dwb_cyc, dwb_ack, dwb_stall, dwb_err);
	*/
	wbpriarbiter #(32,32) wbu_zip_arbiter(i_clk,
		// The ZIP CPU Master -- gets priority in the arbiter
		zip_cyc, zip_stb, zip_we, zip_addr, zip_data,
			zip_ack, zip_stall, zip_err,
		// The JTAG interface Master, secondary priority,
		// will suffer a 1clk delay in arbitration
		(wbu_cyc)&&(~wbu_zip_sel), (wbu_stb)&&(~wbu_zip_sel), wbu_we,
			wbu_addr, wbu_data,
			wbu_ack, wbu_stall, wbu_err,
		// Common bus returns
		dwb_cyc, dwb_stb, dwb_we, dwb_addr, dwb_odata,
			dwb_ack, dwb_stall, dwb_err);

	// 
	// 
	// And because the ZIP CPU and the Arbiter create an unacceptable
	// delay, we fail timing.  So we add in a delay cycle ...
	// 
	// 
`ifdef	NO_ZIP_WBU_DELAY
	assign	wb_cyc    = dwb_cyc;
	assign	wb_stb    = dwb_stb;
	assign	wb_we     = dwb_we;
	assign	wb_addr   = dwb_addr;
	assign	wb_data   = dwb_odata;
	assign	dwb_idata = wb_idata;
	assign	dwb_ack   = wb_ack;
	assign	dwb_stall = wb_stall;
	assign	dwb_err   = wb_err;
`else
	busdelay	wbu_zip_delay(i_clk,
			dwb_cyc, dwb_stb, dwb_we, dwb_addr, dwb_odata,
				dwb_ack, dwb_stall, dwb_idata, dwb_err,
			wb_cyc, wb_stb, wb_we, wb_addr, wb_data,
				wb_ack, wb_stall, wb_idata, wb_err);
`endif


`else // if no ZIP_CPU
	assign	zip_cyc = 1'b0;
	assign	zip_stb = 1'b0;
	assign	zip_we  = 1'b0;
	assign	zip_cpu_int = 1'b0;
	assign	zip_addr = 32'h000;
	assign	zip_data = 32'h000;

	reg	r_zip_dbg_ack;
	initial	r_zip_dbg_ack = 1'b0;
	always @(posedge i_clk)
		r_zip_dbg_ack <= ((wbu_cyc)&&(wbu_zip_sel)&(wbu_stb));
	assign	zip_dbg_ack = r_zip_dbg_ack;
	assign	zip_dbg_stall = 1'b0;
	assign	zip_dbg_data = 32'h000;

	assign	dwb_addr = wbu_addr;
	assign	dwb_odata = wbu_data;
	assign	dwb_we = wbu_we;
	assign	dwb_stb = (wbu_stb);
	assign	dwb_cyc = (wbu_cyc);
	assign	wb_cyc  = dwb_cyc;
	assign	wb_stb  = dwb_stb;
	assign	wb_we   = dwb_we;
	assign	wb_addr = dwb_addr;
	assign	wb_data = dwb_odata;
	assign	wbu_ack = dwb_ack;
	assign	wbu_stall = dwb_stall;
	assign	dwb_idata = wb_idata;
	assign	wbu_err = dwb_err;
`endif



	wire	io_sel, pwm_sel, uart_sel, flash_sel, flctl_sel, scop_sel,
			cfg_sel, mem_sel, sdram_sel, sdcard_sel,
			none_sel, many_sel, io_bank;
	wire	io_ack, flash_ack, scop_ack, cfg_ack, mem_ack,
			sdram_ack, sdcard_ack, uart_ack, pwm_ack;
	wire	io_stall, flash_stall, scop_stall, cfg_stall, mem_stall,
			sdram_stall, sdcard_stall, uart_stall, pwm_stall;

	wire	[31:0]	io_data, flash_data, scop_data, cfg_data, mem_data,
			sdram_data, sdcard_data, uart_data, pwm_data;
	reg	[31:0]	bus_err_addr;

	assign	wb_ack = (wb_cyc)&&((io_ack)||(uart_ack)||(pwm_ack)
				||(scop_ack)||(cfg_ack)
				||(mem_ack)||(flash_ack)||(sdram_ack)
				||(sdcard_ack)
				||((none_sel)&&(1'b1)));
	assign	wb_stall = ((io_sel)&&(io_stall))
			||((uart_sel)&&(uart_stall))
			||((pwm_sel)&&(pwm_stall))
			||((scop_sel)&&(scop_stall))
			||((cfg_sel)&&(cfg_stall))
			||((mem_sel)&&(mem_stall))
			||((sdram_sel)&&(sdram_stall))
			||((sdcard_sel)&&(sdcard_stall))
			||((flash_sel||flctl_sel)&&(flash_stall));
			// (none_sel)&&(1'b0)

	/*
	assign	wb_idata = (io_ack)?io_data
			: ((scop_ack)?scop_data
			: ((cfg_ack)?cfg_data
			: ((mem_ack)?mem_data
			: ((flash_ack)?flash_data
			: 32'h00))));
	*/
	assign	wb_idata =  (io_ack|scop_ack)?((io_ack )? io_data  : scop_data)
			: ((uart_ack|pwm_ack)?((uart_ack)?uart_data: pwm_data)
			: ((cfg_ack) ? cfg_data
			: ((sdram_ack|sdcard_ack)
					?((sdram_ack)? sdram_data : sdcard_data)
			: ((mem_ack)?mem_data:flash_data)))); // if (flash_ack)
	assign	wb_err = ((wb_stb)&&(none_sel || many_sel))
				|| ((wb_cyc)&&(many_ack));

	// Addresses ...
	//	0000 xxxx	configuration/control registers
	//	001x xxxx	Down-sampler taps	(64 taps, 2 at a time)
	//	1xxx xxxx	Up-sampler taps
	//	1 xxxx xxxx xxxx xxxx xxxx	Up-sampler taps

`define	SPEEDY_IO
`ifndef	SPEEDY_IO

	wire	pre_io, pre_pwm, pre_uart, pre_flctl, pre_scop;
	assign	io_bank  = (wb_cyc)&&(wb_addr[31:5] == 27'h8);
	assign	pre_io   = (~pre_flctl)&&(~pre_pwm)&&(~pre_uart)&&(~pre_scop);
	assign	io_sel   = (io_bank)&&(pre_io);
	assign	pre_pwm  = (wb_addr[4: 1]== 4'h4);
	assign	pwm_sel  = (io_bank)&&(pre_pwm);
	assign	pre_uart = (wb_addr[4: 1]== 4'h5)||(wb_addr[4:0]==5'h7);
	assign	uart_sel = (io_bank)&&(pre_uart);
	assign	pre_flctl= (wb_addr[4: 2]== 3'h3);
	assign	flctl_sel= (io_bank)&&(pre_flctl);
	assign	pre_scop = (wb_addr[4: 3]== 2'h3);
	assign	scop_sel = (io_bank)&&(pre_scop);
	assign	cfg_sel  =((wb_cyc)&&(wb_addr[31: 6]== 26'h05));
	// zip_sel is not on the bus at this point
	assign	mem_sel  =((wb_cyc)&&(wb_addr[31:13]== 19'h01));
	assign	flash_sel=((wb_cyc)&&(wb_addr[31:18]== 14'h01));
`ifdef	SDCARD_ACCESS
	assign	sdcard_sel=((wb_cyc)&&(wb_addr[31:2]== 30'h48));
`else
	assign	sdcard_sel=1'b0;
`endif
	assign	sdram_sel=((wb_cyc)&&(wb_addr[31:23]== 9'h01));
`else
	wire	[3:0]	iovec;
	assign	iovec = { wb_addr[23],wb_addr[18],wb_addr[13],wb_addr[8] };

	assign	sdram_sel = (iovec[3]);
	assign	flash_sel = (iovec[3:2]==2'b01);
	assign	mem_sel   = (iovec[3:1]==3'b001);
	assign	io_bank   = (iovec[3:0]==4'b0001)&&(wb_addr[7:5]==3'b000);
	assign	cfg_sel   = (iovec[3:0]==4'b0001)&&(wb_addr[6]);
	assign	sdcard_sel= (iovec[3:0]==4'b0001)&&(wb_addr[6:5]==2'b01);
	assign	scop_sel  = (io_bank)&&(wb_addr[7:3]==5'b00011);
	assign	io_sel    = (io_bank)&&(wb_addr[7:5]==3'b000)
				&&(wb_addr[4:0] != 5'b00111) // Not UART Ctrl
				&&(wb_addr[3] != 1'b1);//Not PWM/UART/Flash/Scp
	assign	flctl_sel = (io_bank)&&(wb_addr[4:2]==3'b011);
	assign	pwm_sel   = (io_bank)&&(wb_addr[4:1]==4'b0100);
	// Note that in the following definition, the UART is given four words
	// despite the fact that it can probably only use 3.
	assign	uart_sel  = (io_bank)&&((wb_addr[4:1]==4'b0101)
					||(wb_addr[4:0]==5'b00111));

`endif

	assign	none_sel =((wb_stb)&&(~
			(io_sel
			||uart_sel
			||pwm_sel
			||flctl_sel
			||scop_sel
			||cfg_sel
			||mem_sel
			||sdram_sel
			||sdcard_sel
			||flash_sel)));
	assign	many_sel =((wb_stb)&&(
			 {3'h0, io_sel}
			+{3'h0, uart_sel}
			+{3'h0, pwm_sel}
			+{3'h0, flctl_sel}
			+{3'h0, scop_sel}
			+{3'h0, cfg_sel}
			+{3'h0, mem_sel}
			+{3'h0, sdram_sel}
			+{3'h0, sdcard_sel}
			+{3'h0, flash_sel} > 1));

	wire	many_ack;
	assign	many_ack =((wb_cyc)&&(
			 {3'h0, io_ack}
			+{3'h0, uart_ack}
			+{3'h0, pwm_ack}
			// FLCTL acks through the flash, so one less check here
			+{3'h0, scop_ack}
			+{3'h0, cfg_ack}
			+{3'h0, mem_ack}
			+{3'h0, sdram_ack}
			+{3'h0, sdcard_ack}
			+{3'h0, flash_ack} > 1));

	always @(posedge i_clk)
		if (wb_err)
			bus_err_addr <= wb_addr;

	wire		flash_interrupt, sdcard_interrupt, scop_interrupt,
			uart_rx_int, uart_tx_int, pwm_int;
	wire	[(NGPO-1):0]	w_gpio;
	// The I/O processor, herein called an ioslave
	ioslave	#(NGPO, NGPI) runio(i_clk,
			wb_cyc, (io_sel)&&(wb_stb), wb_we, wb_addr[4:0],
				wb_data, io_ack, io_stall, io_data,
			i_gpio, w_gpio,
			bus_err_addr,
			{
			sdcard_interrupt,
			uart_tx_int, uart_rx_int, pwm_int, scop_interrupt,
				flash_interrupt,
`ifdef	XULA25
				zip_cpu_int
`else
				1'b0
`endif
				},
			w_ints_to_zip_cpu,
			w_interrupt);
		// 8684
		// 1'bx, 4'h0, scop_sel, scop_ack, ~scop_stall, 
		//	wb_err, ~vga_interrupt, 2'b00, flash_interrupt
	//

	//
	//	UART device
	//
	wire	[31:0]	uart_debug;
	uartdev	serialport(i_clk, i_rx_uart, o_tx_uart,
			wb_cyc, (wb_stb)&&(uart_sel), wb_we,
					{ ~wb_addr[2], wb_addr[0]}, wb_data,
			uart_ack, uart_stall, uart_data,
			uart_rx_int, uart_tx_int,
			uart_debug);

	//
	//	PWM (audio) device
	//
	// The audio rate is given by the number of clock ticks between
	// samples.  If we are running at 80 MHz, then divide that by the
	// sample rate to get the first parameter for the PWM device.  The
	// second parameter is zero or one, indicating whether or not the
	// audio rate can be adjusted (1), or whether it is fixed within the
	// build (0).
`ifdef	XULA25
// `define	FMHACK

`ifdef	FMHACK
	wbfmtxhack	#(16'd1813)	// 44.1 kHz, user adjustable
`else
	wbpwmaudio	#(16'd1813,1,16)	// 44.1 kHz, user adjustable
`endif

`else
	wbpwmaudio	#(16'h270f,0,16) //  8   kHz, fixed audio rate
`endif
		pwmdev(i_clk,
			wb_cyc, (wb_stb)&&(pwm_sel), wb_we, wb_addr[0],
			wb_data, pwm_ack, pwm_stall, pwm_data, o_pwm, pwm_int);

`ifdef	FMHACK
	assign	o_gpio = {(NGPO){o_pwm}};
`else
	assign	o_gpio = w_gpio;
`endif

			

	//
	//	FLASH MEMORY CONFIGURATION ACCESS
	//
	wire	flash_cs_n, flash_sck, flash_mosi;
	wire	spi_user, sdcard_grant, flash_grant;
`ifdef	FLASH_ACCESS
	wbspiflash	flashmem(i_clk,
		wb_cyc,(wb_stb&&flash_sel),(wb_stb)&&(flctl_sel),wb_we,
			wb_addr[17:0], wb_data,
		flash_ack, flash_stall, flash_data,
		flash_sck, flash_cs_n, o_sf_cs_n, flash_mosi, i_spi_miso,
		flash_interrupt, flash_grant);
`else
	reg	r_flash_ack;
	initial	r_flash_ack = 1'b0;
	always @(posedge i_clk)
		r_flash_ack <= (wb_stb)&&((flash_sel)||(flctl_sel));

	assign	flash_ack = r_flash_ack;
	assign	flash_stall = 1'b0;
	assign	flash_data = 32'h0000;
	assign	flash_interrupt = 1'b0;

	assign	flash_cs_n = 1'b1;
	assign	flash_sck  = 1'b1;
	assign	flash_mosi = 1'b1;
`endif

	//
	//	SDCARD ACCESS
	//
	wire	sdcard_cs_n, sdcard_sck, sdcard_mosi;
	wire	[31:0]	sdspi_scope;
`ifdef	SDCARD_ACCESS
	sdspi	sdcard_controller(i_clk,
		// Wishbone interface
		wb_cyc, (wb_stb)&&(sdcard_sel), wb_we, wb_addr[1:0], wb_data,
		//	return
			sdcard_ack, sdcard_stall, sdcard_data,
		// SPI interface
		sdcard_cs_n, sdcard_sck, sdcard_mosi, i_spi_miso,
		sdcard_interrupt, sdcard_grant, sdspi_scope);
`else
	reg	r_sdcard_ack;
	initial	r_sdcard_ack = 1'b0;
	always @(posedge i_clk)
		r_sdcard_ack <= (wb_stb)&&(sdcard_sel);
	assign	sdcard_stall = 1'b0;
	assign	sdcard_ack = r_sdcard_ack;
	assign	sdcard_data = 32'h0000;
	assign	sdcard_interrupt= 1'b0;
`endif	// SDCARD_ACCESS


`ifdef	FLASH_ACCESS
`ifdef	SDCARD_ACCESS
	spiarbiter	spichk(i_clk,
		// Channel zero
		flash_cs_n, flash_sck, flash_mosi,
		// Channel one
		sdcard_cs_n, sdcard_sck, sdcard_mosi,
		o_sf_cs_n, o_sd_cs_n, o_spi_sck, o_spi_mosi,
		spi_user);
	assign	sdcard_grant =  spi_user;
	assign	flash_grant  = ~spi_user;
`else
	// Flash access, but no SD card access
	assign	o_sf_cs_n  = flash_cs_n;
	assign	o_sd_cs_n  = 1'b1;
	assign	o_spi_sck  = flash_sck;
	assign	o_spi_mosi = flash_mosi;
	assign	spi_user = 1'b0;
	assign	flash_grant = 1'b1;
	assign	sdcard_grant= 1'b0;
`endif // SDCARD_ACCESS && FLASH_ACCESS
`else // FLASH_ACCESS
`ifdef	SDCARD_ACCESS
	// SDCard access, but no flash access
	assign	o_sf_cs_n  = 1'b1;
	assign	o_sd_cs_n  = sdcard_cs_n;
	assign	o_spi_sck  = sdcard_sck;
	assign	o_spi_mosi = sdcard_mosi;
	assign	spi_user = 1'b1;
	assign	flash_grant = 1'b0;
	assign	sdcard_grant= 1'b1;
`else
	// No SPI access ...
	assign	o_sf_cs_n  = 1'b1;
	assign	o_sd_cs_n  = 1'b1;
	assign	o_spi_sck  = 1'b1;
	assign	o_spi_mosi = 1'b1;
	assign	spi_user = 1'b0;
	assign	flash_grant = 1'b0;
	assign	sdcard_grant= 1'b0;
`endif // SDCARD_ACCESS, w/o FLASH_ACCESS
`endif // !FLASH_ACCESS


	//
	//	MULTIBOOT/ICAPE2 CONFIGURATION ACCESS
	//
	wire	[31:0]	cfg_scope;
`ifdef	FANCY_ICAP_ACCESS
	wbicape6	fpga_cfg(i_clk, wb_cyc,(cfg_sel)&&(wb_stb), wb_we,
				wb_addr[5:0], wb_data,
				cfg_ack, cfg_stall, cfg_data,
				cfg_scope);
`else
	assign	cfg_scope = 32'h0000;
	reg	r_cfg_ack;
	initial	r_cfg_ack = 1'b0;
	always @(posedge i_clk)
		r_cfg_ack <= ((cfg_sel)&&(wb_stb)&&(~cfg_stall));
	assign	cfg_ack = r_cfg_ack;
	assign	cfg_stall = 1'b0;
	assign	cfg_data = 32'h0000;
`endif


	//
	//	RAM MEMORY ACCESS
	//
`ifdef	IMPLEMENT_ONCHIP_RAM
	memdev	#(13) ram(i_clk, wb_cyc, (wb_stb)&&(mem_sel), wb_we,
			wb_addr[12:0], wb_data, mem_ack, mem_stall, mem_data);
`else
	reg	r_mem_ack;
	always @(posedge i_clk)
		r_mem_ack = (wb_stb)&&(mem_sel);
	assign	mem_data = 32'h000;
	assign	mem_stall = 1'b0;
	assign	mem_ack = r_mem_ack;
`endif


	//
	//	SDRAM Memory Access
	//
	wire	[31:0]	sdram_debug;
`ifndef	BYPASS_SDRAM_ACCESS
	wbsdram	sdram(i_clk, 
		wb_cyc, (wb_stb)&&(sdram_sel),
			wb_we, wb_addr[22:0], wb_data,
			sdram_ack, sdram_stall, sdram_data,
		o_ram_cs_n, o_ram_cke, o_ram_ras_n, o_ram_cas_n, o_ram_we_n,
			o_ram_bs, o_ram_addr,
			o_ram_drive_data, i_ram_data, o_ram_data, o_ram_dqm,
		sdram_debug);
`else
	reg	r_sdram_ack;
	initial	r_sdram_ack = 1'b0;
	always @(posedge i_clk)
		r_sdram_ack <= (wb_stb)&&(sdram_sel);
	assign	sdram_ack = r_sdram_ack;
	assign	sdram_stall = 1'b0;
	assign	sdram_data = 32'h0000;

	assign	o_ram_ce_n  = 1'b1;
	assign	o_ram_ras_n = 1'b1;
	assign	o_ram_cas_n = 1'b1;
	assign	o_ram_we_n  = 1'b1;

	assign	sdram_debug = 32'h0000;
`endif

	//
	//
	//	WISHBONE SCOPES
	//
	//
	//
	//
	wire	[31:0]	scop_flash_data;
	wire	scop_flash_ack, scop_flash_stall, scop_flash_interrupt;

`ifndef	FLASH_ACCESS
`ifdef	FLASH_SCOPE
`undef	FLASH_SCOPE // FLASH_SCOPE only makes sense if you have flash access
`endif
`endif

`ifdef	FLASH_SCOPE
	reg	[31:0]	r_flash_debug, last_flash_debug;
	always @(posedge i_clk)
		r_flash_debug <= flash_debug;
	always @(posedge i_clk)
		last_flash_debug <= r_flash_debug;
	wbscope spiscope(i_clk, 1'b1, (~o_spi_cs_n), r_flash_debug,
		// Wishbone interface
		i_clk, wb_cyc, ((wb_stb)&&(scop_sel)&&(wb_addr[2:1]==2'b00)), wb_we, wb_addr[0],
			wb_data,
			scop_flash_ack, scop_flash_stall, scop_flash_data,
		scop_flash_interrupt);
`else
`ifdef	WBUS_SCOPE
	wbscopc #(5'ha) wbuscope(i_clk, 1'b1, wbus_debug[31], wbus_debug[30:0],
		// Wishbone interface
		i_clk, wb_cyc, ((wb_stb)&&(scop_sel)&&(wb_addr[2:1]==2'b00)), wb_we, wb_addr[0],
			wb_data,
			scop_flash_ack, scop_flash_stall, scop_flash_data,
		scop_flash_interrupt);
`else
	assign	scop_flash_data = 32'h00;
	assign	scop_flash_ack  = (wb_stb)&&(scop_sel)&&(wb_addr[2:1]==2'b00);
	assign scop_flash_stall = 1'b0;
	assign scop_flash_interrupt = 1'b0;
`endif
`endif


	wire	[31:0]	scop_cfg_data;
	wire		scop_cfg_ack, scop_cfg_stall, scop_cfg_interrupt;
`ifdef	CFG_SCOPE
	wire		scop_cfg_trigger;
	assign	scop_cfg_trigger = (wb_stb)&&(cfg_sel);
	wbscope	#(5'h7) wbcfgscope(i_clk, 1'b1, scop_cfg_trigger, cfg_scope,
		// Wishbone interface
		i_clk, wb_cyc, ((wb_stb)&&(scop_sel)&&(wb_addr[2:1]==2'b01)),
				wb_we, wb_addr[0], wb_data,
			scop_cfg_ack, scop_cfg_stall, scop_cfg_data,
		scop_cfg_interrupt);
`else
`ifdef	SDCARD_SCOPE
	wire		scop_sd_trigger, scop_sd_ce;
	assign	scop_sd_trigger = (wb_stb)&&(sdcard_sel)&&(wb_we);
	assign	scop_sd_ce = 1'b1; // sdspi_scope[31];
	wbscope #(5'h9) sdspiscope(i_clk, scop_sd_ce,
			scop_sd_trigger, sdspi_scope,
		// Wishbone interface
		i_clk, wb_cyc, ((wb_stb)&&(scop_sel)&&(wb_addr[2:1]==2'b01)),
			wb_we, wb_addr[0], wb_data,
		scop_cfg_ack, scop_cfg_stall, scop_cfg_data,scop_cfg_interrupt);
`else
	assign	scop_cfg_data = 32'h00;
	assign	scop_cfg_ack  = (wb_stb)&&(scop_sel)&&(wb_addr[2:1]==2'b01);
	assign	scop_cfg_stall = 1'b0;
	assign	scop_cfg_interrupt = 1'b0;
`endif
`endif

	wire	[31:0]	scop_two_data;
	wire		scop_two_ack, scop_two_stall, scop_two_interrupt;
`ifdef	SDRAM_SCOPE
	wire		sdram_trigger;
	assign	sdram_trigger = sdram_debug[18]; // sdram_sel;
	
	wbscope	#(5'hb) sdramscope(i_clk, 1'b1, sdram_trigger,
			sdram_debug,
			//{ sdram_trigger, wb_data[30:0] },
		// Wishbone interface
		i_clk, wb_cyc, ((wb_stb)&&(scop_sel)&&(wb_addr[2:1]==2'b10)), wb_we, wb_addr[0],
			wb_data,
			scop_two_ack, scop_two_stall, scop_two_data,
		scop_two_interrupt);
`else
`ifdef	UART_SCOPE
	wire		uart_trigger;
	assign	uart_trigger = uart_debug[31];
	
	// wbscopc #(5'ha) uartscope(i_clk,1'b1, uart_trigger, uart_debug[30:0],
	wbscope	#(5'ha) uartscope(i_clk, 1'b1, uart_trigger, uart_debug[31:0],
		// Wishbone interface
		i_clk, wb_cyc, ((wb_stb)&&(scop_sel)&&(wb_addr[2:1]==2'b10)), wb_we, wb_addr[0],
			wb_data,
			scop_two_ack, scop_two_stall, scop_two_data,
		scop_two_interrupt);
`else
	assign	scop_two_data = 32'h00;
	assign	scop_two_ack  = (wb_stb)&&(scop_sel)&&(wb_addr[2:1]==2'b10);
	assign	scop_two_stall = 1'b0;
	assign	scop_two_interrupt = 1'b0;
`endif
`endif

	wire	[31:0]	scop_zip_data;
	wire		scop_zip_ack, scop_zip_stall, scop_zip_interrupt;
`ifdef	ZIP_SCOPE
	reg		zip_trigger, pre_trigger_a, pre_trigger_b;
	always @(posedge i_clk)
	begin
		pre_trigger_a <= (wb_stb)&&(wb_addr[31:0]==32'h010b);
		pre_trigger_b <= (|wb_data[31:8]);
		zip_trigger= (pre_trigger_a)&&(pre_trigger_b)||(zip_debug[31]);
	end
	wbscope	#(5'h9) zipscope(i_clk, 1'b1, zip_trigger,
			zip_debug,
		// Wishbone interface
		i_clk, wb_cyc, ((wb_stb)&&(scop_sel)&&(wb_addr[2:1]==2'b11)), wb_we, wb_addr[0],
			wb_data,
			scop_zip_ack, scop_zip_stall, scop_zip_data,
		scop_zip_interrupt);
`else
	assign	scop_zip_data = 32'h00;
	assign	scop_zip_ack  = (wb_stb)&&(scop_sel)&&(wb_addr[2:1]==2'b11);
	assign	scop_zip_stall = 1'b0;
	assign	scop_zip_interrupt = 1'b0;
`endif


	assign	scop_interrupt = scop_flash_interrupt || scop_cfg_interrupt
				|| scop_two_interrupt || scop_zip_interrupt;
	assign	scop_ack   = scop_cfg_ack | scop_flash_ack | scop_two_ack | scop_zip_ack;
	assign	scop_stall = ((~wb_addr[2])?
				((wb_addr[1])?scop_flash_stall:scop_cfg_stall)
				: ((wb_addr[1])?scop_two_stall:scop_zip_stall));
	assign	scop_data  = ((scop_cfg_ack)?scop_cfg_data
				: ((scop_flash_ack) ? scop_flash_data
				: ((scop_two_ack) ? scop_two_data
				: scop_zip_data)));


endmodule

// 0x8684 interrupts ...???
