#ifndef DRAM_H
#define DRAM_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "types.h"

#define MAX_DRAM_BANKS          256



//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

typedef struct DRAM DRAM;
typedef struct Rowbuf_Entry Rowbuf_Entry;


struct Rowbuf_Entry {
	bool valid; // 0 means the rowbuffer entry is invalid
	uint64_t rowid; // If the entry is valid, which row?
};


struct DRAM {
	Rowbuf_Entry perbank_row_buf[MAX_DRAM_BANKS];

	// stats 
	unsigned long long stat_read_access;
	unsigned long long stat_write_access;
	unsigned long long stat_read_delay;
	unsigned long long stat_write_delay;
};



//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

DRAM* dram_new();
void dram_print_stats(DRAM *dram);
uint64_t dram_access(DRAM* dram, Addr lineaddr, bool is_dram_write);
uint64_t dram_access_mode_CDE(DRAM* dram, Addr lineaddr, bool is_dram_write);


#endif // DRAM_H
