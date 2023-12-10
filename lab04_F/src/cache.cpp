#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <climits>
#include <cstring>

#include "cache.h"

/////////////////////////////////////////////////////////////////////////////////////
// ---------------------- DO NOT MODIFY THE PRINT STATS FUNCTION --------------------
/////////////////////////////////////////////////////////////////////////////////////

void cache_print_stats(Cache* c, char* header){
	double read_mr =0;
	double write_mr =0;

	if (c->stat_read_access) {
		read_mr = (double)(c->stat_read_miss) / (double)(c->stat_read_access);
	}

	if (c->stat_write_access) {
		write_mr = (double)(c->stat_write_miss) / (double)(c->stat_write_access);
	}

	printf("\n%s_READ_ACCESS    \t\t : %10llu", header, c->stat_read_access);
	printf("\n%s_WRITE_ACCESS   \t\t : %10llu", header, c->stat_write_access);
	printf("\n%s_READ_MISS      \t\t : %10llu", header, c->stat_read_miss);
	printf("\n%s_WRITE_MISS     \t\t : %10llu", header, c->stat_write_miss);
	printf("\n%s_READ_MISS_PERC  \t\t : %10.3f", header, 100*read_mr);
	printf("\n%s_WRITE_MISS_PERC \t\t : %10.3f", header, 100*write_mr);
	printf("\n%s_DIRTY_EVICTS   \t\t : %10llu", header, c->stat_dirty_evicts);
	printf("\n");
}

void cache_print_stats_coherence(Cache* c, char* header){
	double read_mr =0;
	double write_mr =0;

	if (c->stat_read_access) {
		read_mr = (double)(c->stat_read_miss) / (double)(c->stat_read_access);
	}

	if (c->stat_write_access) {
		write_mr = (double)(c->stat_write_miss) / (double)(c->stat_write_access);
	}

	printf("\n%s_READ_ACCESS    \t\t : %10llu", header, c->stat_read_access);
	printf("\n%s_WRITE_ACCESS   \t\t : %10llu", header, c->stat_write_access);
	printf("\n%s_READ_MISS      \t\t : %10llu", header, c->stat_read_miss);
	printf("\n%s_WRITE_MISS     \t\t : %10llu", header, c->stat_write_miss);
	printf("\n%s_READ_MISS_PERC  \t\t : %10.3f", header, 100*read_mr);
	printf("\n%s_WRITE_MISS_PERC \t\t : %10.3f", header, 100*write_mr);
	printf("\n%s_DIRTY_EVICTS   \t\t : %10llu", header, c->stat_dirty_evicts);
	printf("\n%s_INVALIDATION_MESSAGES	\t\t : %10llu",header,c->stat_num_invalidations);
	printf("\n%s_GET_X_MESSAGES		\t\t : %10llu",header,c->stat_GetX_msg);
	printf("\n%s_GET_S_MESSAGES		\t\t : %10llu",header,c->stat_GetS_msg);
	printf("\n%s_PUT_X_MESSAGES		\t\t : %10llu",header,c->stat_PutX_msg);
	printf("\n%s_CACHE_TO_CACHE_TRANSFERS	\t\t : %10llu",header,c->cache_to_cache_transfers);
	printf("\n%s_S_TO_M_TRANSITIONS \t\t : %10llu", header, c->num_s_to_m_transitions);
	printf("\n%s_I_TO_M_TRANSITIONS \t\t : %10llu", header, c->num_i_to_m_transitions);
	printf("\n%s_M_TO_S_TRANSITIONS \t\t : %10llu", header, c->num_m_to_s_transitions);
	printf("\n%s_I_TO_S_TRANSITIONS \t\t : %10llu", header, c->num_i_to_s_transitions);
	printf("\n%s_M_TO_I_TRANSITIONS \t\t : %10llu", header, c->num_m_to_i_transitions);
	printf("\n%s_S_TO_I_TRANSITIONS \t\t : %10llu", header, c->num_s_to_i_transitions);
	printf("\n");
}
/////////////////////////////////////////////////////////////////////////////////////
// Allocate memory for the data structures 
// Initialize the required fields 
/////////////////////////////////////////////////////////////////////////////////////

Cache* cache_new(uint64_t size, uint64_t assoc, uint64_t linesize, uint64_t repl_policy){

}

/////////////////////////////////////////////////////////////////////////////////////
// Return HIT if access hits in the cache, MISS otherwise 
// If is_write is TRUE, then mark the resident line as dirty
// Update appropriate stats
/////////////////////////////////////////////////////////////////////////////////////


bool cache_access(Cache* c, Addr lineaddr, uint32_t is_write, uint32_t core_id){
  

}

/////////////////////////////////////////////////////////////////////////////////////
// Install the line: determine victim using replacement policy 
// Copy victim into last_evicted_line for tracking writebacks
/////////////////////////////////////////////////////////////////////////////////////

void cache_install(Cache* c, Addr lineaddr, uint32_t is_write, uint32_t core_id){


}

////////////////////////////////////////////////////////////////////
// You may find it useful to split victim selection from install
////////////////////////////////////////////////////////////////////

uint32_t cache_find_victim(Cache *c, uint32_t set_index, uint32_t core_id){
	

}


// Cache access based on coherence, will involve the coherence state transitions.
/////////////////////////////////////////////////////////////////////////////////////
// Return HIT if access hits in the cache, MISS otherwise 
// If is_write is TRUE, then mark the resident line as dirty
// Update appropriate stats and type of messages.
// Update the coherence state based on the type of the transcation
/////////////////////////////////////////////////////////////////////////////////////


bool cache_access_coherence (Cache *c, Addr lineaddr, uint32_t is_write, uint32_t core_id)
{
	
}


/////////////////////////////////////////////////////////////////////////////////////
// Install the line: determine victim using replacement policy 
// Copy victim into last_evicted_line for tracking writebacks
// Update the coherence flag as needed.
/////////////////////////////////////////////////////////////////////////////////////

void cache_install_coherence (Cache *c, Addr lineaddr, uint32_t is_write, uint32_t core_id) { //Install a new line into the cache with a coherence state.

	
}

//////////////////////////////////////////////////////////////////////////////////////
// Obtain the Coherence state for the given cacheline.
/////////////////////////////////////////////////////////////////////////////////////
Coherence_State get_Cacheline_state (Cache* c, Addr lineaddr)
{
	// If a line is present, the saved coherence state is returned

	// Else, return INVALID.
}

////////////////////////////////////////////////////////////////////////////////////////////
// We set the cacheline flag based on the Coherence state of the next core. 
// Here, we are trying to either invalidate or make it shared based on the transaction of the core.
// The remote core's bus actions are monitored. In an ideal coherence protocol, 
// we manage the coherence messages through GetX, GetS, Inv, PutX. Instead, we monitor only the
// read or write from the remote core and change states accordingly.
/////////////////////////////////////////////////////////////////////////////////////////////
void set_Cacheline_state (Cache* c, Addr lineaddr, bool snooped_action) {


}
