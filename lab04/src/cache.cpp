#include "cache.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdio.h>

// cursed external reference global in to sim.cpp
extern uint64_t cycle;
// The number of ways assigned to core 0 (rest to core 1)
extern uint64_t SWP_CORE0_WAYS;

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
	// printf("\n%s_TOTAL_EVICTS   \t\t : %10lu", header, this->m_stat_evicts);
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
	auto iter_match = std::find_if(ways.begin(),
	                               ways.end(),
	                               [&](const Cache_Line &line) { return (line.valid) && (line.tag == lineaddr); });

	// The tag was not found, MISS condition
	if ( iter_match == ways.end() ) {
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
	ways.splice(ways.begin(), ways, iter_match);

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

const Cache_Line &Cache::install(Addr lineaddr, bool is_write, uint32_t core_id) {
	// lineaddr is already tag and index portion (blocksize extracted in memsys.cpp)
	uint64_t index = lineaddr & (m_num_sets - 1);
	// (num_sets is power-of-2, so get the bitmask (e.g. 64 -> 100_0000 - 1 = 11_1111)

	// Random access into the set for this index
	auto &ways = m_sets.at(index);

	// Conflict Miss. The ways are full
	// Find a victim to evict
	if ( ways.size() >= m_assoc ) {
		this->set_victim(ways, core_id); // populates m_last_evicted
	}

	ways.push_front({.tag = lineaddr, // Storing the whole tag+index here because it makes identifying victims for writeback easier
	                 .valid = true,
	                 .dirty = is_write,
	                 .core_id = core_id,
	                 .last_access_cycle = cycle,
	                 .lfu_count = 0});

	// Invalid if nothing was evicted
	return m_last_evicted;
}

// Find a victim to remove from the given ways.
// After execution, ensures ways size is decreased by 1
// Sets the last_evicted class member
void Cache::set_victim(std::list<Cache_Line> &ways, uint32_t core_id) {

	switch ( m_repl_policy ) {
		case LRU:
			// Save the last element in the DLL and remove it
			m_last_evicted = ways.back();
			ways.pop_back();
			break;
		case LFU_MRU:
			this->do_lfu_policy(ways);
			break;
		case SWP:
			this->do_swp_policy(ways, core_id);
			break;
		default:
			assert(false); // uh oh
			break;
	};

	m_stat_evicts++;

	// Track evictions
	if ( m_last_evicted.dirty ) {
		m_stat_dirty_evicts++;
	}
}

void Cache::do_lfu_policy(std::list<Cache_Line> &ways) {
	auto iter = std::min_element(ways.cbegin(), ways.cend(), Cache::comp_lfu);
	m_last_evicted = *iter; // Copy this element into member var
	ways.erase(iter);       // This deallocates the element
}

void Cache::do_swp_policy(std::list<Cache_Line> &ways, uint32_t core_id) {

	uint64_t SWP_CORE1_WAYS = m_assoc - SWP_CORE0_WAYS;

	// count the number of lines matching this access core_id
	uint count = std::count_if(ways.begin(), ways.end(),
	                           [&](const Cache_Line &line) { return line.core_id == core_id; });

	// this implementation only works for 2 cores (0,1)
	uint filter = 0;
	if ( count >= (core_id ? SWP_CORE1_WAYS : SWP_CORE0_WAYS) ) {
		// Only select lines matching this core IF quota has been reached
		filter = core_id;
	} else {
		// If this core deserves more space, evict from the other core's space
		filter = core_id ? 1 : 0;
	}

	// Using a lambda again to capture environment (filter var)
	auto it = std::min_element(ways.cbegin(), ways.cend(), [&](const Cache_Line &a, const Cache_Line &b) {
		// Perform normal LFU+MRU if both elements match filter
		if ( (a.core_id == filter) && (b.core_id == filter) )
			return Cache::comp_lfu(a, b);

		// Otherwise, prioritize element that matches filter
		return (a.core_id == filter);

		// Case when neither matches filter wont occur due to other logic
	});

	m_last_evicted = *it;
	ways.erase(it);
}

// Given two lines a and b, return true if a is less frequently used than b
// This could be an operator of Cache_Line. But that might be less readable
bool Cache::comp_lfu(const Cache_Line &a, const Cache_Line &b) {
	// In the case of a tie, the most recent (largest last_access_cycle) wins
	if ( a.lfu_count == b.lfu_count ) {
		return a.last_access_cycle > b.last_access_cycle;
	}
	// Otherwise, LFU wins
	return (a.lfu_count < b.lfu_count);
}
