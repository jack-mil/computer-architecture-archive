#ifndef CACHE_H
#define CACHE_H

#include "types.h"
#include <assert.h>
#include <list>
#include <stdint.h>
#include <vector>

#define MAX_WAYS 16

// 2^6 - 1 6-bit saturating counter
static const uint16_t LFU_cnt_max = (1 << 6) - 1;

// 0:LRU 1:LFU+MRU 2:SWP (Part E)
enum Repl_Policy {
	LRU = 0,
	LFU_MRU = 1,
	SWP = 2,
	NUM_POLICYS
};

// A cache line holds information about the memory block it holds
struct Cache_Line {
	uint64_t tag; // Higher order address bits beyond index. Identifies the data
	bool valid;   // If line is present in the cache
	bool dirty;   // If data has been written to (present only in the cache)

	uint32_t core_id; // Identify the assigned core for this line

	uint64_t last_access_cycle; // Track for LRU/MRU replacement policy (cycle)
	uint64_t lfu_count;         // Track the number of accesses for LFU replacement policy (frequency)
};

// A cache set it made up of 1 or more ways
struct Cache_Set {
	// Ways implemented as linked list for quick insertion
	std::list<Cache_Line> ways;
};

// A cache with configurable number of sets and associativity (ways per set)
class Cache {
  public:
	// Constructor for cache. Creates vector of sets with the right number of ways
	Cache(uint32_t num_sets, uint16_t assoc, Repl_Policy policy)
	    : m_assoc(assoc),
	      m_num_sets(num_sets),
	      m_repl_policy(policy),
	      sets(num_sets){};
	bool access(Addr lineaddr, bool is_write, uint32_t core_id);
	Cache_Line install(Addr lineaddr, bool is_write, uint32_t core_id);
	Cache_Line find_victim(uint32_t set_index, uint32_t core_id);
	void print_stats(const char *header);

	// The last evicted cache block for writeback
	Cache_Line last_evicted = {};

  private:
	uint64_t stat_read_access = 0;  // Number of read (lookup accesses do not count as READ accesses) accesses made to the cache
	uint64_t stat_write_access = 0; // Number of write accesses made to the cache
	uint64_t stat_read_miss = 0;    // Number of READ requests that lead to a MISS at the respective cache.
	uint64_t stat_write_miss = 0;   // Number of WRITE requests that lead to a MISS at the respective cache
	uint64_t stat_evicts = 0;       // Count of requests to evict DIRTY lines.
	uint64_t stat_dirty_evicts = 0; // Count of requests to evict DIRTY lines.

	uint16_t m_assoc;
	uint16_t m_num_sets;
	Repl_Policy m_repl_policy;

	std::vector<std::list<Cache_Line>> sets;
};

//////////////////////////////////////////////////////////////////////////////////////
// Mandatory variables required for generating the desired final reports as necessary
// Used by cache_print_stats()
//////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
// Functions to be implemented
//////////////////////////////////////////////////////////////////////////////////////////////

Cache *cache_new(uint64_t size, uint64_t assocs, uint64_t linesize, uint64_t repl_policy);
bool cache_access(Cache *c, Addr lineaddr, bool is_write, uint32_t core_id);
Cache_Line cache_install(Cache *c, Addr lineaddr, bool is_write, uint32_t core_id);
bool comp_lfu(const Cache_Line &a, const Cache_Line &b);

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

void cache_print_stats(Cache *c, char *header);
#endif // CACHE_H
