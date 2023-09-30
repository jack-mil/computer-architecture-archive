#ifndef _CACHE_H_
#define _CACHE_H_
#include <inttypes.h>

// Feel free to add any structs or functions you deem necessary

typedef struct cache {
	int lookups;
	int hits;

} cache;

// Feel free to modify the arguments of the functions
cache *init_cache(void);
int cache_lookup(cache *c, uint64_t addr, int write);
void cache_insert(cache *c, uint64_t addr);
void cache_remove_lru(cache *c);
void updateLRU(cache *c);
#endif