#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <climits>
#include <cstring>

#include "cache.h"

extern uint64_t SWP_CORE0_WAYS;
extern uint64_t cycle;


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
	//if ((COHERENCE == 1) && (strcmp(header,"L2CACHE") != 0)) //strcmp check
	//{
	//	printf("\n%s_NUM_BUS_TRANSFERS \t\t : %10llu", header, c->stat_num_bus_transfers);
	//	printf("\n%s_NUM_INVALIDATIONS \t\t : %10llu", header, c->stat_num_invalidations);
	//}
	printf("\n");
}

/////////////////////////////////////////////////////////////////////////////////////
// Allocate memory for the data structures 
// Initialize the required fields 
/////////////////////////////////////////////////////////////////////////////////////

Cache* cache_new(uint64_t size, uint64_t assoc, uint64_t linesize, uint64_t repl_policy){

	Cache *c = (Cache*)calloc(1, sizeof(Cache));
	c->num_ways = assoc;
	c->repl_policy = repl_policy;

	if (c->num_ways > MAX_WAYS) {
    	printf("Change MAX_WAYS in cache.h to support %lu ways\n", c->num_ways);
    	exit(-1);
	}

	c->num_sets = size / (linesize * assoc);
	c->sets = (Cache_Set*)calloc(c->num_sets, sizeof(Cache_Set));

	return c;
}

/////////////////////////////////////////////////////////////////////////////////////
// Return HIT if access hits in the cache, MISS otherwise 
// If is_write is TRUE, then mark the resident line as dirty
// Update appropriate stats
/////////////////////////////////////////////////////////////////////////////////////


bool cache_access(Cache* c, Addr lineaddr, uint32_t is_write, uint32_t core_id){
  
  	bool outcome = MISS;

	Addr tag_new = lineaddr;

	Cache_Set* set = &(c->sets[lineaddr % c->num_sets]);
	
	is_write ? c->stat_write_access++ : c->stat_read_access++;


	for (uint j=0; j < c->num_ways; j++){
		if (set->line[j].valid && (set->line[j].tag == tag_new)) {

			if (is_write) {
				set->line[j].dirty = true;
			}
			set->line[j].last_access_time = cycle;
			if (set->line[j].frequency == 63)
				set->line[j].frequency = 63;
			else
				set->line[j].frequency++;
			outcome = HIT;
			break;
		}
	}

	if (!outcome) {
		is_write? c->stat_write_miss++ : c->stat_read_miss++;
	}
	
	return outcome;
}

/////////////////////////////////////////////////////////////////////////////////////
// Install the line: determine victim using replacement policy 
// Copy victim into last_evicted_line for tracking writebacks
/////////////////////////////////////////////////////////////////////////////////////

void cache_install(Cache* c, Addr lineaddr, uint32_t is_write, uint32_t core_id){

	uint64_t index = lineaddr % c->num_sets;

	uint32_t victim = cache_find_victim(c, index, core_id);
	Cache_Line* victim_line = &(c->sets[index].line[victim]);
		
	if (victim_line->valid){
		c->last_evicted_line = *victim_line;
		if (victim_line->dirty) {
			c->stat_dirty_evicts++;
		}
	}
	
	victim_line->tag = lineaddr;
	victim_line->valid = true;
	victim_line->dirty = is_write;
	victim_line->core_id = core_id;
	victim_line->last_access_time = cycle;
	victim_line->frequency = 0;
}

////////////////////////////////////////////////////////////////////
// You may find it useful to split victim selection from install
////////////////////////////////////////////////////////////////////


uint32_t cache_find_victim(Cache *c, uint32_t set_index, uint32_t core_id){
	
	uint32_t victim = 0, core0_usage = 0;

	uint64_t dummy = cycle;

	uint64_t mru = 0;

	uint64_t freq = 64; //We set the maximum frequency as 64. So, we begin from the number 65

	Cache_Set* set = &(c->sets[set_index]); 

	// Prioritize invalid lines as victims
	for (uint i=0; i<c->num_ways; i++) {
		if (!(set->line[i].valid)) {
			victim = i;
			return victim;
		}
	}

  	switch (c->repl_policy) {
		
		case 0: // LRU
			for (uint i=0; i<c->num_ways; i++) {
				if (set->line[i].last_access_time < dummy) {
					assert(set->line[i].valid);
					dummy = set->line[i].last_access_time;
					victim = i;
				}
			}
			break;

		case 1: //LFU with additional MRU
			for (uint64_t i = 0; i < c->num_ways; i++)
			{
				if (set->line[i].frequency <= freq)
				{
					assert(set->line[i].valid);
					if (set->line[i].frequency == freq)
					{
						if (set->line[i].last_access_time > mru)
						{
							mru = set->line[i].last_access_time;
							freq = set->line[i].frequency;
							victim = i;
						}
					}
					else
					{
						mru = set->line[i].last_access_time;
						freq = set->line[i].frequency;
						victim = i;
					}
				}
			}
			break;
	}

	return victim;
}
