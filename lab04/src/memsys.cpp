#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memsys.h"

#define PAGE_SIZE 4096

//---- Cache Latencies  ------

#define DCACHE_HIT_LATENCY 1
#define ICACHE_HIT_LATENCY 1
#define L2CACHE_HIT_LATENCY 10

extern MODE SIM_MODE;
extern uint64_t CACHE_LINESIZE;
extern uint64_t REPL_POLICY;

extern uint64_t DCACHE_SIZE;
extern uint64_t DCACHE_ASSOC;
extern uint64_t ICACHE_SIZE;
extern uint64_t ICACHE_ASSOC;
extern uint64_t L2CACHE_SIZE;
extern uint64_t L2CACHE_ASSOC;
extern uint64_t L2CACHE_REPL;
extern uint64_t NUM_CORES;

extern uint8_t DRAM_PAGE_POLICY;

// Use delete on the return value of this function (should be a constructor?)
Memsys *memsys_new(void) {
	// Memsys *sys = (Memsys *)calloc(1, sizeof(Memsys));
	Memsys *sys = new Memsys();

	switch ( SIM_MODE ) {
		case SIM_MODE_A:
			sys->dcache = cache_new(DCACHE_SIZE, DCACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
			break;

		case SIM_MODE_B:
		case SIM_MODE_C:
			sys->dcache = cache_new(DCACHE_SIZE, DCACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
			sys->icache = cache_new(ICACHE_SIZE, ICACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
			sys->l2cache = cache_new(L2CACHE_SIZE, L2CACHE_ASSOC, CACHE_LINESIZE, L2CACHE_REPL);
			sys->dram = dram_new(DRAM_PAGE_POLICY);
			break;

		case SIM_MODE_D:
		case SIM_MODE_E:
			sys->l2cache = cache_new(L2CACHE_SIZE, L2CACHE_ASSOC, CACHE_LINESIZE, L2CACHE_REPL);
			sys->dram = dram_new(DRAM_PAGE_POLICY);
			for ( uint i = 0; i < NUM_CORES; i++ ) {
				sys->dcache_coreid[i] = cache_new(DCACHE_SIZE, DCACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
				sys->icache_coreid[i] = cache_new(ICACHE_SIZE, ICACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
			}
			break;
		default:
			break;
	}
	return sys;
}

// Destructor to free all this memory. Use delete on return value of memsys_new()
Memsys::~Memsys() {
	delete dcache;
	delete icache;
	delete l2cache;
	for ( uint i = 0; i < NUM_CORES; i++ ) {
		delete dcache_coreid[i];
		delete icache_coreid[i];
	}
	delete dram;
}

////////////////////////////////////////////////////////////////////
// Return the latency of a memory operation
////////////////////////////////////////////////////////////////////

uint64_t memsys_access(Memsys *sys, Addr addr, Access_Type type, uint32_t core_id) {
	uint32_t delay = 0;

	// all cache transactions happen at line granularity, so get lineaddr
	Addr lineaddr = addr / CACHE_LINESIZE;

	switch ( SIM_MODE ) {
		case SIM_MODE_A:
			delay = memsys_access_modeA(sys, lineaddr, type, core_id);
			break;

		case SIM_MODE_B:
		case SIM_MODE_C:
			delay = memsys_access_modeBC(sys, lineaddr, type, core_id);
			break;

		case SIM_MODE_D:
		case SIM_MODE_E:
			delay = memsys_access_modeDE(sys, lineaddr, type, core_id);
			break;

		default:
			break;
	}

	// update the stats

	switch ( type ) {
		case ACCESS_TYPE_IFETCH:
			sys->stat_ifetch_access++;
			sys->stat_ifetch_delay += delay;
			break;
		case ACCESS_TYPE_LOAD:
			sys->stat_load_access++;
			sys->stat_load_delay += delay;
			break;
		case ACCESS_TYPE_STORE:
			sys->stat_store_access++;
			sys->stat_store_delay += delay;
			break;
		default:
			break;
	}

	return delay;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

void memsys_print_stats(Memsys *sys) {
	char header[256];
	sprintf(header, "MEMSYS");

	double ifetch_delay_avg = 0, load_delay_avg = 0, store_delay_avg = 0;

	if ( sys->stat_ifetch_access ) {
		ifetch_delay_avg = (double)(sys->stat_ifetch_delay) / (double)(sys->stat_ifetch_access);
	}

	if ( sys->stat_load_access ) {
		load_delay_avg = (double)(sys->stat_load_delay) / (double)(sys->stat_load_access);
	}

	if ( sys->stat_store_access ) {
		store_delay_avg = (double)(sys->stat_store_delay) / (double)(sys->stat_store_access);
	}

	printf("\n");
	printf("\n%s_IFETCH_ACCESS  \t\t : %10llu", header, sys->stat_ifetch_access);
	printf("\n%s_LOAD_ACCESS    \t\t : %10llu", header, sys->stat_load_access);
	printf("\n%s_STORE_ACCESS   \t\t : %10llu", header, sys->stat_store_access);
	printf("\n%s_IFETCH_AVGDELAY\t\t : %10.3f", header, ifetch_delay_avg);
	printf("\n%s_LOAD_AVGDELAY  \t\t : %10.3f", header, load_delay_avg);
	printf("\n%s_STORE_AVGDELAY \t\t : %10.3f", header, store_delay_avg);
	printf("\n");

	switch ( SIM_MODE ) {
		case SIM_MODE_A:
			sprintf(header, "DCACHE");
			cache_print_stats(sys->dcache, header);
			break;
		case SIM_MODE_B:
		case SIM_MODE_C:
			sprintf(header, "ICACHE");
			cache_print_stats(sys->icache, header);
			sprintf(header, "DCACHE");
			cache_print_stats(sys->dcache, header);
			sprintf(header, "L2CACHE");
			cache_print_stats(sys->l2cache, header);
			dram_print_stats(sys->dram);
			break;

		case SIM_MODE_D:
		case SIM_MODE_E:
			assert(NUM_CORES == 2); // Hardcoded
			sprintf(header, "ICACHE_0");
			cache_print_stats(sys->icache_coreid[0], header);
			sprintf(header, "DCACHE_0");
			cache_print_stats(sys->dcache_coreid[0], header);
			sprintf(header, "ICACHE_1");
			cache_print_stats(sys->icache_coreid[1], header);
			sprintf(header, "DCACHE_1");
			cache_print_stats(sys->dcache_coreid[1], header);
			sprintf(header, "L2CACHE");
			cache_print_stats(sys->l2cache, header);
			dram_print_stats(sys->dram);
			break;
		default:
			break;
	}
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

uint64_t memsys_access_modeA(Memsys *sys, Addr lineaddr, Access_Type type, uint32_t core_id) {

	// IFETCH accesses go to icache, which we don't have in part A
	bool needs_dcache_access = !(type == ACCESS_TYPE_IFETCH);

	// Stores write to the caches
	bool is_write = (type == ACCESS_TYPE_STORE);

	if ( needs_dcache_access ) {
		// Miss
		if ( !cache_access(sys->dcache, lineaddr, is_write, core_id) ) {
			// Install the new line in L1
			cache_install(sys->dcache, lineaddr, is_write, core_id);
		}
	}

	// Timing is not simulated in Part A
	return 0;
}

uint64_t memsys_access_modeBC(Memsys *sys, Addr lineaddr, Access_Type type, uint32_t core_id) {

	uint64_t delay;

	// Perform the ICACHE/ DCACHE access
	bool dcache_access = !(type == ACCESS_TYPE_IFETCH);
	bool is_write = (type == ACCESS_TYPE_STORE);

	// Check for hit in tier 1 cache (instruction or data cache)
	if ( dcache_access ) {
		if ( cache_access(sys->dcache, lineaddr, is_write, core_id) ) {
			return DCACHE_HIT_LATENCY;
		}
	} else {
		if ( cache_access(sys->icache, lineaddr, is_write, core_id) ) {
			return ICACHE_HIT_LATENCY;
		}
	} // On Hit, just return the hit latency

	// On miss, access the L2 Cache + install the new line + if needed, perform writeback

	// On L1 miss, get delay from accessing the L2 system. Read only (dirty flag set when installed below)
	delay = memsys_L2_access(sys, lineaddr, false, core_id); // "GET" the data from L2 (or DRAM)

	// L1 Cache(s) are allocate-on-miss. Install the line in the appropriate cache
	Cache_Line victim = cache_install(dcache_access ? sys->dcache : sys->icache, lineaddr, is_write, core_id);

	// Perform writeback to lower level cache IF a dirty line was evicted
	if ( victim.valid && victim.dirty ) { // short circuit
		// victim tag is actually tag+index = lineaddr of the victim
		memsys_L2_access(sys, victim.tag, true, core_id); // perform a writeback access to L2
	}

	// Miss delay is time to check for hit, plus however long lower layer took
	return delay + (dcache_access ? DCACHE_HIT_LATENCY : ICACHE_HIT_LATENCY);
}

uint64_t memsys_L2_access(Memsys *sys, Addr lineaddr, bool is_writeback, uint32_t core_id) {

	uint64_t delay = 0;

	// Check for Hit in L2 cache
	if ( cache_access(sys->l2cache, lineaddr, is_writeback, core_id) ) {
		return L2CACHE_HIT_LATENCY;
	} // On Hit, just return the hit latency

	// On L2 miss, access DRAM + install the new line + if needed, perform writeback

	// Get delay from DRAM on L2 write (when L1 writes-back)
	if ( !is_writeback ) {
		delay = dram_access(sys->dram, lineaddr, false);
	}

	// L2 Cache is allocate-on-miss. Install the line (either from DRAM or L1) and evict if necessary
	Cache_Line victim = cache_install(sys->l2cache, lineaddr, is_writeback, core_id);

	// Perform writeback to DRAM IF a dirty line was evicted
	if ( victim.valid && victim.dirty ) { // short circuit
		// victim tag is actually tag+index = lineaddr of the victim
		dram_access(sys->dram, victim.tag, true); // Perform write to DRAM
	}

	// Miss delay is time to check for hit, plus however long lower layer took
	return delay + L2CACHE_HIT_LATENCY;
}

/////////////////////////////////////////////////////////////////////
// This function converts virtual page number (VPN) to physical frame
// number (PFN).  Note, you will need additional operations to obtain
// VPN from lineaddr and to get physical lineaddr using PFN.
/////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// DO NOT MODIFY THE CODE OF THIS FUNCTIONS
////////////////////////////////////////////////////////////////////

uint64_t memsys_convert_vpn_to_pfn(Memsys *sys, uint64_t vpn, uint32_t core_id) {

	uint64_t tail = vpn & 0x000fffff;
	uint64_t head = vpn >> 20;
	uint64_t pfn = tail + (core_id << 21) + (head << 21);
	assert(NUM_CORES == 2); // Need to make this general
	return pfn;
}

uint64_t memsys_access_modeDE(Memsys *sys, Addr v_lineaddr, Access_Type type, uint32_t core_id) {

	uint64_t delay = 0;

	// Convert the lineaddr from virtual (v) to physical (p) using the
	// function memsys_convert_vpn_to_pfn(). Page size is defined to be 4 KB.
	// NOTE: VPN_to_PFN operates at page granularity and returns page addr.

	return delay;
}

uint64_t memsys_L2_access_multicore(Memsys *sys, Addr lineaddr, bool is_writeback, uint32_t core_id) {

	uint64_t delay = 0;

	// If there is a miss in the L1 Cache, access the L2Cache for the specific lineaddr.

	// If the L2Cache does not have the cache line, access the memory + install the new line + if needed, perform the writeback.

	return delay;
}
