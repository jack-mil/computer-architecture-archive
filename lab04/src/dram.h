#ifndef DRAM_H
#define DRAM_H

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "types.h"

//////////////////////////////////////////////////////////////////
// Define the Data structures here with the correct field (Refer to Appendix B for more details)
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
struct DRAM {
	uint64_t stat_read_access;
	uint64_t stat_write_access;
	uint64_t stat_read_delay;
	uint64_t stat_write_delay;
};

DRAM *dram_new();
void dram_print_stats(DRAM *dram);
uint64_t dram_access(DRAM *dram, Addr lineaddr, bool is_dram_write);
uint64_t dram_access_mode_CDE(DRAM *dram, Addr lineaddr, bool is_dram_write);

#endif // DRAM_H
