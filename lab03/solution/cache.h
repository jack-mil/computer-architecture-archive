#ifndef _CACHE_H_
#define _CACHE_H_
#include <inttypes.h>

// Feel free to add any structs you deem necessary
typedef struct cache_entry {
	uint64_t addr;
	cache_entry *next;
	cache_entry *prev;

} cache_entry;

typedef struct cache {
	cache_entry *head;
	cache_entry *tail;
	int size;

	int lookups;
	int hits;

} cache;

cache *init_cache(void);
int cache_lookup(cache *c, uint64_t addr, int write);
void cache_insert(cache *c, uint64_t addr);
void cache_remove_lru(cache *c);
void updateLRU(cache *c, cache_entry *curr);
#endif