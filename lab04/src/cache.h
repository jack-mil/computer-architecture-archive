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

// A cache with configurable number of sets and associativity (ways per set)
class Cache {
  public:
	// Constructor for cache. Creates vector of sets with the right number of ways
	Cache(uint32_t num_sets, uint16_t assoc, Repl_Policy policy)
	    : m_assoc(assoc),
	      m_num_sets(num_sets),
	      m_repl_policy(policy),
	      m_sets(num_sets){};
	// Check for a hit at the specified line address
	bool access(Addr lineaddr, bool is_write, uint32_t core_id);
	// Insert a new cache line into this cache. Return the victim if it exists
	const Cache_Line &install(Addr lineaddr, bool is_write, uint32_t core_id);
	// Print information about the accesses of this object. For grading
	void print_stats(const char *header);
	// Get a pointer to a cache initalized by size instead of using the Cache() ctor
	static Cache *by_size(uint64_t size, uint64_t assoc, uint64_t line_size, uint64_t repl_policy);

  private:
	uint64_t m_stat_read_access{};  // Number of read (lookup accesses do not count as READ accesses) accesses made to the cache
	uint64_t m_stat_write_access{}; // Number of write accesses made to the cache
	uint64_t m_stat_read_miss{};    // Number of READ requests that lead to a MISS at the respective cache.
	uint64_t m_stat_write_miss{};   // Number of WRITE requests that lead to a MISS at the respective cache
	uint64_t m_stat_evicts{};       // Count of requests to evict DIRTY lines.
	uint64_t m_stat_dirty_evicts{}; // Count of requests to evict DIRTY lines.

	// The last evicted cache block for writeback
	Cache_Line m_last_evicted{};

	uint16_t m_assoc;
	uint16_t m_num_sets;
	Repl_Policy m_repl_policy;

	// A vector of sets, each with a DLL to represent cache ways.
	std::vector<std::list<Cache_Line>> m_sets;

	// Evicts a victim at index according to repl_policy. Sets m_last_evicted
	void set_victim(std::list<Cache_Line> &set, uint32_t core_id);
	void do_lfu_policy(std::list<Cache_Line> &ways);
	void do_swp_policy(std::list<Cache_Line> &ways, uint32_t core_id);
	// Static helper function for LFU+MRU policy
	static bool comp_lfu(const Cache_Line &a, const Cache_Line &b);
};

inline Cache *Cache::by_size(uint64_t size, uint64_t assoc, uint64_t line_size, uint64_t repl_policy) {
	assert(assoc <= MAX_WAYS);
	// The number of sets required for this size and associativity
	return new Cache((size / line_size) / assoc, assoc, (Repl_Policy)repl_policy);
}

#endif // CACHE_H
