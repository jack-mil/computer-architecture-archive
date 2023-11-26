#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "memsys.h"

#define PAGE_SIZE 4096

//---- Cache Latencies  ------

#define DCACHE_HIT_LATENCY   1
#define ICACHE_HIT_LATENCY   1
#define L2CACHE_HIT_LATENCY  10

extern MODE   SIM_MODE;
extern uint64_t  CACHE_LINESIZE;
extern uint64_t  REPL_POLICY;

extern uint64_t  DCACHE_SIZE;
extern uint64_t  DCACHE_ASSOC;
extern uint64_t  ICACHE_SIZE;
extern uint64_t  ICACHE_ASSOC;
extern uint64_t  L2CACHE_SIZE;
extern uint64_t  L2CACHE_ASSOC;
extern uint64_t  L2CACHE_REPL;
extern uint64_t  NUM_CORES;

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////


Memsys* memsys_new(void){
	Memsys* sys = (Memsys*)calloc(1, sizeof (Memsys));

	switch(SIM_MODE) {
		case SIM_MODE_A:
			sys->dcache = cache_new(DCACHE_SIZE, DCACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
			break;  

		case SIM_MODE_B:
		case SIM_MODE_C:
			sys->dcache = cache_new(DCACHE_SIZE, DCACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
			sys->icache = cache_new(ICACHE_SIZE, ICACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
			sys->l2cache = cache_new(L2CACHE_SIZE, L2CACHE_ASSOC, CACHE_LINESIZE, L2CACHE_REPL);
			sys->dram = dram_new();
			break;

		case SIM_MODE_D:
		case SIM_MODE_E:
			sys->l2cache = cache_new(L2CACHE_SIZE, L2CACHE_ASSOC, CACHE_LINESIZE, L2CACHE_REPL);
			sys->dram = dram_new();
			for (uint i=0; i<NUM_CORES; i++) {
				sys->dcache_coreid[i] = cache_new(DCACHE_SIZE, DCACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
				sys->icache_coreid[i] = cache_new(ICACHE_SIZE, ICACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
			}
			break;
		case SIM_MODE_F:
			sys->l2cache = cache_new(L2CACHE_SIZE, L2CACHE_ASSOC, CACHE_LINESIZE, L2CACHE_REPL);
			sys->dram = dram_new();
			for (uint i = 0; i < NUM_CORES; i++) {
				sys->dcache_coreid[i] = cache_new(DCACHE_SIZE, DCACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
				sys->icache_coreid[i] = cache_new(ICACHE_SIZE, ICACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
			}
			break;
		default:
			break;
	}
	return sys;
}


////////////////////////////////////////////////////////////////////
// Return the latency of a memory operation
////////////////////////////////////////////////////////////////////

uint64_t memsys_access(Memsys* sys, Addr addr, Access_Type type, uint32_t core_id){
	uint32_t delay = 0;


	// all cache transactions happen at line granularity, so get lineaddr
	Addr lineaddr = addr / CACHE_LINESIZE;

	switch (SIM_MODE) {
		case SIM_MODE_A:
			delay = memsys_access_modeA(sys,lineaddr,type, core_id);
			break;
		case SIM_MODE_B:
		case SIM_MODE_C:
			delay = memsys_access_modeBC(sys,lineaddr,type, core_id);
			break;

		case SIM_MODE_D:
		case SIM_MODE_E:
			delay = memsys_access_modeDE(sys,lineaddr,type, core_id);
			break;
		case SIM_MODE_F:
			delay = memsys_access_modeF(sys,lineaddr,type,core_id);
			break;
		default:
			break;
	}

  	//update the stats
  
	switch (type) {
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

void memsys_print_stats(Memsys* sys){
	char header[256];
	sprintf(header, "MEMSYS");

	double ifetch_delay_avg=0, load_delay_avg=0, store_delay_avg=0;

	if (sys->stat_ifetch_access) {
		ifetch_delay_avg = (double)(sys->stat_ifetch_delay) / (double)(sys->stat_ifetch_access);
	}

	if (sys->stat_load_access) {
		load_delay_avg = (double)(sys->stat_load_delay) / (double)(sys->stat_load_access);
	}

	if (sys->stat_store_access) {
		store_delay_avg = (double)(sys->stat_store_delay) / (double)(sys->stat_store_access);
	}


	printf("\n");
	printf("\n%s_IFETCH_ACCESS  \t\t : %10llu",  header, sys->stat_ifetch_access);
	printf("\n%s_LOAD_ACCESS    \t\t : %10llu",  header, sys->stat_load_access);
	printf("\n%s_STORE_ACCESS   \t\t : %10llu",  header, sys->stat_store_access);
	printf("\n%s_IFETCH_AVGDELAY\t\t : %10.3f",  header, ifetch_delay_avg);
	printf("\n%s_LOAD_AVGDELAY  \t\t : %10.3f",  header, load_delay_avg);
	printf("\n%s_STORE_AVGDELAY \t\t : %10.3f",  header, store_delay_avg);
	printf("\n");

	switch (SIM_MODE) {
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
			assert(NUM_CORES==2); //Hardcoded
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
		case SIM_MODE_F:
			assert(NUM_CORES==2); //Hardcoded
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

uint64_t memsys_access_modeA(Memsys* sys, Addr lineaddr, Access_Type type, uint32_t core_id){
  
	// IFETCH accesses go to icache, which we don't have in part A
	bool needs_dcache_access = !(type == ACCESS_TYPE_IFETCH);

	// Stores write to the caches
	bool is_write = (type == ACCESS_TYPE_STORE);

	if (needs_dcache_access) {
		// Miss
		if (!cache_access(sys->dcache,lineaddr,is_write,core_id)) {
			// Install the new line in L1
			cache_install(sys->dcache,lineaddr,is_write,core_id);
		}
	}

	// Timing is not simulated in Part A
	return 0;
}

typedef uint64_t uns64;
typedef uint32_t uns;
typedef bool Flag;
#define TRUE 1
#define FALSE 0

uint64_t memsys_access_modeBC(Memsys* sys, Addr lineaddr, Access_Type type, uint32_t core_id){

	uint64_t delay;

	if (type == ACCESS_TYPE_IFETCH) {
		delay = ICACHE_HIT_LATENCY;
		// Icache miss
		if (!cache_access(sys->icache,lineaddr,false,core_id)) {
			// Access L2
			delay += memsys_L2_access(sys,lineaddr,false,core_id);
			// Install the new line in L1 icache
			cache_install(sys->icache,lineaddr,false,core_id);
		}
	}
	else {
		delay = DCACHE_HIT_LATENCY;
		bool is_write = (type == ACCESS_TYPE_STORE);
		// Dcache miss
		if (!cache_access(sys->dcache,lineaddr,is_write,core_id)) {
			// Access L2
			delay += memsys_L2_access(sys,lineaddr,false,core_id);
			// Install the new line in L1 dcache
			cache_install(sys->dcache,lineaddr,is_write,core_id);
			// Access L2 to writeback potentially evicted line
			memsys_L2_access(sys,sys->dcache->last_evicted_line.tag,true,core_id);
		}
	}

	return delay;
}

uint64_t memsys_L2_access(Memsys* sys, Addr lineaddr, bool is_writeback, uint32_t core_id){

	uint64_t delay = L2CACHE_HIT_LATENCY;

	// Read access in the L2
	if (!is_writeback) {
		// Miss
		if (!cache_access(sys->l2cache,lineaddr,false,core_id)) {
			// Access memory
			delay += dram_access(sys->dram,lineaddr,false);

			// Write the line in L2
			cache_install(sys->l2cache,lineaddr,false,core_id);
		}
	}
	// Writeback from L1
	else {
		// If the evicted line from L1 is valid+dirty, write it in L2
		if (sys->dcache->last_evicted_line.valid && sys->dcache->last_evicted_line.dirty) {
			if (!cache_access(sys->l2cache,lineaddr,true,core_id)) {
				cache_install(sys->l2cache,lineaddr,true,core_id);
			}
		}
	}

	// If cache_install() evicted a dirty line from L2, write it back to memory
	if (sys->l2cache->last_evicted_line.valid && sys->l2cache->last_evicted_line.dirty) {
		dram_access(sys->dram,sys->l2cache->last_evicted_line.tag,true);
	}
	sys->dcache->last_evicted_line.valid = false;
	sys->l2cache->last_evicted_line.valid = false;

	return delay;
}



/////////////////////////////////////////////////////////////////////
// This function converts virtual page number (VPN) to physical frame
// number (PFN).  Note, you will need additional operations to obtain
// VPN from lineaddr and to get physical lineaddr using PFN.
/////////////////////////////////////////////////////////////////////

uint64_t memsys_convert_vpn_to_pfn(Memsys *sys, uint64_t vpn, uint32_t core_id){


	uint64_t tail = vpn & 0x000fffff;
	uint64_t head = vpn >> 20;
	uint64_t pfn  = tail + (core_id << 21) + (head << 21);
	assert(NUM_CORES==2); //Need to make this general
	return pfn;
}

uint64_t memsys_access_modeDE(Memsys* sys, Addr v_lineaddr, Access_Type type, uint32_t core_id){
	uint64_t delay;
	
	return delay;
}


uint64_t memsys_L2_access_multicore(Memsys* sys, Addr lineaddr, bool is_writeback, uint32_t core_id){

	uint64_t delay

	return delay;
}
