#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "dram.h"

// Cursed external references to sim.cpp (cmd line args)
extern uint64_t CACHE_LINE_SIZE;
extern MODE SIM_MODE;

/////////////////////////////////////////////////////////////////////////////////////
// ---------------------- DO NOT MODIFY THE PRINT STATS FUNCTION --------------------
/////////////////////////////////////////////////////////////////////////////////////
void dram_print_stats(Dram *dram) {
	double rddelay_avg = 0, wrdelay_avg = 0;
	char header[256];
	sprintf(header, "DRAM");

	if ( dram->stat_read_access ) {
		rddelay_avg = (double)(dram->stat_read_delay) / (double)(dram->stat_read_access);
	}

	if ( dram->stat_write_access ) {
		wrdelay_avg = (double)(dram->stat_write_delay) / (double)(dram->stat_write_access);
	}

	printf("\n%s_READ_ACCESS\t\t : %10lu", header, dram->stat_read_access);
	printf("\n%s_WRITE_ACCESS\t\t : %10lu", header, dram->stat_write_access);
	printf("\n%s_READ_DELAY_AVG\t\t : %10.3f", header, rddelay_avg);
	printf("\n%s_WRITE_DELAY_AVG\t\t : %10.3f", header, wrdelay_avg);
}


Dram *dram_new(uint8_t policy) {
	Dram *dram = new Dram((Page_Policy)policy);
	return dram;
}

uint64_t dram_access(Dram *dram, Addr lineaddr, bool is_dram_write) {

	uint64_t delay;

	switch ( SIM_MODE ) {
		case SIM_MODE_A:
		case SIM_MODE_B:
			delay = 100;
			break;
		case SIM_MODE_C:
		case SIM_MODE_E:
		case SIM_MODE_F:
			delay = dram_access_mode_CDE(dram, lineaddr, is_dram_write);
			break;
		default:
			assert(false);
			break;
	}

	if ( is_dram_write ) {
		dram->stat_write_access++;
		dram->stat_write_delay += delay;
	} else {
		dram->stat_read_access++;
		dram->stat_read_delay += delay;
	}
	return delay;
}

///////////////////////////////////////////////////////////////////
// Modify the function below only for Parts C,D,E
///////////////////////////////////////////////////////////////////

uint64_t dram_access_mode_CDE(Dram *dram, Addr lineaddr, bool is_dram_write) {

	// Short circuit all of this in case of closed page policy (no need to simulate)
	if ( dram->page_policy == CLOSED_PAGE ) {
		return LATENCY_BUS + LATENCY_CAS + LATENCY_RAS; // 10 + 45 + 45 = 100
	}

	// We need to simulate row buffer hit/miss for open page policy

	// lineaddr = tag+index, effectively a "cache block id"
	// To interleave, map 0 -> bank 0, 1 -> bank 1, n -> lineaddr % NUM_BANKS
	// uint8_t bank_id = lineaddr % DRAM_BANKS;
	uint bank_id = lineaddr & (DRAM_BANKS - 1);

	// uint64_t row_id = lineaddr / DRAM_BANKS;
	uint64_t row_id = lineaddr;

	uint delay = 0;
	Row_Buffer &buf = dram->banks[bank_id];

	if ( buf.valid && buf.row_id == row_id ) { // Row buffer hit
		delay = LATENCY_CAS;
	} else if ( buf.valid && buf.row_id != row_id ) { // Row buffer miss (conflict)
		// Fill this row
		// buf.valid = true;
		buf.row_id = row_id;
		delay = LATENCY_PRE + LATENCY_RAS + LATENCY_CAS;
	} else { // Row buffer is empty (invalid)
		buf.valid = true;
		buf.row_id = row_id;
		delay = LATENCY_PRE +  LATENCY_RAS + LATENCY_CAS;
	}

	// Figure out which page this lineaddr belongs to.
	return delay;
}
