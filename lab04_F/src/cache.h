#ifndef CACHE_H
#define CACHE_H

#include <stdint.h>
#include "types.h"


//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

typedef enum Coherence_State_Enum {
	INVALID,
	SHARED,
	MODIFIED
} Coherence_State;

///////////////////////////////////////////////////////////////////////////////////
/// Define the necessary data structures here (Look at Appendix A for more details)
///////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////
// Mandatory variables required for generating the desired final reports as necessary
// Used by cache_print_stats()
//////////////////////////////////////////////////////////////////////////////////////

// stat_read_access : Number of read (lookup accesses do not count as READ accesses) accesses made to the cache
// stat_write_access : Number of write accesses made to the cache
// stat_read_miss : Number of READ requests that lead to a MISS at the respective cache.
// stat_write_miss : Number of WRITE requests that lead to a MISS at the respective cache
// stat_dirty_evicts : Count of requests to evict DIRTY lines.
// stat_GetX_msg : Number of GetX messages initiated by the cache
// stat_num_invalidations : Number of Invalidation messages initiated by the cache
// stat_GetS_msg : Number of GetS messages initiated by the cache
// stat_PutX_msg : Number of PutX messages initiated by the cache
// cache_to_cache_transfers : Number of Cache to Cache transfers initiated by the cache
// num_s_to_m_transitions : Number of transitions from SHARED to MODIFIED
// num_s_to_i_transitions : Number of transitions from SHARED to INVALID
// num_i_to_m_transitions : Number of transitions from INVALID to MODIFIED
// num_i_to_s_transitions : Number of transitions from INVALID to SHARED
// num_m_to_s_transitions : Number of transitions from MODIFIED to SHARED
// num_m_to_i_transitions : Number of transitions from MODIFIED to INVALID


/////////////////////////////////////////////////////////////////////////////////////////
// Functions to be implemented
//////////////////////////////////////////////////////////////////////////////////////////////

Cache* cache_new(uint64_t size, uint64_t assocs, uint64_t linesize, uint64_t repl_policy);
bool cache_access(Cache* c, Addr lineaddr, uint32_t is_write, uint32_t core_id);
void cache_install(Cache* c, Addr lineaddr, uint32_t is_write, uint32_t core_id);
uint32_t cache_find_victim(Cache *c, uint32_t set_index, uint32_t core_id);


// Part F
bool cache_access_coherence (Cache* c, Addr lineaddr, uint32_t is_write, uint32_t core_id);
void cache_install_coherence (Cache* c, Addr lineaddr, uint32_t is_write, uint32_t core_id);
Coherence_State get_Cacheline_state(Cache *c, Addr lineaddr);
void set_Cacheline_state(Cache *c, Addr lineaddr, bool snooped_action);

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

void cache_print_stats(Cache* c, char* header);
void cache_print_stats_coherence(Cache* c, char* header); // Part F

#endif // CACHE_H
