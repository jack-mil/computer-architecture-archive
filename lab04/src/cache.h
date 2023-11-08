#ifndef CACHE_H
#define CACHE_H

#include <stdint.h>
#include "types.h"
#include <list>
#include <vector>
#include <assert.h>

#define MAX_WAYS 16

// 0:LRU 1:LFU+MRU 2:SWP (Part E)
enum Repl_Policy {
	LRU = 0,
	LFU_MRU = 1,
	SWP = 2,
	NUM_POLICYS
};

// A cache line holds information about the memory block it holds
struct Cache_Line {
	uint32_t tag;       // Higher order address bits beyond index. Identifies the data
	bool valid = false; // If line is present in the cache
	bool dirty;         // If data has been written to (present only in the cache)

	uint32_t core_id; // Identify the assigned core for this line

	uint64_t last_access_cycle; // Track for LRU/MRU replacement policy (cycle)
	uint64_t accesses;                    // Track the number of accesses for LFU replacement policy (frequency)
};

// A cache set it made up of 1 or more ways
struct Cache_Set {
	uint16_t num_ways;
	// Ways implemented as linked list for quick insertion
	std::list<Cache_Line> ways;

	// Constructor to initialize correct number of ways
	Cache_Set(uint16_t ways) : ways(0) {
		num_ways = ways;
		assert(num_ways <= MAX_WAYS);
	};
};

// A cache with configurable number of sets and associativity (ways per set)
class Cache {
  public:
	uint64_t stat_read_access;  // Number of read (lookup accesses do not count as READ accesses) accesses made to the cache
	uint64_t stat_write_access; // Number of write accesses made to the cache
	uint64_t stat_read_miss;    // Number of READ requests that lead to a MISS at the respective cache.
	uint64_t stat_write_miss;   // Number of WRITE requests that lead to a MISS at the respective cache
	uint64_t stat_dirty_evicts; // Count of requests to evict DIRTY lines.

	uint64_t block_size;
	uint16_t assoc;
	uint16_t num_sets;
	Repl_Policy repl_policy;

	Cache_Line last_evicted;

	std::vector<Cache_Set> sets;

	// Constructor for cache. Creates vector of sets with the right number of ways
	Cache(uint32_t num_sets, uint16_t assoc, uint64_t block_size, Repl_Policy policy)
	    : sets(num_sets, Cache_Set{assoc}) {
		assoc = assoc;
		repl_policy = policy;
		num_sets = num_sets;
		block_size = block_size;
	};

	bool access(Addr lineaddr, uint32_t is_write, uint32_t core_id);
};



//////////////////////////////////////////////////////////////////////////////////////
// Mandatory variables required for generating the desired final reports as necessary
// Used by cache_print_stats()
//////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
// Functions to be implemented
//////////////////////////////////////////////////////////////////////////////////////////////

Cache *cache_new(uint64_t size, uint64_t assocs, uint64_t linesize, uint64_t repl_policy);
bool cache_access(Cache *c, Addr lineaddr, uint32_t is_write, uint32_t core_id);
void cache_install(Cache *c, Addr lineaddr, uint32_t is_write, uint32_t core_id);
uint32_t cache_find_victim(Cache *c, uint32_t set_index, uint32_t core_id);

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

void cache_print_stats(Cache *c, char *header);

#endif // CACHE_H
