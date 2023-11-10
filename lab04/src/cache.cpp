#include "cache.h"
#include <algorithm>
// #include <assert.h>
// #include <climits>
#include <cmath>
#include <cstring>
#include <stdio.h>
// #include <stdlib.h>

// cursed external reference global in to sim.cpp
extern uint64_t cycle;
/////////////////////////////////////////////////////////////////////////////////////
// ---------------------- DO NOT MODIFY THE PRINT STATS FUNCTION --------------------
/////////////////////////////////////////////////////////////////////////////////////

void cache_print_stats(Cache *c, char *header) {
	double read_mr = 0;
	double write_mr = 0;

	if ( c->stat_read_access ) {
		read_mr = (double)(c->stat_read_miss) / (double)(c->stat_read_access);
	}

	if ( c->stat_write_access ) {
		write_mr = (double)(c->stat_write_miss) / (double)(c->stat_write_access);
	}

	printf("\n%s_READ_ACCESS    \t\t : %10lu", header, c->stat_read_access);
	printf("\n%s_WRITE_ACCESS   \t\t : %10lu", header, c->stat_write_access);
	printf("\n%s_READ_MISS      \t\t : %10lu", header, c->stat_read_miss);
	printf("\n%s_WRITE_MISS     \t\t : %10lu", header, c->stat_write_miss);
	printf("\n%s_READ_MISS_PERC  \t\t : %10.3f", header, 100 * read_mr);
	printf("\n%s_WRITE_MISS_PERC \t\t : %10.3f", header, 100 * write_mr);
	printf("\n%s_DIRTY_EVICTS   \t\t : %10lu", header, c->stat_dirty_evicts);
	printf("\n");
}

/////////////////////////////////////////////////////////////////////////////////////
// Allocate memory for the data structures
// Initialize the required fields
/////////////////////////////////////////////////////////////////////////////////////

Cache *cache_new(uint64_t size, uint64_t assoc, uint64_t line_size, uint64_t repl_policy) {

	// The number of sets required for this size and associativity
	uint64_t num_sets = (size / line_size) / assoc;

	assert(assoc <= MAX_WAYS);

	return new Cache(num_sets, assoc, line_size, (Repl_Policy)repl_policy);
}

/////////////////////////////////////////////////////////////////////////////////////
// Return HIT if access hits in the cache, MISS otherwise
// If is_write is TRUE, then mark the resident line as dirty
// Update appropriate stats
/////////////////////////////////////////////////////////////////////////////////////

bool cache_access(Cache *c, Addr lineaddr, bool is_write, uint32_t core_id) {
	return c->access(lineaddr, is_write, core_id);
};

bool Cache::access(Addr lineaddr, bool is_write, uint32_t core_id) {
	if ( is_write ) {
		stat_write_access++;
	} else {
		stat_read_access++;
	};

	// uint64_t tag_and_index = lineaddr >> (uint64_t)std::ceil(std::log2(m_block_size));
	// auto index_bits = (uint64_t)std::ceil(std::log2(num_sets));
	// uint64_t tag = tag_and_index >> index_bits;

	// lineaddr is already tag and index portion (blocksize extracted in memsys.cpp)
	uint64_t tag = lineaddr / m_num_sets;               // Integer division shenanigans. Tag is probably not necessary, just store the whole lineaddr in ways
	uint64_t index = lineaddr & (this->m_num_sets - 1); //(num_sets is power-of-2, so get the bitmask (e.g. 64 -> 100_0000 - 1 = 11_1111)

	// Random access into the set for this index
	Cache_Set &set = this->sets.at(index);

	// Sanity check
	assert(set.ways.size() <= this->m_assoc);
	// Search for a cache line in ways of this set
	// Uses C++11 Lambda to equate cache lines by tag (and ensure valid)
	auto it = std::find_if(set.ways.begin(),
	                       set.ways.end(),
	                       [&](const Cache_Line &line) { return (line.valid) && (line.tag == tag); });

	// The tag was not found, MISS condition
	if ( it == set.ways.end() ) {
		// Update stats and return false
		if ( is_write ) {
			stat_write_miss++;
		} else {
			stat_read_miss++;
		};
		return MISS;
	}
	// Otherwise, was a HIT

	// Use splice to reposition the found element to the beginning of the list
	// This self organizing ensures most recently used at front
	// and makes read/write access to this element fast
	// (still results in a walk down the LL, but better than deallocate and re-insert)
	set.ways.splice(set.ways.begin(), set.ways, it);

	// Now we can access the line we just hit at front()
	// by updating state
	if ( set.ways.front().lfu_count < LFU_cnt_max )
		set.ways.front().lfu_count++;
	set.ways.front().dirty = is_write;
	set.ways.front().last_access_cycle = cycle;

	return HIT;
}

/////////////////////////////////////////////////////////////////////////////////////
// Install the line: determine victim using replacement policy
// Copy victim into last_evicted_line for tracking writebacks
/////////////////////////////////////////////////////////////////////////////////////

void cache_install(Cache *c, Addr lineaddr, bool is_write, uint32_t core_id) {
	return c->install(lineaddr, is_write, core_id);
}

void Cache::install(Addr lineaddr, bool is_write, uint32_t core_id) {
	// lineaddr is already tag and index portion (blocksize extracted in memsys.cpp)
	uint64_t tag = lineaddr / m_num_sets;         // Integer division shenanigans. Tag is probably not necessary, just store the whole lineaddr in ways
	uint64_t index = lineaddr & (m_num_sets - 1); //(num_sets is power-of-2, so get the bitmask (e.g. 64 -> 100_0000 - 1 = 11_1111)

	// Random access into the set for this index
	Cache_Set &set = sets.at(index);

	if ( set.ways.size() >= this->m_assoc ) {
		this->find_victim(index, core_id);		// Find victim will make space in the set
	}
	
	// Prep the new line
	Cache_Line new_line = Cache_Line(tag, is_write, core_id, cycle);
	new_line.valid = true;		// Ensure it's valid (default true in constructor)
	// Insert it into the most recently accessed spot
	set.ways.push_front(new_line);

	return;
}

// You may find it useful to split victim selection from install
uint32_t Cache::find_victim(uint32_t set_index, uint32_t core_id) {

	Cache_Line victim;
	Cache_Set &set = this->sets.at(set_index);
	switch ( this->m_repl_policy ) {
		case LRU:
			// Save the last element in the DLL and remove it
			victim = set.ways.back();
			set.ways.pop_back();
			break;
		case LFU_MRU:
			break;
		case SWP:
			break;
		default:
			assert(false); // uh oh
			break;
	}

	// Track evictions
	if ( victim.dirty )
		this->stat_dirty_evicts++;

	this->last_evicted = victim;
	return 0;
}
