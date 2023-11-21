#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "dram.h"

// Cursed external references to sim.cpp (cmd line args)
extern uint64_t CACHE_LINESIZE;
extern MODE SIM_MODE;

void Dram::print_stats() {
	double rddelay_avg = 0, wrdelay_avg = 0, mr = 0;

	char header[]{"DRAM"};

	if ( m_stat_read_access ) {
		rddelay_avg = (double)(m_stat_read_delay) / (double)(m_stat_read_access);
	}

	if ( m_stat_write_access ) {
		wrdelay_avg = (double)(m_stat_write_delay) / (double)(m_stat_write_access);
	}

	if ( m_stat_buffer_miss && m_stat_read_access && m_stat_write_access ) {
		mr = (double)m_stat_buffer_miss / (double)(m_stat_read_access + m_stat_write_access);
	}

	printf("\n%s_READ_ACCESS\t\t : %10lu", header, m_stat_read_access);
	printf("\n%s_WRITE_ACCESS\t\t : %10lu", header, m_stat_write_access);
	printf("\n%s_BUFFER_MISS\t\t : %10lu", header, m_stat_buffer_miss);
	printf("\n%s_BUFFER_MISS_PERC\t\t : %10.3f", header, 100 * mr);

	printf("\n%s_READ_DELAY_AVG\t\t : %10.3f", header, rddelay_avg);
	printf("\n%s_WRITE_DELAY_AVG\t\t : %10.3f", header, wrdelay_avg);
}

uint64_t Dram::access(Addr lineaddr, bool is_dram_write) {

	uint64_t delay;

	switch ( SIM_MODE ) {
		case SIM_MODE_A:
		case SIM_MODE_B:
			delay = 100;
			break;
		case SIM_MODE_C:
		case SIM_MODE_D:
		case SIM_MODE_E:
			delay = this->access_mode_CDE(lineaddr, is_dram_write);
			break;
		default:
			assert(false);
			break;
	}

	if ( is_dram_write ) {
		m_stat_write_access++;
		m_stat_write_delay += delay;
	} else {
		m_stat_read_access++;
		m_stat_read_delay += delay;
	}
	return delay;
}

/* Parts C,D,E Simulate DRAM open/closed page policies */
uint64_t Dram::access_mode_CDE(Addr lineaddr, bool is_dram_write) {

	// Short circuit all of this in case of closed page policy (no need to simulate)
	if ( m_page_policy == CLOSED_PAGE ) {
		m_stat_buffer_miss++;                     // Every access is a buffer miss if closed page policy
		return LATENCY_BUS + LATENCY_CAS + LATENCY_RAS; // 10 + 45 + 45 = 100
	}

	// For OPEN_PAGE policy //

	// Need to simulate row buffer hit/miss

	// To interleave, bank id is bottom 4 bits
	uint bank_id = lineaddr & (DRAM_BANKS - 1); // lineaddr % DRAM_BANKS

	// Division works to right shift because parameters assumed to be powers-of-2
	// Each row buffer contains N = ROW_SIZE / CACHE_LINESIZE cache blocks
	uint64_t row_id = lineaddr / DRAM_BANKS / (ROW_BUFFER_SIZE / CACHE_LINESIZE);

	uint delay = 0;
	Row_Buffer &buf = m_banks[bank_id]; // Look at the assigned bank row buffer

	if ( buf.valid && buf.row_id == row_id ) { // Row buffer hit

		// Only pay the CAS penalty
		delay = LATENCY_CAS;

	} else if ( buf.valid && buf.row_id != row_id ) { // Row buffer miss (conflict)

		m_stat_buffer_miss++;
		// Replace this row
		buf.row_id = row_id;
		delay = LATENCY_PRE + LATENCY_RAS + LATENCY_CAS;

	} else { // Row buffer is empty (invalid)

		m_stat_buffer_miss++;
		// Bring in a new row
		buf.valid = true;
		buf.row_id = row_id;
		delay = LATENCY_RAS + LATENCY_CAS;
	}

	// Bus penalty is constant for all cases
	return delay + LATENCY_BUS;
}
