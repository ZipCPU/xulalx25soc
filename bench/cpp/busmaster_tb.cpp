////////////////////////////////////////////////////////////////////////////////
//
// Filename:	busmaster_tb.cpp
//
// Project:	XuLA2-LX25 SoC based upon the ZipCPU
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
// Copyright (C) 2015-2018, Gisselquist Technology, LLC
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
#include <signal.h>
#include <time.h>
#include <ctype.h>

#include "verilated.h"
#include "Vbusmaster.h"
#include "byteswap.h"

#include "cpudefs.h"
#ifdef	XULA25
#undef	XULA25
#define	CPUDEFS_XULA25
#endif


#include "design.h"
#ifdef	XULA25
#ifndef	CPUDEFS_XULA25
#error	XULA25 not defined consistently in busmaster.v and cpudefs.v
#endif
#else
#ifdef	CPUDEFS_XULA25
#error	XULA25 not defined consistently in busmaster.v and cpudefs.v
#endif
#endif

#include "testb.h"
// #include "twoc.h"
#include "pipecmdr.h"
#include "qspiflashsim.h"
#include "sdramsim.h"
#include "sdspisim.h"
#include "uartsim.h"
#include "zipelf.h"
#include "regdefs.h"

#include "port.h"

#ifdef	NEW_VERILATOR
#define	VVAR(A)	busmaster__DOT_ ## A
#else
#define	VVAR(A)	v__DOT_ ## A
#endif
#define	CPUVAR(A)		VVAR(_swic__DOT__thecpu__DOT_ ## A)


#define	sd_cmd_busy	VVAR(_sdcardi__DOT__r_cmd_busy)
#define	sd_clk		VVAR(_sdcardi__DOT__r_sdspi_clk)
#define	sd_cmd_state	VVAR(_sdcardi__DOT__r_cmd_state)
#define	sd_rsp_state	VVAR(_sdcardi__DOT__r_rsp_state)
#define	sd_ll_cmd_stb	VVAR(_sdcard__DOT__ll_cmd_stb)
#define	sd_ll_cmd_dat	VVAR(_sdcard__DOT__ll_cmd_dat)
#define	sd_ll_z_counter	VVAR(_sdcard__DOT__lowlevel__DOT__r_z_counter)
#define	sd_ll_clk_counter	VVAR(_sdcard__DOT__lowlevel__DOT__r_clk_counter)
#define	sd_ll_idle	VVAR(_sdcard__DOT__lowlevel__DOT__r_idle)
#define	sd_ll_state	VVAR(_sdcard__DOT__lowlevel__DOT__r_state)
#define	sd_ll_byte	VVAR(_sdcard__DOT__lowlevel__DOT__r_byte)
#define	sd_ll_ireg	VVAR(_sdcard__DOT__lowlevel__DOT__r_ireg)
#define	sd_ll_out_stb	VVAR(_sdcard__DOT__ll_out_stb)
#define	sd_ll_out_dat	VVAR(_sdcard__DOT__ll_out_dat)
#define	sd_lgblklen	VVAR(_sdcard__DOT__r_lgblklen)
#define	sd_fifo_rd_crc	VVAR(_sdcard__DOT__fifo_rd_crc_reg)
#define	sd_cmd_crc	VVAR(_sdcard__DOT__r_cmd_crc,)
#define	sd_cmd_crc_cnt	VVAR(_sdcard__DOT__r_cmd_crc_cnt)
#define	sd_fifo_rd_crc_stb	VVAR(_sdcard__DOT__fifo_rd_crc_stb)
#define	ll_fifo_pkt_state	VVAR(_sdcard__DOT__ll_fifo_pkt_state)
#define	sd_have_data_response_token	VVAR(_sdcard__DOT__r_have_data_response_token)
#define	sd_fifo_wr_crc		VVAR(_sdcard__DOT__fifo_wr_crc_reg)
#define	sd_fifo_wr_crc_stb	VVAR(_sdcard__DOT__fifo_wr_crc_stb,)
#define	sd_ll_fifo_wr_state	VVAR(_sdcard__DOT__ll_fifo_wr_state,)
#define	sd_ll_fifo_wr_complete	VVAR(_sdcard__DOT__ll_fifo_wr_complete)
#define	sd_use_fifo	VVAR(_sdcard__DOT__r_use_fifo)
#define	sd_fifo_wr	VVAR(_sdcard__DOT__r_fifo_wr)

#define	block_ram	VVAR(_ram__DOT__mem)
#define	cpu_cmd_halt	VVAR(_swic__DOT__cmd_halt)
#define	cpu_iflags	VVAR(_swic__DOT__thecpu__DOT__w_iflags)
#define	cpu_uflags	VVAR(_swic__DOT__thecpu__DOT__w_uflags)
#define	cpu_cmd_addr	VVAR(_swic__DOT__cmd_addr)
// CPU globals
#define	cpu_break 	VVAR(_swic__DOT__cpu_break)
#define	cpu_ipc		CPUVAR(_ipc)
#define	cpu_upc		CPUVAR(_SET_USER_PC__DOT__r_upc)
#define	cpu_gie		CPUVAR(_SET_GIE__DOT__r_gie)
#define	cpu_regs	CPUVAR(_regset)
#define	cpu_bus_err	CPUVAR(_bus_err)
#define	cpu_ibus_err	CPUVAR(_ibus_err_flag)
#define	cpu_ubus_err	CPUVAR(_SET_USER_BUSERR__DOT__r_ubus_err_flag)
#define	cpu_sleep	CPUVAR(_sleep)
//
#define	cpu_pf_pc	VVAR(_swic__DOT__thecpu__DOT__pf_pc)
#define	cpu_pf_cyc	VVAR(_swic__DOT__thecpu__DOT__pf_cyc)
#define	cpu_pf_illegal	VVAR(_swic__DOT__thecpu__DOT__pf_illegal)
#define	cpu_pf_valid	VVAR(_swic__DOT__thecpu__DOT__pf_valid)
#define	cpu_pf_instruction   VVAR(_swic__DOT__thecpu__DOT__pf_instruction)
#define	cpu_pf_instruction_pc VVAR(_swic__DOT__thecpu__DOT__pf_instruction_pc)
//
#define	cpu_early_branch CPUVAR(_instruction_decoder__DOT__GEN_EARLY_BRANCH_LOGIC__DOT__r_early_branch)
#define	cpu_branch_pc	CPUVAR(_instruction_decoder__DOT__GEN_EARLY_BRANCH_LOGIC__DOT__r_branch_pc)
//
#define	cpu_dcd_pc	CPUVAR(_dcd_pc)
#define	cpu_dcd_valid	CPUVAR(_instruction_decoder__DOT__r_valid)
//
#define	cpu_op_ce	CPUVAR(_op_ce)
#define	cpu_op_valid	CPUVAR(_op_valid)
#define	cpu_op_valid_mem	CPUVAR(_op_valid_mem)
#define	cpu_op_valid_alu	CPUVAR(_op_valid_alu)
#define	cpu_op_valid_div	CPUVAR(_op_valid_div)
#define	cpu_op_valid_fpu	CPUVAR(_op_valid_fpu)
#define	cpu_op_lock	CPUVAR(_OPLOCK__DOT__r_op_lock)
#define	cpu_op_sim	CPUVAR(_op_sim)
#define	cpu_sim_immv	CPUVAR(_op_sim_immv)
#define	cpu_op_pc	CPUVAR(_op_pc)
//
#define	cpu_alu_pc	CPUVAR(_GEN_ALU_PC__DOT__r_alu_pc)
#define	cpu_alu_ce	CPUVAR(_alu_ce)
#define	cpu_alu_wR	CPUVAR(_alu_wR)
#define	cpu_alu_reg	CPUVAR(_alu_reg)
#define	cpu_alu_valid	CPUVAR(_alu_valid)
//
#define	cpu_div_valid	CPUVAR(_div_valid)
//
#define	cpu_mem_rdaddr	CPUVAR(_domem__DOT__rdaddr)
#define	cpu_mem_wraddr	CPUVAR(_domem__DOT__wraddr)
#define	cpu_mem_wreg	CPUVAR(_mem_wreg)
#define	cpu_new_pc	CPUVAR(_new_pc)
//
#define	cpu_wr_reg_ce	CPUVAR(_wr_reg_ce)
#define	cpu_wr_flags_ce	CPUVAR(_wr_flags_ce)
#define	cpu_wr_reg_id	CPUVAR(_wr_reg_id)
#define	cpu_wr_gpreg_vl	CPUVAR(_wr_gpreg_vl)
#define	cpu_wr_spreg_vl	CPUVAR(_wr_spreg_vl)
//
// #define	VVAR(_wb_addr)		VVAR(_dwb_addr)
// #define	VVAR(_dwb_stall)	VVAR(_wb_stall)
// #define	VVAR(_dwb_ack)		VVAR(_wb_ack)
// #define	VVAR(_wb_cyc)		VVAR(_dwb_cyc)
// #define	VVAR(_wb_stb)		VVAR(_dwb_stb)
// #define	VVAR(_wb_we)		VVAR(_dwb_we)
// #define	VVAR(_dwb_idata)	VVAR(_wb_idata)
// #define	VVAR(_wb_data)		VVAR(_dwb_odata)
//
#define	wb_addr		VVAR(_dwb_addr)
#define	wb_stall	VVAR(_wb_stall)
#define	wb_ack		VVAR(_wb_ack)
#define	wb_cyc		VVAR(_dwb_cyc)
#define	wb_stb		VVAR(_dwb_stb)
#define	wb_we		VVAR(_dwb_we)
#define	wb_err		VVAR(_wb_err)
#define	wb_idata	VVAR(_wb_idata)
#define	wb_data		VVAR(_dwb_odata)
//
#define	sdram_clocks_til_idle	VVAR(_sdram__DOT__clocks_til_idle)
#define	cpu_pf_illegal		VVAR(_swic__DOT__thecpu__DOT__pf_illegal)
#define	cpu_dcd_illegal		VVAR(_swic__DOT__thecpu__DOT__dcd_illegal)
//
#define	cpu_op_stall		VVAR(_swic__DOT__thecpu__DOT__op_stall)
#define	cpu_op_stall		VVAR(_swic__DOT__thecpu__DOT__op_stall)
#define	cpu_op_illegal		VVAR(_swic__DOT__thecpu__DOT__op_illegal)
#define	cpu_pf_lastpc		VVAR(_swic__DOT__thecpu__DOT__pf__DOT__r_lastpc)
#define	cpu_pf_rvsrc		VVAR(_swic__DOT__thecpu__DOT__pf__DOT__rvsrc)
#define	tx_zero_baud_counter VVAR(_serialport__DOT__txmod__DOT__zero_baud_counter)
#define	tx_baud_counter		VVAR(_serialport__DOT__txmod__DOT__baud_counter)
#define	cpu_ubreak		CPUVAR(_SET_TRAP_N_UBREAK__DOT__r_ubreak)
#define	cpu_switch_to_interrupt	VVAR(_swic__DOT__thecpu__DOT__w_switch_to_interrupt)
#define	pic_interrupt		VVAR(_swic__DOT__MAIN_PIC__DOT__pic__DOT__r_interrupt)
#define	apic_interrupt		VVAR(_swic__DOT__ALT_PIC__DOT__ctri__DOT__r_interrupt)
#define	main_int_vector		VVAR(_swic__DOT__main_int_vector)
#define	alt_int_vector		VVAR(_swic__DOT__alt_int_vector)

#define	sdram_state		VVAR(_sdram__DOT__m_state)
#define	sdram_refresh_clk	VVAR(_sdram__DOT__refresh_clk)
#define	sdram_need_refresh	VVAR(_sdram__DOT__need_refresh)
#define	sdram_bank_active	VVAR(_sdram__DOT__bank_active)
#define	sdram_bank_row		VVAR(_sdram__DOT__bank_row)
#define	sdram_barrell_ack	VVAR(_sdram__DOT__r_barrell_ack)
#define	sdram_bus_cyc		VVAR(_sdram__DOT__bus_cyc)
#define	sdram_r_pending		VVAR(_sdram__DOT__r_pending)
#define	sdram_r_we		VVAR(_sdram__DOT__r_we)
#define	sdram_r_data		VVAR(_sdram__DOT__r_data)
#define	sdram_data		VVAR(_sdram_data)
#define	sdram_r_addr		VVAR(_sdram__DOT__r_addr)
#define	cpu_op_pipe		CPUVAR(_GEN_OP_PIPE__DOT__r_op_pipe)
#define	cpu_break_pending	CPUVAR(_GEN_PENDING_BREAK__DOT__r_break_pending)
#define	cpu_op_break		CPUVAR(_r_op_break)
#define	cpu_dcd_illegal		VVAR(_swic__DOT__thecpu__DOT__dcd_illegal)
#define	cpu_cyc_gbl		VVAR(_swic__DOT__thecpu__DOT__genblk26__DOT__MEM__DOT__domem__DOT__r_wb_cyc_gbl)
#define	cpu_cyc_lcl		VVAR(_swic__DOT__thecpu__DOT__genblk26__DOT__MEM__DOT__domem__DOT__r_wb_cyc_lcl)
#define	cpu_lcl_cyc		VVAR(_swic__DOT__cpu_lcl_cyc)
#define	cpu_halted		VVAR(_swic__DOT__thecpu__DOT__r_halted)
#define	cpu_alu_busy		CPUVAR(_doalu__DOT__r_busy)
#define	cpu_alu_illegal		CPUVAR(_r_alu_illegal)
#define	cpu_div_busy		CPUVAR(_div_busy)
#define	cpu_div_error		CPUVAR(_div_error)
#define	sdram_r_pending	VVAR(_sdram__DOT__r_pending)

#define	wbu_cyc		VVAR(_wbu_cyc)
#define	wbu_stb		VVAR(_wbu_stb)
#define	wbu_we		VVAR(_wbu_we)

#define	dwb_cyc		VVAR(_dwb_cyc)
#define	ext_cyc		VVAR(_swic__DOT__ext_cyc)
#define	exec_stb	VVAR(_genbus__DOT__exec_stb)
#ifdef	XULA25
#define	pic_gie		VVAR(_swic__DOT__MAIN_PIC__DOT__pic__DOT__r_gie)
#define	pic_any		VVAR(_swic__DOT__MAIN_PIC__DOT__pic__DOT__w_any)
#define	pic_int_enable	VVAR(_swic__DOT__MAIN_PIC__DOT__pic__DOT__r_int_enable)
#define	pic_int_state	VVAR(_swic__DOT__MAIN_PIC__DOT__pic__DOT__r_int_state)
#endif
#define	uart_tx_zero_baud	VVAR(_serialport__DOT__txmod__DOT__zero_baud_counter)
#define	uart_tx_busy		VVAR(_serialport__DOT__txmod__DOT__r_busy)
#define	uart_tx_data		VVAR(_serialport__DOT__txmod__DOT__lcl_data)
#define	uart_tx_counter		VVAR(_serialport__DOT__txmod__DOT__baud_counter)
#define	uart_tx_state		VVAR(_serialport__DOT__txmod__DOT__state)
#define	arbiter_owner	VVAR(_wbu_zip_arbiter__DOT__r_a_owner)
#define	wbu_stb		VVAR(_wbu_stb)
#define	cpu_mem_stb	VVAR(_swic__DOT__thecpu__DOT__mem_stb_gbl)
#define	dwb_idata	VVAR(_dwb_idata)
#define	wbu_stall	VVAR(___Vcellinp__genbus____pinNumber10)
#define	wbu_ack		VVAR(___Vcellinp__genbus____pinNumber9)
#define	sdram_maintenance	VVAR(_sdram__DOT__maintenance_mode)
#define	sdram_sel		VVAR(_iovec)&8
#define	sdram_cs	VVAR(_sdram__DOT__m_ram_cs_n)
#define	sdram_ras	VVAR(_sdram__DOT__m_ram_ras_n)
#define	sdram_cas	VVAR(_sdram__DOT__m_ram_cas_n)
#define	sdram_we	VVAR(_sdram__DOT__m_ram_we_n)

#define	FLASH_ADDRESS	0x100000
#define	FLASH_LENGTH	0x100000

#define	SDRAM_ADDRESS	0x2000000
#define	SDRAM_LENGTH	0x2000000

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
	bool		m_done;
	int		m_bomb;

	BUSMASTER_TB(void) : PIPECMDR(FPGAPORT), m_uart(FPGAPORT+1) {
		m_start_time = time(NULL);
		m_last_pic = 0;
		m_last_tx_state = 0;
		m_done = false;
		m_bomb = 0;
	}

	void	reset(void) {
		m_core->i_clk = 0;
		m_core->eval();
	}

	void	trace(const char *vcd_trace_file_name) {
		fprintf(stderr, "Opening TRACE(%s)\n", vcd_trace_file_name);
		opentrace(vcd_trace_file_name);
	}

	void	close(void) {
		TESTB<Vbusmasterr>::closetrace();
	}

	bool	load(uint32_t addr, const char *buf, uint32_t len) {
		uint32_t	start, offset, wlen, base, naddr;

		base  = RAMBASE;
		naddr = MEMBYTES;

		if ((addr >= base)&&(addr < base + naddr)) {
			// If the start access is in bkram
			start = (addr > base) ? (addr-base) : 0;
			offset = (start + base) - addr;
			wlen = (len-offset > naddr - start)
				? (naddr - start) : len - offset;
//#ifdef	BKRAM_ACCESS
			// FROM bkram.SIM.LOAD
			start = start & (-4);
			wlen = (wlen+3)&(-4);

			// Need to byte swap data to get it into the memory
			char	*bswapd = new char[len+8];
			memcpy(bswapd, &buf[offset], wlen);
			byteswapbuf(len>>2, (uint32_t *)bswapd);
			memcpy(&m_core->block_ram[start>>2], bswapd, wlen);
			delete	bswapd;
			if (addr + len > base + naddr)
				return load(base + naddr, &buf[offset+wlen], len-wlen);
			return true;
//#else	// BKRAM_ACCESS
			//return false;
//#endif	// BKRAM_ACCESS
		}

		base  = SPIFLASH;
		naddr = FLASHBYTES;

		if ((addr >= base)&&(addr < base + naddr)) {
			// If the start access is in flash
			start = (addr > base) ? (addr-base) : 0;
			offset = (start + base) - addr;
			wlen = (len-offset > naddr - start)
				? (naddr - start) : len - offset;
#ifdef	FLASH_ACCESS
			// FROM flash.SIM.LOAD
			m_flash.load(start, &buf[offset], wlen);
			// AUTOFPGA::Now clean up anything else
			// Was there more to write than we wrote?
			if (addr + len > base + naddr)
				return load(base + naddr, &buf[offset+wlen], len-wlen);
			return true;
#else	// FLASH_ACCESS
			return false;
#endif	// FLASH_ACCESS
		}


		base  = SDRAMBASE;
		naddr = SDRAMBYTES;

		if ((addr >= base)&&(addr < base + naddr)) {
			// If the start access is in flash
			start = (addr > base) ? (addr-base) : 0;
			offset = (start + base) - addr;
			wlen = (len-offset > naddr - start)
				? (naddr - start) : len - offset;
#ifndef	BYPASS_SDRAM_ACCESS
			// FROM flash.SIM.LOAD
			m_sdram.load(start, &buf[offset], wlen);
			if (addr + len > base + naddr)
				return load(base + naddr, &buf[offset+wlen], len-wlen);
			return true;
#else	// !BYPASS_SDRAM_ACCESS
			return false;
#endif	// !BYPASS_SDRAM_ACCESS
		}

		return false;
	}

// Looking for string: SIM.METHODS
#ifdef	SDCARD_ACCESS
	void	setsdcard(const char *fn) {
		m_sdcard.load(fn);
	}
#endif

	void	loadelf(const char *elfname) {
		ELFSECTION	**secpp, *secp;
		uint32_t	entry;

		elfread(elfname, entry, secpp);

		for(int s=0; secpp[s]->m_len; s++) {
			secp = secpp[s];

			load(secp->m_start, secp->m_data, secp->m_len);
		}
	}

	bool	gie(void) {
		return (m_core->cpu_gie);
	}

	void dump(const uint32_t *regp) {
		uint32_t	uccv, iccv;
		fflush(stderr);
		fflush(stdout);
		printf("ZIPM--DUMP: ");
		if (gie())
			printf("Interrupts-enabled\n");
		else
			printf("Supervisor mode\n");
		printf("\n");

		iccv = m_core->cpu_iflags;
		uccv = m_core->cpu_uflags;

		printf("sR0 : %08x ", regp[0]);
		printf("sR1 : %08x ", regp[1]);
		printf("sR2 : %08x ", regp[2]);
		printf("sR3 : %08x\n",regp[3]);
		printf("sR4 : %08x ", regp[4]);
		printf("sR5 : %08x ", regp[5]);
		printf("sR6 : %08x ", regp[6]);
		printf("sR7 : %08x\n",regp[7]);
		printf("sR8 : %08x ", regp[8]);
		printf("sR9 : %08x ", regp[9]);
		printf("sR10: %08x ", regp[10]);
		printf("sR11: %08x\n",regp[11]);
		printf("sR12: %08x ", regp[12]);
		printf("sSP : %08x ", regp[13]);
		printf("sCC : %08x ", iccv);
		printf("sPC : %08x\n",regp[15]);

		printf("\n");

		printf("uR0 : %08x ", regp[16]);
		printf("uR1 : %08x ", regp[17]);
		printf("uR2 : %08x ", regp[18]);
		printf("uR3 : %08x\n",regp[19]);
		printf("uR4 : %08x ", regp[20]);
		printf("uR5 : %08x ", regp[21]);
		printf("uR6 : %08x ", regp[22]);
		printf("uR7 : %08x\n",regp[23]);
		printf("uR8 : %08x ", regp[24]);
		printf("uR9 : %08x ", regp[25]);
		printf("uR10: %08x ", regp[26]);
		printf("uR11: %08x\n",regp[27]);
		printf("uR12: %08x ", regp[28]);
		printf("uSP : %08x ", regp[29]);
		printf("uCC : %08x ", uccv);
		printf("uPC : %08x\n",regp[31]);
		printf("\n");
		fflush(stderr);
		fflush(stdout);
	}


	void	execsim(const uint32_t imm) {
		uint32_t	*regp = m_core->cpu_regs;
		int		rbase;
		rbase = (gie())?16:0;

		fflush(stdout);
		if ((imm & 0x03fffff)==0)
			return;
		// fprintf(stderr, "SIM-INSN(0x%08x)\n", imm);
		if ((imm & 0x0fffff)==0x00100) {
			// SIM Exit(0)
			close();
			exit(0);
		} else if ((imm & 0x0ffff0)==0x00310) {
			// SIM Exit(User-Reg)
			int	rcode;
			rcode = regp[(imm&0x0f)+16] & 0x0ff;
			close();
			exit(rcode);
		} else if ((imm & 0x0ffff0)==0x00300) {
			// SIM Exit(Reg)
			int	rcode;
			rcode = regp[(imm&0x0f)+rbase] & 0x0ff;
			close();
			exit(rcode);
		} else if ((imm & 0x0fff00)==0x00100) {
			// SIM Exit(Imm)
			int	rcode;
			rcode = imm & 0x0ff;
			close();
			exit(rcode);
		} else if ((imm & 0x0fffff)==0x002ff) {
			// Full/unconditional dump
			printf("SIM-DUMP\n");
			dump(regp);
		} else if ((imm & 0x0ffff0)==0x00200) {
			// Dump a register
			int rid = (imm&0x0f)+rbase;
			printf("%8ld @%08x R[%2d] = 0x%08x\n", m_tickcount,
			m_core->cpu_ipc,
			rid, regp[rid]);
		} else if ((imm & 0x0ffff0)==0x00210) {
			// Dump a user register
			int rid = (imm&0x0f);
			printf("%8ld @%08x uR[%2d] = 0x%08x\n", m_tickcount,
				m_core->cpu_ipc,
				rid, regp[rid+16]);
		} else if ((imm & 0x0ffff0)==0x00230) {
			// SOUT[User Reg]
			int rid = (imm&0x0f)+16;
			printf("%c", regp[rid]&0x0ff);
		} else if ((imm & 0x0fffe0)==0x00220) {
			// SOUT[User Reg]
			int rid = (imm&0x0f)+rbase;
			printf("%c", regp[rid]&0x0ff);
		} else if ((imm & 0x0fff00)==0x00400) {
			// SOUT[Imm]
			printf("%c", imm&0x0ff);
		} else { // if ((insn & 0x0f7c00000)==0x77800000)
			uint32_t	immv = imm & 0x03fffff;
			// Simm instruction that we dont recognize
			// if (imm)
			printf("SIM 0x%08x\n", immv);
		} fflush(stdout);
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
				m_core->o_ram_data, m_core->o_ram_dqm);

		m_core->i_rx_uart = m_uart(m_core->o_tx_uart,
				m_core->VVAR(_serialport__DOT__r_setup));
		PIPECMDR::tick();
		// Sim instructions
		if ((m_core->cpu_op_sim)
			&&(m_core->cpu_op_valid)
			&&(m_core->cpu_alu_ce)
			&&(!m_core->cpu_new_pc)) {
			//
			execsim(m_core->cpu_sim_immv);
		}

// #define	DEBUGGING_OUTPUT
#ifdef	DEBUGGING_OUTPUT
		bool	writeout = false;
		/*
		if (m_core->sdram_r_pending)
			writeout = true;
		else if (m_core->sdram_bank_active[0])
			writeout = true;
		else if (m_core->sdram_bank_active[1])
			writeout = true;
		else if (m_core->sdram_bank_active[2])
			writeout = true;
		else if (m_core->sdram_bank_active[3])
			writeout = true;
		*/

		// if ((m_core->wbu_cyc)&&(!m_core->wbu_we))
			// writeout = true;
		/*
		if ((m_core->wbu_cyc)&&(!m_core->wbu_we))
			writeout = true;
		if (m_core->exec_stb)
			writeout = true;
		*/

		// if ((m_core->cpu_early_branch)
			// &&(m_core->v__DOT__swic__DOT__thecpu__DOT__instruction == 0x7883ffff))
			// m_busy+=2;
		// else if (m_busy > 0) m_busy--;

		if ((!m_core->cpu_cmd_halt)
				&&(!m_core->cpu_sleep))
			writeout = true;
		// if (m_core->v__DOT__uart_tx_int)
			// writeout = true;
#ifdef	XULA25
		if (m_core->pic_any)
			writeout = true;
#endif

#ifdef	XULA25
		unsigned this_pic = ((m_core->pic_int_enable)<<16) | 
				(m_core->pic_int_state);
#else
		unsigned this_pic = 0;
#endif

		// if (this_pic != m_last_pic)
			// writeout = true;
		unsigned tx_state = ((m_core->uart_tx_zero_baud)<<20)
			|((m_core->uart_tx_busy)<<16)
			|((m_core->uart_tx_data)<<8)
			|((m_core->uart_tx_counter&0x0f)<<4)
			|(m_core->uart_tx_state);
		/*
		if (tx_state != m_last_tx_state)
			writeout = true;
		*/
		int bus_owner = m_core->arbiter_owner;
		/*
		if (bus_owner != m_last_bus_owner)
			writeout = true;
		*/
		/*
		writeout = (writeout)||(m_core->i_rx_stb)
				||((m_core->o_tx_stb)&&(!m_core->i_tx_busy));
		writeout = (writeout)||(m_core->v__DOT____Vcellinp__genbus____pinNumber9);
		writeout = (writeout)||(m_core->wb_stb);
		writeout = (writeout)||(m_core->wb_err);
		*/

writeout = true;

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
				m_core->wbu_cyc,
				0, // m_core->v__DOT____Vcellinp__wbu_zip_arbiter____pinNumber10,
				m_core->dwb_cyc, // was zip_cyc
#ifdef	XULA25
				0, //(m_core->ext_cyc),
#else
				0,
#endif
				m_core->wb_cyc,
				//
				m_core->arbiter_owner ?'Z':'j',
				m_core->wbu_stb,
				// 0, // m_core->v__DOT__dwb_stb, // was zip_stb
				m_core->cpu_mem_stb,
				m_core->wb_stb,
				//
				(m_core->wb_we)?'W':'R',
				m_core->wb_data,
					m_core->wb_idata,
				m_core->wb_addr,
				(m_core->wb_ack)?'A':
					(m_core->wbu_ack)?'a':' ',
				(m_core->wb_stall)?'S':
					(m_core->wbu_stall)?'s':' ',
				(m_core->wb_err)?'E':'.');

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

			// SDRAM debug lines
			printf("%c%c[%d%d%d%d,%d%d%d%d,%d:%04x%c]@%06x(%d) ->%06x%c",
				(m_core->sdram_maintenance)?'M':' ',
				(m_core->sdram_sel)?'!':' ',
				m_core->sdram_cs,  m_core->sdram_ras,
				m_core->sdram_cas, m_core->sdram_we,
				m_core->o_ram_cs_n, m_core->o_ram_ras_n,
				m_core->o_ram_cas_n, m_core->o_ram_we_n,
				m_core->o_ram_bs, m_core->o_ram_data,
				(m_core->o_ram_drive_data)?'D':'-',
				m_core->o_ram_addr,
					(m_core->o_ram_addr>>10)&1,
				m_core->i_ram_data,
				(m_core->o_ram_drive_data)?'-':'V');

			printf(" SD[%d,%d-%3x%d]",
				m_core->sdram_state,
				m_sdram.pwrup(),
				m_core->sdram_refresh_clk,
				m_core->sdram_need_refresh);

			printf(" BNK[%d:%6x,%d:%6x,%d:%6x,%d:%6x],%x%d",
				m_core->sdram_bank_active[0],
				m_core->sdram_bank_row[0],
				m_core->sdram_bank_active[1],
				m_core->sdram_bank_row[1],
				m_core->sdram_bank_active[2],
				m_core->sdram_bank_row[2],
				m_core->sdram_bank_active[3],
				m_core->sdram_bank_row[3],
				m_core->sdram_clocks_til_idle,
				m_core->sdram_barrell_ack);

			printf(" %s%s%c[%08x@%06x]",
				(m_core->sdram_bus_cyc)?"C":" ",
				(m_core->sdram_r_pending)?"PND":"   ",
				(m_core->sdram_r_we)?'W':'R',
				(m_core->sdram_r_we)
					?(m_core->sdram_r_data)
					:(m_core->sdram_data),
				(m_core->sdram_r_addr));

			// CPU Pipeline debugging
			printf("%s%s%s%s%s%s%s%s%s%s%s",
				// (m_core->v__DOT__swic__DOT__dbg_ack)?"A":"-",
				// (m_core->v__DOT__swic__DOT__dbg_stall)?"S":"-",
				// (m_core->v__DOT__swic__DOT__sys_dbg_cyc)?"D":"-",
				(m_core->cpu_cyc_lcl)?"L":"-",
				(m_core->cpu_halted)?"Z":"-",
				(m_core->cpu_break)?"!":"-",
				(m_core->cpu_cmd_halt)?"H":"-",
				(m_core->cpu_gie)?"G":"-",
				(m_core->cpu_pf_cyc)?"P":"-",
				(m_core->cpu_pf_valid)?"V":"-",
				(m_core->cpu_pf_illegal)?"i":" ",
				(m_core->cpu_new_pc)?"N":"-",
				(m_core->cpu_cyc_gbl)?"G":"-",
				(m_core->cpu_lcl_cyc)?"L":"-");
			printf("|%s%s%s%s%s%s",
				(m_core->cpu_dcd_valid)?"D":"-",
				(dcd_ce())?"d":"-",
				"x", // (m_core->v__DOT__swic__DOT__thecpu__DOT__dcdA_stall)?"A":"-",
				"x", // (m_core->v__DOT__swic__DOT__thecpu__DOT__dcdB_stall)?"B":"-",
				"x", // (m_core->v__DOT__swic__DOT__thecpu__DOT__dcdF_stall)?"F":"-",
				(m_core->cpu_dcd_illegal)?"i":"-");
			
			printf("|%s%s%s%s%s%s%s%s%s%s",
				(m_core->cpu_op_valid)?"O":"-",
				(m_core->cpu_op_ce)?"k":"-",
				(m_core->cpu_op_stall)?"s":"-",
				(m_core->cpu_op_illegal)?"i":"-",
				(m_core->cpu_op_break)?"B":"-",
				(m_core->cpu_op_lock)?"L":"-",
				(m_core->cpu_op_pipe)?"P":"-",
				(m_core->cpu_break_pending)?"p":"-",
				(m_core->cpu_gie)?"G":"-",
				(m_core->cpu_op_valid_alu)?"A":"-");
			printf("|%s%s%s%s%s",
				(m_core->cpu_alu_ce)?"a":"-",
				" ", // (m_core->v__DOT__swic__DOT__thecpu__DOT__alu_stall)?"s":"-",
				(m_core->cpu_alu_busy)?"B":"-",
				(m_core->cpu_gie)?"G":"-",
				(m_core->cpu_alu_illegal)?"i":"-");
#define	cpu_adf_ce_unconditional	CPUVAR(_adf_ce_unconditional)
#define	cpu_mem_ce		CPUVAR(_mem_ce)
			printf("|%s%s%s%2x %s%s%s %2d %2d",
				(m_core->cpu_op_valid_mem)?"M":"-",
				(m_core->cpu_mem_ce)?"m":"-",
				(m_core->cpu_adf_ce_unconditional)?"!":"-",
				(m_core->cpu_cmd_addr),
				(m_core->cpu_bus_err)?"BE":"  ",
				(m_core->cpu_ibus_err)?"IB":"  ",
				(m_core->cpu_ubus_err)?"UB":"  ",
#define	cpu_pmem_rdaddr	CPUVAR(_genblk26__DOT__MEM__DOT__domem__DOT__rdaddr)
#define	cpu_pmem_wraddr	CPUVAR(_genblk26__DOT__MEM__DOT__domem__DOT__wraddr)
				m_core->cpu_pmem_rdaddr,
				m_core->cpu_pmem_wraddr);
#ifdef	XULA25
			printf("|%s%s",
				(m_core->cpu_div_busy)?"D":"-",
				(m_core->cpu_div_error)?"E":"-");
#endif
			printf("|%s%s[%2x]%08x",
				(m_core->cpu_wr_reg_ce)?"W":"-",
				(m_core->cpu_wr_flags_ce)?"F":"-",
				m_core->cpu_wr_reg_id,
				m_core->cpu_wr_gpreg_vl);

			// Program counter debugging
			printf(" PC0x%08x/%08x/%08x-%08x %s0x%08x", 
				m_core->cpu_pf_pc,
				m_core->cpu_ipc,
				m_core->cpu_upc,
				m_core->cpu_pf_instruction,
				(m_core->cpu_early_branch)?"EB":"  ",
				m_core->cpu_branch_pc
				);
			// More in-depth
			printf("[%c%08x,%c%08x,%c%08x]",
				(m_core->cpu_dcd_valid)?'D':'-',
				m_core->cpu_dcd_pc,
				(m_core->cpu_op_valid)?'O':'-',
				m_core->cpu_op_pc,
				(m_core->cpu_alu_valid)?'A':'-',
				m_core->cpu_alu_pc);
			
			/*	
			// Prefetch debugging
			printf(" [PC%08x,LST%08x]->[%d%s%s](%d,%08x/%08x)->%08x@%08x",
				m_core->cpu_pf_pc,
				m_core->cpu_pf_lastpc,
				m_core->cpu_pf_rvsrc,
				(m_core->v__DOT__swic__DOT__thecpu__DOT__pf__DOT__rvsrc)
				?((m_core->v__DOT__swic__DOT__thecpu__DOT__pf__DOT__r_v_from_pc)?"P":" ")
				:((m_core->v__DOT__swic__DOT__thecpu__DOT__pf__DOT__r_v_from_pc)?"p":" "),
				(!m_core->v__DOT__swic__DOT__thecpu__DOT__pf__DOT__rvsrc)
				?((m_core->v__DOT__swic__DOT__thecpu__DOT__pf__DOT__r_v_from_last)?"l":" ")
				:((m_core->v__DOT__swic__DOT__thecpu__DOT__pf__DOT__r_v_from_last)?"L":" "),
				m_core->v__DOT__swic__DOT__thecpu__DOT__pf__DOT__isrc,
				m_core->v__DOT__swic__DOT__thecpu__DOT__pf__DOT__r_pc_cache,
				m_core->v__DOT__swic__DOT__thecpu__DOT__pf__DOT__r_last_cache,
				m_core->v__DOT__swic__DOT__thecpu__DOT__instruction,
				m_core->v__DOT__swic__DOT__thecpu__DOT__instruction_pc);
			*/

			// Decode Stage debugging
			// (nothing)

			// Op Stage debugging
//			printf(" Op(%02x,%02x->%02x)",
//				m_core->v__DOT__swic__DOT__thecpu__DOT__dcdOp,
//				m_core->v__DOT__swic__DOT__thecpu__DOT__opn,
//				m_core->v__DOT__swic__DOT__thecpu__DOT__opR);

			printf(" %s[%02x]=%08x(%08x)",
				m_core->cpu_wr_reg_ce?"WR":"--",
				m_core->cpu_wr_reg_id,
				m_core->cpu_wr_gpreg_vl,
#ifdef	XULA25
				m_core->cpu_wr_spreg_vl
#else
				0
#endif
				);
			printf(" Rid=(%d|%d)?%02x:%02x",
				m_core->cpu_alu_wR,
#ifdef	XULA25
				m_core->cpu_div_valid,
#else
				0,
#endif
				m_core->cpu_alu_reg,
				m_core->cpu_mem_wreg);

			// domem, the pipelined memory unit debugging
/*
			printf(" M[%s@0x%08x]",
				(m_core->v__DOT__swic__DOT__thecpu__DOT__op_valid_mem)
				?((m_core->v__DOT__swic__DOT__thecpu__DOT__opn&1)?"W":"R")
				:"-",
#ifdef	XULA25
				m_core->v__DOT__swic__DOT__cpu_addr
#else
				0
#endif
				);
*/

/*
			printf("%s%s",
				(m_core->v__DOT__swic__DOT__thecpu__DOT__domem__DOT__cyc)?"B":"-",
				(m_core->v__DOT__swic__DOT__thecpu__DOT__mem_rdbusy)?"r":"-");
*/
#ifdef	XULA25
			printf(" %s-%s %04x/%04x",
				(m_core->pic_any)?"PIC":"pic",
				(m_core->pic_gie)?"INT":"( )",
				m_core->pic_int_enable,
				m_core->pic_int_state);
#else
			printf(" %s-%s %04x/%04x",
				(m_core->pic_any)?"PIC":"pic",
				(m_core->pic_gie)?"INT":"( )",
				m_core->pic_int_enable,
				m_core->pic_int_state);
#endif
	

		/*
			printf(" %s",
				(m_core->v__DOT__swic__DOT__thecpu__DOT__cc_invalid_for_dcd)?"CCI":"   ");
		*/

			/*
			// Illegal instruction debugging
			printf(" ILL[%s%s%s%s%s%s]",
				(m_core->v__DOT__swic__DOT__thecpu__DOT__pf_err)?"WB":"  ",
				(m_core->v__DOT__swic__DOT__thecpu__DOT__pf_illegal)?"PF":"  ",
				(m_core->v__DOT__swic__DOT__thecpu__DOT__dcd_illegal)?"DCD":"   ",
				(m_core->v__DOT__swic__DOT__thecpu__DOT__op_illegal)?"OP":"  ",
				(m_core->v__DOT__swic__DOT__thecpu__DOT__r_alu_illegal)?"ALU":"   ",
				(m_core->v__DOT__swic__DOT__thecpu__DOT__ill_err_u)?"ILL":"   ");

			*/

			/*
			printf(" UART%08x/%d-%08x", m_last_tx_state,
				m_core->tx_zero_baud_counter,
				m_core->v__DOT__serialport__DOT__txmod__DOT__baud_counter);
			*/

			// Debug some conditions
			if (m_core->cpu_ubreak)
				printf(" BREAK");
			// if (m_core->v__DOT__swic__DOT__thecpu__DOT__w_switch_to_interrupt)
				// printf(" TO-INT");
#ifdef	XULA25
			if (m_core->pic_interrupt)
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
				m_core->v__DOT__swic__DOT__dma_controller__DOT__dma_state,
				m_core->v__DOT__swic__DOT__dma_controller__DOT__cfg_waddr,
				m_core->v__DOT__swic__DOT__dma_controller__DOT__cfg_raddr,
				m_core->v__DOT__swic__DOT__dma_controller__DOT__cfg_len,
				m_core->v__DOT__swic__DOT__dma_controller__DOT__cfg_blocklen_sub_one,
				m_core->v__DOT__swic__DOT__dma_controller__DOT__last_read_request,
				m_core->v__DOT__swic__DOT__dma_controller__DOT__last_read_ack,
				m_core->v__DOT__swic__DOT__dma_controller__DOT__last_write_request,
				m_core->v__DOT__swic__DOT__dma_controller__DOT__last_write_ack,
				m_core->v__DOT__swic__DOT__dc_cyc,
				// m_core->v__DOT__swic__DOT__dc_stb,
				(m_core->v__DOT__swic__DOT__dma_controller__DOT__dma_state == 2)?1:0,

				((m_core->v__DOT__swic__DOT__dma_controller__DOT__dma_state == 4)
				||(m_core->v__DOT__swic__DOT__dma_controller__DOT__dma_state == 5)
				||(m_core->v__DOT__swic__DOT__dma_controller__DOT__dma_state == 6))?'W':'R',
				//(m_core->v__DOT__swic__DOT__dc_we)?'W':'R',
				(m_core->v__DOT__swic__DOT__dc_ack)?'A':' ',
				(m_core->v__DOT__swic__DOT__dc_stall)?'S':' ',
				m_core->v__DOT__swic__DOT__dc_addr,
				m_core->v__DOT__swic__DOT__dma_controller__DOT__rdaddr,
				m_core->v__DOT__swic__DOT__dma_controller__DOT__nread,
				m_core->v__DOT__swic__DOT__dma_controller__DOT__nracks,
				m_core->v__DOT__swic__DOT__dma_controller__DOT__nwacks,
				m_core->v__DOT__swic__DOT__dma_controller__DOT__nwritten,
				m_core->v__DOT__swic__DOT__dc_data);
			*/

#ifdef	OPT_ZIPSYSTEM
#define	main_int_vector	VVAR(_swic__DOT__main_int_vector)
#define	pic_data	VVAR(_swic__DOT__pic_data)
			printf(" %08x-PIC%08x",
				m_core->main_int_vector,
				m_core->pic_data);
#endif

			printf(" R0 = %08x", m_core->cpu_regs[0]);

			printf("\n"); fflush(stdout);
		} m_last_writeout = writeout;

		int writing_to_uart;
		writing_to_uart = (m_core->wb_stb)&&(m_core->wb_addr == 0x010b)
				&&(m_core->wb_we);
		if (writing_to_uart) {
			printf("SENT-TO-UART: %02x %c\n",
				(m_core->wb_data & 0x0ff),
				isgraph(m_core->wb_data&0x0ff)
				? m_core->wb_data&0x0ff : '.');
			assert((m_core->wb_data & (~0xff))==0);
		} 
#endif // DEBUGGING_OUTPUT
if (m_core->cpu_break) {
	m_bomb++;
	dump(m_core->cpu_regs);
} else if (m_bomb) {
	if (m_bomb++ > 12)
		m_done = true;
	fprintf(stderr, "BREAK-BREAK-BREAK (m_bomb = %d)%s\n", m_bomb,
		(m_done)?" -- DONE!":"");
}

	}

#ifdef	DEBUGGING_OUTPUT
	bool	dcd_ce(void) {
		if (!m_core->cpu_dcd_valid)
			return true;
		// if (!m_core->cpu_op_stall)
			// return true;
		return false;
	}
#endif

	bool	done(void) {
		if (!m_trace)
			return m_done;
		else
			return (m_done)||(m_tickcount > 6000000);
	}
};

BUSMASTER_TB	*tb;

void	busmaster_kill(int v) {
	tb->kill();
	fprintf(stderr, "KILLED!!\n");
	exit(0);
}

void	usage(void) {
	fprintf(stderr, "USAGE: busmaster_tb <options> [zipcpu-elf-file]\n");
	fprintf(stderr,
#ifdef	SDCARD_ACCESS
"\t-c <img-file>\n"
"\t\tSpecifies a memory image which will be used to make the SD-card\n"
"\t\tmore realistic.  Reads from the SD-card will be directed to\n"
"\t\t\"sectors\" within this image.\n\n"
#endif
"\t-d\tSets the debugging flag\n"
"\t-t <filename>\n"
"\t\tTurns on tracing, sends the trace to <filename>--assumed to\n"
"\t\tbe a vcd file\n"
);
}

int	main(int argc, char **argv) {
	const	char *elfload = NULL,
#ifdef	SDCARD_ACCESS
			*sdimage_file = NULL,
#endif
			*trace_file = NULL; // "trace.vcd";
	bool	debug_flag = false, willexit = false;
//	int	fpga_port = FPGAPORT, serial_port = -(FPGAPORT+1);
//	int	copy_comms_to_stdout = -1;
#ifdef	OLEDSIM_H
	Gtk::Main	main_instance(argc, argv);
#endif
	Verilated::commandArgs(argc, argv);
	BUSMASTER_TB	*tb = new BUSMASTER_TB;

	for(int argn=1; argn < argc; argn++) {
		if (argv[argn][0] == '-') for(int j=1;
					(j<512)&&(argv[argn][j]);j++) {
			switch(tolower(argv[argn][j])) {
#ifdef	SDCARD_ACCESS
			case 'c': sdimage_file = argv[++argn]; j = 1000; break;
#endif
			case 'd': debug_flag = true;
				if (trace_file == NULL)
					trace_file = "trace.vcd";
				break;
			// case 'p': fpga_port = atoi(argv[++argn]); j=1000; break;
			// case 's': serial_port=atoi(argv[++argn]); j=1000; break;
			case 't': trace_file = argv[++argn]; j=1000; break;
			case 'h': usage(); exit(0); break;
			default:
				fprintf(stderr, "ERR: Unexpected flag, -%c\n\n",
					argv[argn][j]);
				usage();
				break;
			}
		} else if (iself(argv[argn])) {
			elfload = argv[argn];
		} else if (0 == access(argv[argn], R_OK)) {
			sdimage_file = argv[argn];
		} else {
			fprintf(stderr, "ERR: Cannot read %s\n", argv[argn]);
			perror("O/S Err:");
			exit(EXIT_FAILURE);
		}
	}

	if (elfload) {
		/*
		if (serial_port < 0)
			serial_port = 0;
		if (copy_comms_to_stdout < 0)
			copy_comms_to_stdout = 0;
		tb = new TESTBENCH(fpga_port, serial_port,
			(copy_comms_to_stdout)?true:false, debug_flag);
		*/
		willexit = true;
	} else {
		/*
		if (serial_port < 0)
			serial_port = -serial_port;
		if (copy_comms_to_stdout < 0)
			copy_comms_to_stdout = 1;
		tb = new TESTBENCH(fpga_port, serial_port,
			(copy_comms_to_stdout)?true:false, debug_flag);
		*/
	}

	if (debug_flag) {
		printf("Opening Bus-master with\n");
		printf("\tDebug Access port = %d\n", FPGAPORT); // fpga_port);
		printf("\tSerial Console    = %d\n", FPGAPORT+1);
			// (serial_port == 0) ? " (Standard output)" : "");
		/*
		printf("\tDebug comms will%s be copied to the standard output%s.",
			(copy_comms_to_stdout)?"":" not",
			((copy_comms_to_stdout)&&(serial_port == 0))
			? " as well":"");
		*/
		printf("\tVCD File         = %s\n", trace_file);
	} if (trace_file)
		tb->opentrace(trace_file);

	tb->reset();
#ifdef	SDCARD_ACCESS
	tb->setsdcard(sdimage_file);
#endif

	if (elfload) {
		uint32_t	entry;
		ELFSECTION	**secpp = NULL, *secp;
		elfread(elfload, entry, secpp);

		if (secpp) for(int i=0; secpp[i]->m_len; i++) {
			secp = secpp[i];
			tb->load(secp->m_start, secp->m_data, secp->m_len);
		}

		tb->m_core->cpu_ipc = entry;
		tb->tick();
		tb->m_core->cpu_ipc = entry;
		tb->m_core->cpu_cmd_halt = 0;
		tb->tick();
	}

fprintf(stderr, "STARTING UP\n");

	if (willexit) {
		while(!tb->done())
			tb->tick();
	} else
		while(true)
			tb->tick();

	tb->close();
	tb->kill();
	delete tb;

	return	EXIT_SUCCESS;
}

