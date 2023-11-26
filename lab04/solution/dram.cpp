#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "dram.h"

#define ROWBUF_SIZE         1024
#define DRAM_BANKS          16

//---- Latency for Part B ------

#define DRAM_LATENCY_FIXED  100

//---- Latencies for Parts C,D,E --

#define DRAM_T_ACT         45
#define DRAM_T_CAS         45
#define DRAM_T_PRE         45
#define DRAM_T_BUS         10


extern MODE SIM_MODE;
extern uint64_t CACHE_LINESIZE;
extern bool DRAM_PAGE_POLICY;


///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

void dram_print_stats(DRAM* dram){
	double rddelay_avg=0, wrdelay_avg=0;
	char header[256];
	sprintf(header, "DRAM");

	if (dram->stat_read_access) {
		rddelay_avg = (double)(dram->stat_read_delay) / (double)(dram->stat_read_access);
	}

	if (dram->stat_write_access) {
		wrdelay_avg = (double)(dram->stat_write_delay) / (double)(dram->stat_write_access);
	}

	printf("\n%s_READ_ACCESS\t\t : %10llu", header, dram->stat_read_access);
	printf("\n%s_WRITE_ACCESS\t\t : %10llu", header, dram->stat_write_access);
	printf("\n%s_READ_DELAY_AVG\t\t : %10.3f", header, rddelay_avg);
	printf("\n%s_WRITE_DELAY_AVG\t\t : %10.3f", header, wrdelay_avg);

}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

DRAM* dram_new(){
	DRAM* dram = (DRAM*)calloc(1, sizeof (DRAM));
	assert(DRAM_BANKS <= MAX_DRAM_BANKS);
	return dram;
}

uint64_t dram_access(DRAM* dram, Addr lineaddr, bool is_dram_write){
	uint64_t delay = DRAM_LATENCY_FIXED;

	if (SIM_MODE != SIM_MODE_B) {
		delay = dram_access_mode_CDE(dram, lineaddr, is_dram_write);
	}

	// Update stats
	if (is_dram_write) {
		dram->stat_write_access++;
		dram->stat_write_delay += delay;
	}
	else {
		dram->stat_read_access++;
		dram->stat_read_delay += delay;
	}

	return delay;
}

///////////////////////////////////////////////////////////////////
// Modify the function below only for Parts C,D,E
///////////////////////////////////////////////////////////////////

uint64_t dram_access_mode_CDE(DRAM* dram, Addr lineaddr, bool is_dram_write){
	uint64_t delay = DRAM_T_CAS + DRAM_T_BUS;

	uint32_t blocks_in_row = ROWBUF_SIZE / CACHE_LINESIZE;
	
	Rowbuf_Entry* bank_used = &(dram->perbank_row_buf[lineaddr % DRAM_BANKS]);
	
	uint64_t row_no = lineaddr / (blocks_in_row * DRAM_BANKS);

	if(DRAM_PAGE_POLICY == 0){
		if (!(bank_used->valid))	{
			delay += DRAM_T_ACT;	// Empty row buffer
			bank_used->valid = true;
			bank_used->rowid = row_no;
			return delay;
		}
		else {
			if (bank_used->rowid == row_no) {                  // Row Buffer Hit
				return delay;
			}
			else {                                        // Row Buffer Conflict
				delay += DRAM_T_PRE + DRAM_T_ACT;
				bank_used->rowid = row_no;
				return delay;
			}
		}
	}
	else if (DRAM_PAGE_POLICY == 1) {     //Close Page Policy
		delay += DRAM_T_ACT;	// same as Empty row buffer
		return delay;
	}

	return delay;
}


