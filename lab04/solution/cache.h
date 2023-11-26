#ifndef CACHE_H
#define CACHE_H

#include <stdint.h>
#include "types.h"

#define MAX_WAYS 16

typedef struct Cache_Line Cache_Line;
typedef struct Cache_Set Cache_Set;
typedef struct Cache Cache;

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
struct Cache_Line {
	bool valid;
	bool dirty;
	Addr tag;
	uint32_t core_id;
	uint32_t last_access_time; // for LRU,MRU
	uint32_t frequency;
// Note: No data as we are only estimating hit/miss 
};


struct Cache_Set {
	Cache_Line line[MAX_WAYS];
};


struct Cache{
	uint64_t num_sets;
	uint64_t num_ways;
	uint64_t repl_policy;

	Cache_Set *sets;
	Cache_Line last_evicted_line; // for checking writebacks

	//stats
	unsigned long long stat_read_access; 
	unsigned long long stat_write_access; 
	unsigned long long stat_read_miss; 
	unsigned long long stat_write_miss; 
	unsigned long long stat_dirty_evicts; // how many dirty lines were evicted?
};


/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

Cache* cache_new(uint64_t size, uint64_t assocs, uint64_t linesize, uint64_t repl_policy);
bool cache_access(Cache* c, Addr lineaddr, uint32_t is_write, uint32_t core_id);
void cache_install(Cache* c, Addr lineaddr, uint32_t is_write, uint32_t core_id);
uint32_t cache_find_victim(Cache *c, uint32_t set_index, uint32_t core_id);

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

void cache_print_stats(Cache* c, char* header);

#endif // CACHE_H
