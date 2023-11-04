#include "cache.h"
#include <stdio.h>
#include <assert.h>
#include "stdlib.h"

/*LRU Cache as DLL
*Adds to head
*Moves accessed item to head
*LRU at tail
*/

int32_t   CACHE_SIZE=8;//8 blocks
int32_t   BLOCK_SIZE=64;//
int32_t   HIT_LATENCY=2;//
int32_t   MISS_LATENCY=20;//

/*
init_cache initializes the cache data structure and returns a pointer.
The pointer to the cache data structure is part of the pripleine struct (see pipleine.h)
init_cache is called in pipe_init. 
*/


cache* init_cache(void){
    cache* c = (cache*)malloc(sizeof(cache));
    c->head = NULL;
    c->tail =NULL;
    c->size = 0;
    c->lookups = 0;
    c->hits = 0;
    return c;
}

/*
Whenever the cache is full, the LRU block is removed by cache_remove_lru
*/


void cache_remove_lru(cache* c){
    // remove tail
    cache_entry* temp = c->tail;
    c->tail = c->tail->prev;
    c->tail->next = NULL;
    c->size--;
    free(temp);
    return; 
}

/*
cache_insert adds an element to the cache. Be sure to add the entire block
*/

void cache_insert(cache* c,uint64_t addr){
    uint64_t block_addr = addr & 0xFFFFFFFFFFFFFFC0;
    cache_entry* e = (cache_entry*)malloc(sizeof(cache_entry));
    e->addr = block_addr;
    


    if(c->size == 0){
        e->next = NULL;
        e->prev = NULL;
        c->head = e;
        c->tail = e;
        c->size = 1;
        return ;
    }

     if(c->size == CACHE_SIZE){
        cache_remove_lru(c);
    }

    //  if(c->size == 8){
    //     cache_remove_mru(c);
    // }

    e->next = c->head;
    e->prev = NULL;
    c->head->prev = e;
    c->head = e;
    c->size++;
    return;

}
/*
Update the LRU block on an access.
*/
void updateLRU(cache* c, cache_entry* curr){
    if(curr == c->head){return;}

    
    if(curr == c->tail){
        curr->prev->next = curr->next;
        c->tail = curr->prev;
        
        c->head->prev = curr;
        curr->next = c->head;
        curr->prev = NULL;
        c->head = curr;
        return;
    }


    curr->prev->next = curr->next;
    curr->next->prev = curr->prev;
    c->head->prev = curr;
    curr->next = c->head;
    curr->prev = NULL;
    c->head = curr;

    return;
}

/*
This is the function used by the pipeline whenever there is a load or a store instruction.
The function only returns the access latency (since we don't have actual data)
For simplicity, we assume that a block is immediately available to be added to the cache on a miss.
If the lookup is for a write we always return the HIT_LATENCY 
but also add the block to the cache if it's not there
*/

int cache_lookup(cache* c, uint64_t addr,int write){
    //printf("cache size: %d\n",c->size);
    int hit =0;
    int latency =MISS_LATENCY;
    uint64_t block_addr = addr & 0xFFFFFFFFFFFFFFC0;
    cache_entry* curr = c->head;
    c->lookups++;
    while(curr != NULL){
        if(curr->addr == block_addr){
            hit = 1;
            latency =HIT_LATENCY;
            if(curr != c->head){
                
                // move curr to head
                updateLRU(c,curr);
            }
            c->hits++;
            break;
        }
        curr = curr->next;
    }

    if(!hit){
        cache_insert(c,addr);
    }

    if(write){
        return HIT_LATENCY;
    }
    

    return latency;
}
