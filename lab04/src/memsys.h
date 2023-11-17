#ifndef MEMSYS_H
#define MEMSYS_H

#include <stdint.h>

#include "cache.h"
#include "dram.h"
#include "types.h"

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

typedef struct Memsys Memsys;

struct Memsys {
	Cache *dcache; // For Part A
	Cache *icache; // For Parts A,B,C

	Cache *dcache_coreid[MAX_CORES]; // For Parts D,E
	Cache *icache_coreid[MAX_CORES]; // For Parts D,E

	Cache *l2cache; // For Parts A,B,C,D,E
	Dram *dram;     // For Parts C,D,E

	// stats
	unsigned long long stat_ifetch_access;
	unsigned long long stat_load_access;
	unsigned long long stat_store_access;
	uint64_t stat_ifetch_delay;
	uint64_t stat_load_delay;
	uint64_t stat_store_delay;
	~Memsys();
};

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

Memsys *memsys_new();
void memsys_print_stats(Memsys *sys);

uint64_t memsys_access(Memsys *sys, Addr addr, Access_Type type, uint32_t core_id);
uint64_t memsys_access_modeA(Memsys *sys, Addr lineaddr, Access_Type type, uint32_t core_id);
uint64_t memsys_access_modeBC(Memsys *sys, Addr lineaddr, Access_Type type, uint32_t core_id);
uint64_t memsys_access_modeDE(Memsys *sys, Addr lineaddr, Access_Type type, uint32_t core_id);

// For parts B,C,D,E you must use this function to access L2
uint64_t memsys_L2_access(Memsys *sys, Addr lineaddr, bool is_writeback, uint32_t core_id);
uint64_t memsys_L2_access_multicore(Memsys *sys, Addr lineaddr, bool is_writeback, uint32_t core_id);

// This function can convert VPN to PFN
uint64_t memsys_convert_vpn_to_pfn(Memsys *sys, uint64_t vpn, uint32_t core_id);
///////////////////////////////////////////////////////////////////

#endif // MEMSYS_H
