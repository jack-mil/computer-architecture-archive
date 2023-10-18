#ifndef _CACHE_H_
#define _CACHE_H_
#include <inttypes.h>
#include <list>

// Feel free to add any structs or functions you deem necessary

typedef struct Cache {
	int lookups;
	int hits;
	std::list<uint64_t> ways;

} Cache;

// Feel free to modify the arguments of the functions
Cache *init_cache(void);
int cache_lookup(Cache *c, uint64_t addr, bool write);
void cache_insert(Cache *c, uint64_t addr);
void cache_remove_lru(Cache *c);
void updateLRU(Cache *c, uint64_t tag, std::list<uint64_t>::iterator pos);
#endif