#define	XULA25
////////////////////////////////////////////////////////////////////////////////
//
// Filename:	busmaster_tb.cpp
//
// Project:	FPGA library development (XuLA2 development board)
//
// Purpose:	This is piped version of the testbench for the busmaster
//		verilog code.  The busmaster code is designed to be a complete
//	code set implementing all of the functionality of the XESS XuLA2
//	development board.  If done well, the programs talking to this one
//	should be able to talk to the board and apply the same tests to the
//	board itself.
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
#include <signal.h>
#include <time.h>
#include <ctype.h>

#include "verilated.h"
#include "Vbusmaster.h"

#include "testb.h"
// #include "twoc.h"
#include "pipecmdr.h"
#include "qspiflashsim.h"
#include "sdramsim.h"
#include "sdspisim.h"
#include "uartsim.h"

#include "port.h"

// Add a reset line, since Vbusmaster doesn't have one
class	Vbusmasterr : public Vbusmaster {
public:
	int	i_rst;
	virtual	~Vbusmasterr() {}
};

// No particular "parameters" need definition or redefinition here.
class	BUSMASTER_TB : public PIPECMDR<Vbusmasterr> {
public:
	unsigned long	m_tx_busy_count;
	QSPIFLASHSIM	m_flash;
	SDSPISIM	m_sdcard;
	SDRAMSIM	m_sdram;
	unsigned	m_last_led, m_last_pic, m_last_tx_state;
	time_t		m_start_time;
	bool		m_last_writeout;
	UARTSIM		m_uart;
	int		m_last_bus_owner, m_busy;

	BUSMASTER_TB(void) : PIPECMDR(FPGAPORT), m_uart(FPGAPORT+1) {
		m_start_time = time(NULL);
		m_last_pic = 0;
		m_last_tx_state = 0;
	}

	void	reset(void) {
		m_core->i_clk = 1;
		m_core->eval();
	}

	void	setsdcard(const char *fn) {
		m_sdcard.load(fn);
	
		printf("LOADING SDCARD FROM: \'%s\'\n", fn);
	}

	void	tick(void) {
		int	flash_miso, sdcard_miso;

		if ((m_tickcount & ((1<<28)-1))==0) {
			double	ticks_per_second = m_tickcount;
			time_t	seconds_passed = time(NULL)-m_start_time;
			if (seconds_passed != 0) {
			ticks_per_second /= (double)(time(NULL) - m_start_time);
			printf(" ********   %.6f TICKS PER SECOND\n", 
				ticks_per_second);
			}
		}

		// Set up the bus before any clock tick
		m_core->i_clk = 1;
		flash_miso = (m_flash(m_core->o_sf_cs_n,
					m_core->o_spi_sck,
					m_core->o_spi_mosi)&0x02)?1:0;
#ifdef	XULA25
		sdcard_miso = m_sdcard(m_core->o_sd_cs_n, m_core->o_spi_sck,
					m_core->o_spi_mosi);
#else
		sdcard_miso = 1;
#endif

		if ((m_core->o_sf_cs_n)&&(m_core->o_sd_cs_n))
			m_core->i_spi_miso = 1;
		else if ((!m_core->o_sf_cs_n)&&(m_core->o_sd_cs_n))
			m_core->i_spi_miso = flash_miso;
		else if ((m_core->o_sf_cs_n)&&(!m_core->o_sd_cs_n))
			m_core->i_spi_miso = sdcard_miso;
		else
			assert((m_core->o_sf_cs_n)||(m_core->o_sd_cs_n));

		m_core->i_ram_data = m_sdram(1,
				m_core->o_ram_cke, m_core->o_ram_cs_n,
				m_core->o_ram_ras_n, m_core->o_ram_cas_n,
				m_core->o_ram_we_n, m_core->o_ram_bs,
				m_core->o_ram_addr, m_core->o_ram_drive_data,
				m_core->o_ram_data);

		m_core->i_rx_uart = m_uart(m_core->o_tx_uart,
				m_core->v__DOT__serialport__DOT__r_setup);
		PIPECMDR::tick();

// #define	DEBUGGING_OUTPUT
#ifdef	DEBUGGING_OUTPUT
		bool	writeout = false;
		/*
		if (m_core->v__DOT__sdram__DOT__r_pending)
			writeout = true;
		else if (m_core->v__DOT__sdram__DOT__bank_active[0])
			writeout = true;
		else if (m_core->v__DOT__sdram__DOT__bank_active[1])
			writeout = true;
		else if (m_core->v__DOT__sdram__DOT__bank_active[2])
			writeout = true;
		else if (m_core->v__DOT__sdram__DOT__bank_active[3])
			writeout = true;
		*/

		// if ((m_core->v__DOT__wbu_cyc)&&(!m_core->v__DOT__wbu_we))
			// writeout = true;
		/*
		if ((m_core->v__DOT__wbu_cyc)&&(!m_core->v__DOT__wbu_we))
			writeout = true;
		if (m_core->v__DOT__genbus__DOT__exec_stb)
			writeout = true;
		*/

		// if ((m_core->v__DOT__zippy__DOT__thecpu__DOT__instruction_decoder__DOT__genblk3__DOT__r_early_branch)
			// &&(m_core->v__DOT__zippy__DOT__thecpu__DOT__instruction == 0x7883ffff))
			// m_busy+=2;
		// else if (m_busy > 0) m_busy--;
#define	v__DOT__wb_addr		v__DOT__dwb_addr
#define	v__DOT__dwb_stall	v__DOT__wb_stall
#define	v__DOT__dwb_ack		v__DOT__wb_ack
#define	v__DOT__wb_cyc		v__DOT__dwb_cyc
#define	v__DOT__wb_stb		v__DOT__dwb_stb
#define	v__DOT__wb_we		v__DOT__dwb_we
#define	v__DOT__dwb_idata	v__DOT__wb_idata
#define	v__DOT__wb_data		v__DOT__dwb_odata

		if ((!m_core->v__DOT__zippy__DOT__cmd_halt)
				&&(!m_core->v__DOT__zippy__DOT__thecpu__DOT__sleep))
			writeout = true;
		// if (m_core->v__DOT__uart_tx_int)
			// writeout = true;
#ifdef	XULA25
		if (m_core->v__DOT__zippy__DOT__genblk10__DOT__pic__DOT__r_any)
			writeout = true;
#endif

#ifdef	XULA25
		unsigned this_pic = ((m_core->v__DOT__zippy__DOT__genblk10__DOT__pic__DOT__r_int_enable)<<16) | 
				(m_core->v__DOT__zippy__DOT__genblk10__DOT__pic__DOT__r_int_state);
#else
		unsigned this_pic = 0;
#endif

		// if (this_pic != m_last_pic)
			// writeout = true;
		unsigned tx_state = ((m_core->v__DOT__serialport__DOT__txmod__DOT__zero_baud_counter)<<20)
			|((m_core->v__DOT__serialport__DOT__txmod__DOT__r_busy)<<16)
			|((m_core->v__DOT__serialport__DOT__txmod__DOT__lcl_data)<<8)
			|((m_core->v__DOT__serialport__DOT__txmod__DOT__baud_counter&0x0f)<<4)
			|(m_core->v__DOT__serialport__DOT__txmod__DOT__state);
		/*
		if (tx_state != m_last_tx_state)
			writeout = true;
		*/
		int bus_owner = m_core->v__DOT__wbu_zip_arbiter__DOT__r_a_owner;
		bus_owner |= (m_core->v__DOT__wbu_cyc)?2:0;
		bus_owner |= (m_core->v__DOT__dwb_cyc)?4:0;
		bus_owner |= (m_core->v__DOT__wb_cyc)?8:0;
		bus_owner |= (m_core->v__DOT__wb_cyc)?16:0;
		bus_owner |= (m_core->v__DOT__wbu_stb)?32:0;
		bus_owner |= (m_core->v__DOT__zippy__DOT__thecpu__DOT__mem_stb_gbl)?64:0;
		bus_owner |= (m_core->v__DOT__wb_stb)?128:0;
		bus_owner |= (m_core->v__DOT____Vcellinp__wbu_zip_arbiter____pinNumber10)?256:0;
#ifdef	XULA25
		bus_owner |= (m_core->v__DOT__zippy__DOT__ext_cyc)?512:0;
#endif
		/*
		if (bus_owner != m_last_bus_owner)
			writeout = true;
		*/
		/*
		writeout = (writeout)||(m_core->i_rx_stb)
				||((m_core->o_tx_stb)&&(!m_core->i_tx_busy));
		writeout = (writeout)||(m_core->v__DOT____Vcellinp__genbus____pinNumber9);
		writeout = (writeout)||(m_core->v__DOT__wb_stb);
		writeout = (writeout)||(m_core->v__DOT__wb_err);
		*/

		if ((writeout)||(m_last_writeout)) {
			m_last_bus_owner = bus_owner;
			m_last_pic = this_pic;
			m_last_tx_state = tx_state;
			printf("%08lx:", m_tickcount);

			/*
			printf("%d/%02x %d/%02x%s ",
				m_core->i_rx_stb, m_core->i_rx_data,
				m_core->o_tx_stb, m_core->o_tx_data,
				m_core->i_tx_busy?"/BSY":"    ");
			*/

			printf("(%d/%d,%d/%d->%d),(%c:%d,%d->%d)|%c[%08x/%08x]@%08x %c%c%c",
				m_core->v__DOT__wbu_cyc,
				m_core->v__DOT____Vcellinp__wbu_zip_arbiter____pinNumber10,
				m_core->v__DOT__dwb_cyc, // was zip_cyc
#ifdef	XULA25
				(m_core->v__DOT__zippy__DOT__ext_cyc),
#else
				0,
#endif
				m_core->v__DOT__wb_cyc,
				//
				m_core->v__DOT__wbu_zip_arbiter__DOT__r_a_owner?'Z':'j',
				m_core->v__DOT__wbu_stb,
				// 0, // m_core->v__DOT__dwb_stb, // was zip_stb
				m_core->v__DOT__zippy__DOT__thecpu__DOT__mem_stb_gbl,
				m_core->v__DOT__wb_stb,
				//
				(m_core->v__DOT__wb_we)?'W':'R',
				m_core->v__DOT__wb_data,
					m_core->v__DOT__dwb_idata,
				m_core->v__DOT__wb_addr,
				(m_core->v__DOT__dwb_ack)?'A':
					(m_core->v__DOT____Vcellinp__genbus____pinNumber9)?'a':' ',
				(m_core->v__DOT__dwb_stall)?'S':
					(m_core->v__DOT____Vcellinp__genbus____pinNumber10)?'s':' ',
				(m_core->v__DOT__wb_err)?'E':'.');

			/*
			// UART-Wishbone bus converter debug lines
			printf(" RUNWB %d:%09lx %d@0x%08x %3x %3x %d %d/%d/%d %d:%09lx", 
				m_core->v__DOT__genbus__DOT__fifo_in_stb,
				m_core->v__DOT__genbus__DOT__fifo_in_word,
				m_core->v__DOT__genbus__DOT__runwb__DOT__wb_state,
				m_core->v__DOT__wbu_addr,
				m_core->v__DOT__genbus__DOT__runwb__DOT__r_len,
				m_core->v__DOT__genbus__DOT__runwb__DOT__r_acks_needed,
				m_core->v__DOT__genbus__DOT__runwb__DOT__w_eow,
				m_core->v__DOT__genbus__DOT__runwb__DOT__last_read_request,
				m_core->v__DOT__genbus__DOT__runwb__DOT__last_ack,
				m_core->v__DOT__genbus__DOT__runwb__DOT__zero_acks,
				m_core->v__DOT__genbus__DOT__exec_stb,
				m_core->v__DOT__genbus__DOT__exec_word);
			*/

			/*
			// UART-Wishbone bus converter debug lines
			printf(" WBU[%d,%3d,%3d]",
				m_core->v__DOT__genbus__DOT__runwb__DOT__wb_state,
				m_core->v__DOT__genbus__DOT__runwb__DOT__r_len,
				m_core->v__DOT__genbus__DOT__runwb__DOT__r_acks_needed);
			*/

			/*
			// SDRAM debug lines
			printf("%c[%d%d%d%d,%d:%04x%c]@%06x(%d) ->%06x%c",
				(m_core->v__DOT__sdram_sel)?'!':' ',
				m_core->o_ram_cs_n, m_core->o_ram_ras_n,
				m_core->o_ram_cas_n, m_core->o_ram_we_n,
				m_core->o_ram_bs, m_core->o_ram_data,
				(m_core->o_ram_drive_data)?'D':'-',
				m_core->o_ram_addr,
					(m_core->o_ram_addr>>10)&1,
				m_core->i_ram_data,
				(m_core->o_ram_drive_data)?'-':'V');

			printf(" SD[%d,%d-%3x%d]",
				m_core->v__DOT__sdram__DOT__r_state,
				m_sdram.pwrup(),
				m_core->v__DOT__sdram__DOT__refresh_clk,
				m_core->v__DOT__sdram__DOT__need_refresh);

			printf(" BNK[%d:%6x,%d:%6x,%d:%6x,%d:%6x],%x%d",
				m_core->v__DOT__sdram__DOT__bank_active[0],
				m_core->v__DOT__sdram__DOT__bank_row[0],
				m_core->v__DOT__sdram__DOT__bank_active[1],
				m_core->v__DOT__sdram__DOT__bank_row[1],
				m_core->v__DOT__sdram__DOT__bank_active[2],
				m_core->v__DOT__sdram__DOT__bank_row[2],
				m_core->v__DOT__sdram__DOT__bank_active[3],
				m_core->v__DOT__sdram__DOT__bank_row[3],
				m_core->v__DOT__sdram__DOT__clocks_til_idle,
				m_core->v__DOT__sdram__DOT__r_barrell_ack);

			printf(" %s%s%c[%08x@%06x]",
				(m_core->v__DOT__sdram__DOT__bus_cyc)?"C":" ",
				(m_core->v__DOT__sdram__DOT__r_pending)?"PND":"   ",
				(m_core->v__DOT__sdram__DOT__r_we)?'W':'R',
				(m_core->v__DOT__sdram__DOT__r_we)
				?(m_core->v__DOT__sdram__DOT__r_data)
				:(m_core->v__DOT__sdram_data),
				(m_core->v__DOT__sdram__DOT__r_addr));
			*/

			// CPU Pipeline debugging
			printf("%s%s%s%s%s%s%s%s%s%s%s",
				// (m_core->v__DOT__zippy__DOT__dbg_ack)?"A":"-",
				// (m_core->v__DOT__zippy__DOT__dbg_stall)?"S":"-",
				// (m_core->v__DOT__zippy__DOT__sys_dbg_cyc)?"D":"-",
				(m_core->v__DOT__zippy__DOT__cpu_lcl_cyc)?"L":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__r_halted)?"Z":"-",
				(m_core->v__DOT__zippy__DOT__cpu_break)?"!":"-",
				(m_core->v__DOT__zippy__DOT__cmd_halt)?"H":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__gie)?"G":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__pf_cyc)?"P":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__pf_valid)?"V":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__pf_illegal)?"i":" ",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__new_pc)?"N":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__domem__DOT__r_wb_cyc_gbl)?"G":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__domem__DOT__r_wb_cyc_lcl)?"L":"-");
			printf("|%s%s%s%s%s%s",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__r_dcdvalid)?"D":"-",
				(dcd_ce())?"d":"-",
				"x", // (m_core->v__DOT__zippy__DOT__thecpu__DOT__dcdA_stall)?"A":"-",
				"x", // (m_core->v__DOT__zippy__DOT__thecpu__DOT__dcdB_stall)?"B":"-",
				"x", // (m_core->v__DOT__zippy__DOT__thecpu__DOT__dcdF_stall)?"F":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__dcd_illegal)?"i":"-");
			
			printf("|%s%s%s%s%s%s%s%s%s%s",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__opvalid)?"O":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__op_ce)?"k":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__op_stall)?"s":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__op_illegal)?"i":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__r_op_break)?"B":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__genblk5__DOT__r_op_lock)?"L":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__r_op_pipe)?"P":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__r_break_pending)?"p":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__r_op_gie)?"G":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__opvalid_alu)?"A":"-");
			printf("|%s%s%s%s%s",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__alu_ce)?"a":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__alu_stall)?"s":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__doalu__DOT__r_busy)?"B":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__r_alu_gie)?"G":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__r_alu_illegal)?"i":"-");
			printf("|%s%s%s%2x %s%s%s %2d %2d",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__opvalid_mem)?"M":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__mem_ce)?"m":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__adf_ce_unconditional)?"!":"-",
				(m_core->v__DOT__zippy__DOT__cmd_addr),
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__bus_err)?"BE":"  ",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__ibus_err_flag)?"IB":"  ",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__ubus_err_flag)?"UB":"  ",
				m_core->v__DOT__zippy__DOT__thecpu__DOT__domem__DOT__rdaddr,
				m_core->v__DOT__zippy__DOT__thecpu__DOT__domem__DOT__wraddr);
#ifdef	XULA25
			printf("|%s%s",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__div_busy)?"D":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__div_error)?"E":"-");
#endif
			printf("|%s%s[%2x]%08x",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__wr_reg_ce)?"W":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__wr_flags_ce)?"F":"-",
				m_core->v__DOT__zippy__DOT__thecpu__DOT__wr_reg_id,
				m_core->v__DOT__zippy__DOT__thecpu__DOT__wr_gpreg_vl);

			// Program counter debugging
			printf(" PC0x%08x/%08x/%08x-%08x %s0x%08x", 
				m_core->v__DOT__zippy__DOT__thecpu__DOT__pf_pc,
				m_core->v__DOT__zippy__DOT__thecpu__DOT__ipc,
				m_core->v__DOT__zippy__DOT__thecpu__DOT__upc,
				m_core->v__DOT__zippy__DOT__thecpu__DOT__instruction,
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__instruction_decoder__DOT__genblk3__DOT__r_early_branch)?"EB":"  ",
				m_core->v__DOT__zippy__DOT__thecpu__DOT__instruction_decoder__DOT__genblk3__DOT__r_branch_pc
				);
			// More in-depth
			printf("[%c%08x,%c%08x,%c%08x]",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__r_dcdvalid)?'D':'-',
				m_core->v__DOT__zippy__DOT__thecpu__DOT__dcd_pc,
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__opvalid)?'O':'-',
				m_core->v__DOT__zippy__DOT__thecpu__DOT__op_pc,
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__alu_valid)?'A':'-',
				m_core->v__DOT__zippy__DOT__thecpu__DOT__r_alu_pc);
			
			/*	
			// Prefetch debugging
			printf(" [PC%08x,LST%08x]->[%d%s%s](%d,%08x/%08x)->%08x@%08x",
				m_core->v__DOT__zippy__DOT__thecpu__DOT__pf_pc,
				m_core->v__DOT__zippy__DOT__thecpu__DOT__pf__DOT__lastpc,
				m_core->v__DOT__zippy__DOT__thecpu__DOT__pf__DOT__rvsrc,
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__pf__DOT__rvsrc)
				?((m_core->v__DOT__zippy__DOT__thecpu__DOT__pf__DOT__r_v_from_pc)?"P":" ")
				:((m_core->v__DOT__zippy__DOT__thecpu__DOT__pf__DOT__r_v_from_pc)?"p":" "),
				(!m_core->v__DOT__zippy__DOT__thecpu__DOT__pf__DOT__rvsrc)
				?((m_core->v__DOT__zippy__DOT__thecpu__DOT__pf__DOT__r_v_from_last)?"l":" ")
				:((m_core->v__DOT__zippy__DOT__thecpu__DOT__pf__DOT__r_v_from_last)?"L":" "),
				m_core->v__DOT__zippy__DOT__thecpu__DOT__pf__DOT__isrc,
				m_core->v__DOT__zippy__DOT__thecpu__DOT__pf__DOT__r_pc_cache,
				m_core->v__DOT__zippy__DOT__thecpu__DOT__pf__DOT__r_last_cache,
				m_core->v__DOT__zippy__DOT__thecpu__DOT__instruction,
				m_core->v__DOT__zippy__DOT__thecpu__DOT__instruction_pc);
			*/

			// Decode Stage debugging
			// (nothing)

			// Op Stage debugging
//			printf(" Op(%02x,%02x->%02x)",
//				m_core->v__DOT__zippy__DOT__thecpu__DOT__dcdOp,
//				m_core->v__DOT__zippy__DOT__thecpu__DOT__opn,
//				m_core->v__DOT__zippy__DOT__thecpu__DOT__opR);

			printf(" %s[%02x]=%08x(%08x)",
				m_core->v__DOT__zippy__DOT__thecpu__DOT__wr_reg_ce?"WR":"--",
				m_core->v__DOT__zippy__DOT__thecpu__DOT__wr_reg_id,
				m_core->v__DOT__zippy__DOT__thecpu__DOT__wr_gpreg_vl,
#ifdef	XULA25
				m_core->v__DOT__zippy__DOT__thecpu__DOT__wr_spreg_vl
#else
				0
#endif
				);
			printf(" Rid=(%d|%d)?%02x:%02x",
				m_core->v__DOT__zippy__DOT__thecpu__DOT__alu_wr,
#ifdef	XULA25
				m_core->v__DOT__zippy__DOT__thecpu__DOT__div_valid,
#else
				0,
#endif
				m_core->v__DOT__zippy__DOT__thecpu__DOT__alu_reg,
				m_core->v__DOT__zippy__DOT__thecpu__DOT__mem_wreg);

			// domem, the pipelined memory unit debugging
/*
			printf(" M[%s@0x%08x]",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__opvalid_mem)
				?((m_core->v__DOT__zippy__DOT__thecpu__DOT__opn&1)?"W":"R")
				:"-",
#ifdef	XULA25
				m_core->v__DOT__zippy__DOT__cpu_addr
#else
				0
#endif
				);
*/

/*
			printf("%s%s",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__domem__DOT__cyc)?"B":"-",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__mem_rdbusy)?"r":"-");
*/
#ifdef	XULA25
			printf(" %s-%s %04x/%04x",
				(m_core->v__DOT__zippy__DOT__genblk10__DOT__pic__DOT__r_any)?"PIC":"pic",
				(m_core->v__DOT__zippy__DOT__genblk10__DOT__pic__DOT__r_gie)?"INT":"( )",
				m_core->v__DOT__zippy__DOT__genblk10__DOT__pic__DOT__r_int_enable,
				m_core->v__DOT__zippy__DOT__genblk10__DOT__pic__DOT__r_int_state);
#else
			printf(" %s-%s %04x/%04x",
				(m_core->v__DOT__runio__DOT__intcontroller__DOT__r_any)?"PIC":"pic",
				(m_core->v__DOT__runio__DOT__intcontroller__DOT__r_gie)?"INT":"( )",
				m_core->v__DOT__runio__DOT__intcontroller__DOT__r_int_enable,
				m_core->v__DOT__runio__DOT__intcontroller__DOT__r_int_state);
#endif
	

		/*
			printf(" %s",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__cc_invalid_for_dcd)?"CCI":"   ");
		*/

			/*
			// Illegal instruction debugging
			printf(" ILL[%s%s%s%s%s%s]",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__pf_err)?"WB":"  ",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__pf_illegal)?"PF":"  ",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__dcd_illegal)?"DCD":"   ",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__op_illegal)?"OP":"  ",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__r_alu_illegal)?"ALU":"   ",
				(m_core->v__DOT__zippy__DOT__thecpu__DOT__ill_err_u)?"ILL":"   ");

			*/

			/*
			printf(" UART%08x/%d-%08x", m_last_tx_state,
				m_core->v__DOT__serialport__DOT__txmod__DOT__zero_baud_counter,
				m_core->v__DOT__serialport__DOT__txmod__DOT__baud_counter);
			*/

			// Debug some conditions
			if (m_core->v__DOT__zippy__DOT__thecpu__DOT__ubreak)
				printf(" BREAK");
			// if (m_core->v__DOT__zippy__DOT__thecpu__DOT__w_switch_to_interrupt)
				// printf(" TO-INT");
#ifdef	XULA25
			if (m_core->v__DOT__zippy__DOT__genblk10__DOT__pic__DOT__r_interrupt)
				printf(" INTERRUPT");
#endif

			/*
			printf(" SDSPI[%d,%d(%d),(%d)]",
				m_core->v__DOT__sdcard_controller__DOT__r_cmd_busy,
				m_core->v__DOT__sdcard_controller__DOT__r_sdspi_clk,
				m_core->v__DOT__sdcard_controller__DOT__r_cmd_state,
				m_core->v__DOT__sdcard_controller__DOT__r_rsp_state);
			printf(" LL[%d,%2x->CK=%d/%2x,%s,ST=%2d,TX=%2x,RX=%2x->%d,%2x] ",
				m_core->v__DOT__sdcard_controller__DOT__ll_cmd_stb,
				m_core->v__DOT__sdcard_controller__DOT__ll_cmd_dat,
				m_core->v__DOT__sdcard_controller__DOT__lowlevel__DOT__r_z_counter,
				// (m_core->v__DOT__sdcard_controller__DOT__lowlevel__DOT__r_clk_counter==0)?1:0,
				m_core->v__DOT__sdcard_controller__DOT__lowlevel__DOT__r_clk_counter,
				(m_core->v__DOT__sdcard_controller__DOT__lowlevel__DOT__r_idle)?"IDLE":"    ",
				m_core->v__DOT__sdcard_controller__DOT__lowlevel__DOT__r_state,
				m_core->v__DOT__sdcard_controller__DOT__lowlevel__DOT__r_byte,
				m_core->v__DOT__sdcard_controller__DOT__lowlevel__DOT__r_ireg,
				m_core->v__DOT__sdcard_controller__DOT__ll_out_stb,
				m_core->v__DOT__sdcard_controller__DOT__ll_out_dat
				);
			printf(" CRC=%02x/%2d",
				m_core->v__DOT__sdcard_controller__DOT__r_cmd_crc,
				m_core->v__DOT__sdcard_controller__DOT__r_cmd_crc_cnt);
			printf(" SPI(%d,%d,%d/%d,%d)->?",
				m_core->o_sf_cs_n,
				m_core->o_sd_cs_n,
				m_core->o_spi_sck, m_core->v__DOT__sdcard_sck,
				m_core->o_spi_mosi);

			printf(" CK=%d,LN=%d",
				m_core->v__DOT__sdcard_controller__DOT__r_sdspi_clk,
				m_core->v__DOT__sdcard_controller__DOT__r_lgblklen);


			if (m_core->v__DOT__sdcard_controller__DOT__r_use_fifo){
				printf(" FIFO");
				if (m_core->v__DOT__sdcard_controller__DOT__r_fifo_wr)
					printf("-WR(%04x,%d,%d,%d)",
						m_core->v__DOT__sdcard_controller__DOT__fifo_rd_crc_reg,
						m_core->v__DOT__sdcard_controller__DOT__fifo_rd_crc_stb,
						m_core->v__DOT__sdcard_controller__DOT__ll_fifo_pkt_state,
						m_core->v__DOT__sdcard_controller__DOT__r_have_data_response_token);
				else
					printf("-RD(%04x,%d,%d,%d)",
						m_core->v__DOT__sdcard_controller__DOT__fifo_wr_crc_reg,
						m_core->v__DOT__sdcard_controller__DOT__fifo_wr_crc_stb,
						m_core->v__DOT__sdcard_controller__DOT__ll_fifo_wr_state,
						m_core->v__DOT__sdcard_controller__DOT__ll_fifo_wr_complete
						);
			}

			if (m_core->v__DOT__sdcard_controller__DOT__ll_fifo_rd)
				printf(" LL-RD");
			if (m_core->v__DOT__sdcard_controller__DOT__ll_fifo_wr)
				printf(" LL-WR");
			if (m_core->v__DOT__sdcard_controller__DOT__r_have_start_token)
				printf(" START-TOK");
			printf(" %3d/%02x",
				m_core->v__DOT__sdcard_controller__DOT__ll_fifo_addr,
				m_core->v__DOT__sdcard_controller__DOT__fifo_byte&0x0ff);
			*/


			/*
			printf(" DMAC[%d]: %08x/%08x/%08x(%03x)%d%d%d%d -- (%d,%d,%c)%c%c:@%08x-[%4d,%4d/%4d,%4d-#%4d]%08x",
				m_core->v__DOT__zippy__DOT__dma_controller__DOT__dma_state,
				m_core->v__DOT__zippy__DOT__dma_controller__DOT__cfg_waddr,
				m_core->v__DOT__zippy__DOT__dma_controller__DOT__cfg_raddr,
				m_core->v__DOT__zippy__DOT__dma_controller__DOT__cfg_len,
				m_core->v__DOT__zippy__DOT__dma_controller__DOT__cfg_blocklen_sub_one,
				m_core->v__DOT__zippy__DOT__dma_controller__DOT__last_read_request,
				m_core->v__DOT__zippy__DOT__dma_controller__DOT__last_read_ack,
				m_core->v__DOT__zippy__DOT__dma_controller__DOT__last_write_request,
				m_core->v__DOT__zippy__DOT__dma_controller__DOT__last_write_ack,
				m_core->v__DOT__zippy__DOT__dc_cyc,
				// m_core->v__DOT__zippy__DOT__dc_stb,
				(m_core->v__DOT__zippy__DOT__dma_controller__DOT__dma_state == 2)?1:0,

				((m_core->v__DOT__zippy__DOT__dma_controller__DOT__dma_state == 4)
				||(m_core->v__DOT__zippy__DOT__dma_controller__DOT__dma_state == 5)
				||(m_core->v__DOT__zippy__DOT__dma_controller__DOT__dma_state == 6))?'W':'R',
				//(m_core->v__DOT__zippy__DOT__dc_we)?'W':'R',
				(m_core->v__DOT__zippy__DOT__dc_ack)?'A':' ',
				(m_core->v__DOT__zippy__DOT__dc_stall)?'S':' ',
				m_core->v__DOT__zippy__DOT__dc_addr,
				m_core->v__DOT__zippy__DOT__dma_controller__DOT__rdaddr,
				m_core->v__DOT__zippy__DOT__dma_controller__DOT__nread,
				m_core->v__DOT__zippy__DOT__dma_controller__DOT__nracks,
				m_core->v__DOT__zippy__DOT__dma_controller__DOT__nwacks,
				m_core->v__DOT__zippy__DOT__dma_controller__DOT__nwritten,
				m_core->v__DOT__zippy__DOT__dc_data);
			*/

#ifdef	OPT_ZIPSYSTEM
			printf(" %08x-PIC%08x",
				m_core->v__DOT__zippy__DOT__main_int_vector,
				m_core->v__DOT__zippy__DOT__pic_data);
#endif

			printf(" R0 = %08x", m_core->v__DOT__zippy__DOT__thecpu__DOT__regset[0]);

			printf("\n"); fflush(stdout);
		} m_last_writeout = writeout;

		int writing_to_uart;
		writing_to_uart = (m_core->v__DOT__wb_stb)
				&&(m_core->v__DOT__wb_addr == 0x010b)
				&&(m_core->v__DOT__wb_we);
		if (writing_to_uart) {
			printf("SENT-TO-UART: %02x %c\n",
				(m_core->v__DOT__wb_data & 0x0ff),
				isgraph(m_core->v__DOT__wb_data&0x0ff)
				? m_core->v__DOT__wb_data&0x0ff
				: '.');
			assert((m_core->v__DOT__wb_data & (~0xff))==0);
		} 
#endif // DEBUGGING_OUTPUT
	}

#ifdef	DEBUGGING_OUTPUT
	bool	dcd_ce(void) {
		if (!m_core->v__DOT__zippy__DOT__thecpu__DOT__r_dcdvalid)
			return true;
		// if (!m_core->v__DOT__zippy__DOT__thecpu__DOT__op_stall)
			// return true;
		return false;
	}
#endif

};

BUSMASTER_TB	*tb;

void	busmaster_kill(int v) {
	tb->kill();
	fprintf(stderr, "KILLED!!\n");
	exit(0);
}

int	main(int argc, char **argv) {
	Verilated::commandArgs(argc, argv);
	tb = new BUSMASTER_TB;

	// signal(SIGINT,  busmaster_kill);

	tb->reset();
	if (argc > 1)
		tb->setsdcard(argv[1]);
	else
		tb->setsdcard("/dev/zero");

	while(1)
		tb->tick();

	exit(0);
}

