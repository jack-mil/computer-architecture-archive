#include "cache.h"
#include "stdlib.h"
#include <assert.h>
#include <stdio.h>

/*LRU Cache as DLL
 *Adds to head
 *Moves accessed item to head
 *LRU at tail
 */

int32_t CACHE_SIZE = 8;    // 8 blocks
int32_t BLOCK_SIZE = 64;   //
int32_t HIT_LATENCY = 2;   //
int32_t MISS_LATENCY = 20; //

/*
init_cache initializes the cache data structure and returns a pointer.
The pointer to the cache data structure is part of the pipeline struct (see pipleine.h)
init_cache is called in pipe_init.
*/

Cache *init_cache(void) {
	Cache *cache = (Cache *)calloc(1, sizeof(Cache));
	return cache;
}

/*
Whenever the cache is full, the LRU block is removed by cache_remove_lru
*/

void cache_remove_lru(Cache *c) {
}

/*
cache_insert adds an element to the cache. Be sure to add the entire block
*/

void cache_insert(Cache *c, uint64_t addr) {
}
/*
Update the LRU block on an access.
*/
void updateLRU(Cache *c) {
}

/*
This is the function used by the pipeline whenever there is a load or a store instruction.
The function only returns the access latency (since we don't have actual data)
For simplicity, we assume that a block is immediately available to be added to the cache on a miss.
If the lookup is for a write we always return a latency of 1
but also add the block to the cache if it's not there
*/

int cache_lookup(Cache *c, uint64_t addr, int write) {
	return 1;
}
