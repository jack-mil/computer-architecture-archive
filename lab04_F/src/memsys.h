#ifndef MEMSYS_H
#define MEMSYS_H

#include <stdint.h>

#include "types.h"
#include "cache.h"
#include "dram.h"

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

typedef struct Memsys Memsys;

struct Memsys {
	Cache* dcache;  // For Part A
	Cache* icache;  // For Parts A,B,C

	Cache* dcache_coreid[MAX_CORES];  // For Parts D,E, F
	Cache* icache_coreid[MAX_CORES];  // For Parts D,E, F

	Cache* l2cache; // For Parts B,C,D,E, F
	DRAM* dram;    // For Parts C,D,E

	// stats 
	unsigned long long stat_ifetch_access;
	unsigned long long stat_load_access;
	unsigned long long stat_store_access;
	uint64_t stat_ifetch_delay;
	uint64_t stat_load_delay;
	uint64_t stat_store_delay;
};



///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

Memsys* memsys_new();
void memsys_print_stats(Memsys* sys);

uint64_t memsys_access(Memsys* sys, Addr addr, Access_Type type, uint32_t core_id);
uint64_t memsys_access_modeA(Memsys* sys, Addr lineaddr, Access_Type type, uint32_t core_id);
uint64_t memsys_access_modeBC(Memsys* sys, Addr lineaddr, Access_Type type, uint32_t core_id);
uint64_t memsys_access_modeDE(Memsys* sys, Addr lineaddr, Access_Type type, uint32_t core_id);
uint64_t memsys_access_modeF(Memsys* sys, Addr lineaddr, Access_Type type, uint32_t core_id);

// For parts B,C,D,E you must use this function to access L2 
uint64_t memsys_L2_access(Memsys* sys, Addr lineaddr, bool is_writeback, uint32_t core_id);
uint64_t memsys_L2_access_multicore(Memsys* sys, Addr lineaddr, bool is_writeback, uint32_t core_id);

// This function can convert VPN to PFN. Use for parts D, E, F (ICACHE)
uint64_t memsys_convert_vpn_to_pfn(Memsys* sys, uint64_t vpn, uint32_t core_id);

// The function receives the virtual address and returns the physical address, unlike the memsys_convert_vpn_to_pfn() which takes in the VPN and returns PFN
// Use for Part F (DCACHE)
uint64_t memsys_convert_vpn_to_pfn_modeF(Memsys* sys, uint64_t v_lineaddr, uint32_t core_id);
///////////////////////////////////////////////////////////////////

#endif // MEMSYS_H
