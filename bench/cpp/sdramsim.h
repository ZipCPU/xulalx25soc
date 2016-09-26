#ifndef	SDRAMSIM_H

#define	NBANKS	4
#define	POWERED_UP_STATE	6
#define	CLK_RATE_HZ		100000000 // = 100 MHz = 100 * 10^6
#define	PWRUP_WAIT_CKS		((int)(.000200 * CLK_RATE_HZ))
#define	MAX_BANKOPEN_TIME	((int)(.000100 * CLK_RATE_HZ))
#define	MAX_REFRESH_TIME	((int)(.064 * CLK_RATE_HZ))
#define	SDRAM_QSZ		16

class	SDRAMSIM {
	int	m_pwrup;
	short	*m_mem;
	short	m_last_value, m_qmem[4];
	int	m_bank_status[NBANKS];
	int	m_bank_row[NBANKS];
	int	m_bank_open_time[NBANKS];
	unsigned	*m_refresh_time;
	int		m_refresh_loc, m_nrefresh;
	int	m_qloc, m_qdata[SDRAM_QSZ], m_qmask, m_wr_addr;
	int	m_clocks_till_idle;
	bool	m_next_wr;
	unsigned	m_fail;
public:
	SDRAMSIM(void) {
		m_mem = new short[(1<<24)]; // 32 MB, or 16 Mshorts

		m_refresh_time = new unsigned[(1<<13)];
		for(int i=0; i<m_nrefresh; i++)
			m_refresh_time[i] = 0;
		m_refresh_loc = 0;

		m_pwrup = 0;
		m_clocks_till_idle = 0;

		m_last_value = 0;
		m_clocks_till_idle = PWRUP_WAIT_CKS;
		m_wr_addr = 0;

		m_qloc  = 0;
		m_qmask = SDRAM_QSZ-1;

		m_next_wr = true;
		m_fail = 0;
	}

	~SDRAMSIM(void) {
		delete m_mem;
	}

	short operator()(int clk, int cke,
			int cs_n, int ras_n, int cas_n, int we_n, int bs, 
				unsigned addr,
			int driv, short data);
	int	pwrup(void) const { return m_pwrup; }
};

#endif
