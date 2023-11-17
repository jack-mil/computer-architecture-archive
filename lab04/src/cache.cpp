#include "cache.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdio.h>

// cursed external reference global in to sim.cpp
extern uint64_t cycle;

void Cache::print_stats(const char *header) {
	double read_mr = 0;
	double write_mr = 0;

	if ( this->m_stat_read_access ) {
		read_mr = (double)(this->m_stat_read_miss) / (double)(this->m_stat_read_access);
	}

	if ( this->m_stat_write_access ) {
		write_mr = (double)(this->m_stat_write_miss) / (double)(this->m_stat_write_access);
	}

	printf("\n%s_READ_ACCESS    \t\t : %10lu", header, this->m_stat_read_access);
	printf("\n%s_WRITE_ACCESS   \t\t : %10lu", header, this->m_stat_write_access);
	printf("\n%s_READ_MISS      \t\t : %10lu", header, this->m_stat_read_miss);
	printf("\n%s_WRITE_MISS     \t\t : %10lu", header, this->m_stat_write_miss);
	printf("\n%s_READ_MISS_PERC  \t\t : %10.3f", header, 100 * read_mr);
	printf("\n%s_WRITE_MISS_PERC \t\t : %10.3f", header, 100 * write_mr);
	// printf("\n%s_TOTAL_EVICTS   \t\t : %10lu", header, this->stat_evicts);
	printf("\n%s_DIRTY_EVICTS   \t\t : %10lu", header, this->m_stat_dirty_evicts);
	printf("\n");
}

bool Cache::access(Addr lineaddr, bool is_write, uint32_t core_id) {
	if ( is_write ) {
		m_stat_write_access++;
	} else {
		m_stat_read_access++;
	};

	// lineaddr is already tag and index portion (blocksize extracted in memsys.cpp)
	uint64_t index = lineaddr & (this->m_num_sets - 1); //(num_sets is power-of-2, so get the bitmask (e.g. 64 -> 100_0000 - 1 = 11_1111)

	// Random access into the set for this index
	auto &ways = this->m_sets.at(index);

	// Sanity check
	assert(ways.size() <= this->m_assoc);
	// Search for a cache line in ways of this set. O(n) linear to assoc (unavoidable?)
	// Uses C++11 Lambda to equate cache lines by tag (and ensure valid)
	auto it = std::find_if(ways.begin(),
	                       ways.end(),
	                       [&](const Cache_Line &line) { return (line.valid) && (line.tag == lineaddr); });

	// The tag was not found, MISS condition
	if ( it == ways.end() ) {
		// Update stats and return false
		if ( is_write ) {
			m_stat_write_miss++;
		} else {
			m_stat_read_miss++;
		};
		return MISS;
	}
	// Otherwise, was a HIT

	// Use splice to reposition the found element to the beginning of the list
	// Self organizing ensures MRU at front and LRU at back
	ways.splice(ways.begin(), ways, it);

	// Now we can access the line we just hit at front()
	// by updating state
	if ( ways.front().lfu_count < LFU_cnt_max ) {
		ways.front().lfu_count++;
	}
	// Once the line becomes dirty, it remains dirty until evicted (writeback)
	ways.front().dirty = !ways.front().dirty ? is_write : true;
	ways.front().last_access_cycle = cycle;

	return HIT;
}

Cache_Line Cache::install(Addr lineaddr, bool is_write, uint32_t core_id) {
	// lineaddr is already tag and index portion (blocksize extracted in memsys.cpp)
	// uint64_t tag = lineaddr / m_num_sets;         // Integer division shenanigans. Tag is probably not necessary, just store the whole lineaddr in ways
	uint64_t index = lineaddr & (m_num_sets - 1); //(num_sets is power-of-2, so get the bitmask (e.g. 64 -> 100_0000 - 1 = 11_1111)

	// Random access into the set for this index
	auto &set = m_sets.at(index);

	Cache_Line victim = {};
	// Conflict Miss. The ways are full
	if ( set.size() >= m_assoc ) {
		victim = this->find_victim(index, core_id); // Find victim will make space in the set
	}

	// Prep the new line
	const Cache_Line &new_line = {.tag = lineaddr, // Storing the whole tag+index here because it makes identifying victims for writeback easier
	                              .valid = true,
	                              .dirty = is_write,
	                              .core_id = core_id,
	                              .last_access_cycle = cycle,
	                              .lfu_count = 0};
	// Insert it into the most recently accessed spot
	// TODO don't allocate memory, reusing invalid spots
	set.push_front(new_line);

	// Invalid if nothing was evicted
	return victim;
}

// Given two lines a and b, return true if a is less frequently used than b
bool Cache::comp_lfu(const Cache_Line &a, const Cache_Line &b) {
	// In the case of a tie, the most recent (largest last_access_cycle) wins
	if ( a.lfu_count == b.lfu_count ) {
		return a.last_access_cycle > b.last_access_cycle;
	}
	// Otherwise, LFU wins
	return (a.lfu_count < b.lfu_count);
}

// Find a victim to remove from the set at set_index. Ensures one open way (evicts)
// Returns a copy of the Cache_Line that was evicted (for writeback)
Cache_Line Cache::find_victim(uint32_t set_index, uint32_t core_id) {

	Cache_Line victim;
	auto &set = m_sets.at(set_index);
	switch ( m_repl_policy ) {
		case LRU:
			// Save the last element in the DLL and remove it
			victim = set.back();
			set.pop_back();
			break;
		case LFU_MRU: { // new scope weirdness to prevent 'crosses initialization' error
			auto it = std::min_element(set.cbegin(), set.cend(), comp_lfu);
			victim = *it;
			set.erase(it);
		} break;
		case SWP:
			break;
		default:
			assert(false); // uh oh
			break;
	};
	m_stat_evicts++;

	// Track evictions
	if ( victim.dirty ) {
		m_stat_dirty_evicts++;
	}

	this->last_evicted = victim;
	return victim;
}
