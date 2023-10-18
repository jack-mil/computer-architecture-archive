#include "cache.h"
// #include "stdlib.h"
#include <algorithm>
#include <assert.h>
#include <stdio.h>

/*LRU Cache as DLL
 *Adds to head
 *Moves accessed item to head
 *LRU at tail
 */

uint32_t CACHE_SIZE = 8;    // 8 blocks, 512B cache
uint32_t BLOCK_SIZE = 64;   // 6 offset bits
uint32_t HIT_LATENCY = 2;   //
uint32_t MISS_LATENCY = 20; //

/*
init_cache initializes the cache data structure and returns a pointer.
The pointer to the cache data structure is part of the pipeline struct (see pipleine.h)
init_cache is called in pipe_init.
*/

Cache *init_cache(void) {
	// Cache *cache = (Cache *)calloc(1, sizeof(Cache));
	Cache *cache = new Cache();
	// cache->ways = std::list<uint64_t> (CACHE_SIZE);
	return cache;
}

/*
Whenever the cache is full, the LRU block is removed by cache_remove_lru
*/

void cache_remove_lru(Cache *c) {
	// simply drop/delete the tail element
	c->ways.pop_back();
}

/*
cache_insert adds an element to the cache. Be sure to add the entire block
*/

void cache_insert(Cache *c, uint64_t tag) {
	// If the cache is currently full, evict the LRU element
	if ( c->ways.size() >= CACHE_SIZE ) {
		cache_remove_lru(c);
	}
	// Always insert at head
	c->ways.push_front(tag);
}
/*
Update the LRU block on an access.
*/
void updateLRU(Cache *c, uint64_t tag, std::list<uint64_t>::iterator pos) {

	// Erase the element from wherever it is now
	c->ways.erase(pos);
	// And move it to the front (Most Recently Used)
	c->ways.push_front(tag);
}

/*
This is the function used by the pipeline whenever there is a load or a store instruction.
The function only returns the access latency (since we don't have actual data)
For simplicity, we assume that a block is immediately available to be added to the cache on a miss.
If the lookup is for a write we always return a latency of 1
but also add the block to the cache if it's not there
*/

int cache_lookup(Cache *c, uint64_t addr, bool write) {
	c->lookups++;

	uint64_t tag = addr >> 6; // get the upper 58 bits of the addr as block tag

	// Search the ways for this tag present
	auto it = std::find(c->ways.begin(), c->ways.end(), tag);

	if ( it == c->ways.end() ) { // MISS condition
		// insert block at the head
		cache_insert(c, tag);

		if ( write ) {
			return 1;
		} else {
			return MISS_LATENCY;
		}
	} else { // HIT condition
		c->hits++;
		// Move accessed item to head
		updateLRU(c, tag, it);
		if ( write ) {
			return 1;
		} else {
			return HIT_LATENCY;
		}
	}
}
