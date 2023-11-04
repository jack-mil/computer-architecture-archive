#include <assert.h>
#include <stdio.h>

#include "rob.h"

extern int32_t NUM_ROB_ENTRIES;

/////////////////////////////////////////////////////////////
// Init function initializes the ROB
/////////////////////////////////////////////////////////////

ROB *ROB_init(void) {
	int ii;
	ROB *t = (ROB *)calloc(1, sizeof(ROB));
	for ( ii = 0; ii < MAX_ROB_ENTRIES; ii++ ) {
		t->ROB_Entries[ii].valid = false;
		t->ROB_Entries[ii].ready = false;
		t->ROB_Entries[ii].exec = false;
	}
	t->head_ptr = 0;
	t->tail_ptr = 0;
	return t;
}

/////////////////////////////////////////////////////////////
// Print State
/////////////////////////////////////////////////////////////
void ROB_print_state(ROB *t) {
	int ii = 0;
	// printf("Printing ROB \n");
	printf("Entry  Inst   Valid   Ready Exec\n");
	for ( ii = 0; ii < NUM_ROB_ENTRIES; ii++ ) {
		printf("%5d ::  %d\t", ii, (int)t->ROB_Entries[ii].inst.inst_num);
		printf(" %5d\t", t->ROB_Entries[ii].valid);
		printf(" %5d\t", t->ROB_Entries[ii].ready);
		printf(" %5d\n", t->ROB_Entries[ii].exec);
	}
	printf("\n");
}

/////////////////////////////////////////////////////////////
// If there is space in ROB return true, else false
/////////////////////////////////////////////////////////////

bool ROB_check_space(ROB *t) {
	if ( !(t->ROB_Entries[(t->tail_ptr)].valid) )
		return true;
	else
		return false;
}

/////////////////////////////////////////////////////////////
// insert entry at tail, increment tail (do check_space first)
/////////////////////////////////////////////////////////////

int ROB_insert(ROB *t, Inst_Info inst) {
	if ( !(ROB_check_space(t)) ) return -1;

	t->ROB_Entries[t->tail_ptr].inst = inst;
	t->ROB_Entries[t->tail_ptr].valid = true;
	t->ROB_Entries[t->tail_ptr].ready = false;
	t->ROB_Entries[t->tail_ptr].exec = false;
	t->tail_ptr = ((t->tail_ptr) + 1) % NUM_ROB_ENTRIES;

	if ( (t->tail_ptr) == 0 )
		return (NUM_ROB_ENTRIES - 1);
	else
		return ((t->tail_ptr) - 1);
}

/////////////////////////////////////////////////////////////
// When an inst gets scheduled for execution, mark exec
/////////////////////////////////////////////////////////////

void ROB_mark_exec(ROB *t, Inst_Info inst) {
	for ( int i = 0; i < NUM_ROB_ENTRIES; i++ ) {
		if ( (t->ROB_Entries[i].valid) && (t->ROB_Entries[i].inst.inst_num == inst.inst_num) ) {
			if ( (t->ROB_Entries[i].inst.src1_tag == -1) && (t->ROB_Entries[i].inst.src2_tag == -1) ) t->ROB_Entries[i].exec = true;
			return;
		}
	}
}

/////////////////////////////////////////////////////////////
// Once an instruction finishes execution, mark rob entry as done
/////////////////////////////////////////////////////////////

void ROB_mark_ready(ROB *t, Inst_Info inst) {
	for ( int i = 0; i < NUM_ROB_ENTRIES; i++ ) {
		if ( (t->ROB_Entries[i].valid) && (t->ROB_Entries[i].inst.inst_num == inst.inst_num) ) {
			t->ROB_Entries[i].ready = true;
			return;
		}
	}
}

/////////////////////////////////////////////////////////////
// Find whether the prf (rob entry) is ready
/////////////////////////////////////////////////////////////

bool ROB_check_ready(ROB *t, int tag) {
	if ( (t->ROB_Entries[tag].valid) && (t->ROB_Entries[tag].ready) )
		return true;
	else
		return false;
}

/////////////////////////////////////////////////////////////
// Check if the oldest ROB entry is ready for commit
/////////////////////////////////////////////////////////////

bool ROB_check_head(ROB *t) {
	// printf("ROB HEAD: %d\n",t -> head_ptr);
	if ( (t->ROB_Entries[(t->head_ptr)].valid) && (t->ROB_Entries[(t->head_ptr)].ready) )
		return true;
	else
		return false;
}

/////////////////////////////////////////////////////////////
// For writeback of freshly ready tags, wakeup waiting inst
/////////////////////////////////////////////////////////////

void ROB_wakeup(ROB *t, int tag) {
	for ( int i = 0; i < NUM_ROB_ENTRIES; i++ ) {
		if ( t->ROB_Entries[i].valid ) {
			if ( t->ROB_Entries[i].inst.src1_tag == tag ) {
				t->ROB_Entries[i].inst.src1_ready = true;
				t->ROB_Entries[i].inst.src1_tag = -1;
			}
			if ( t->ROB_Entries[i].inst.src2_tag == tag ) {
				t->ROB_Entries[i].inst.src2_ready = true;
				t->ROB_Entries[i].inst.src2_tag = -1;
			}
		}
	}
}

/////////////////////////////////////////////////////////////
// Remove oldest entry from ROB (after ROB_check_head)
/////////////////////////////////////////////////////////////

Inst_Info ROB_remove_head(ROB *t) {
	if ( ROB_check_head(t) ) {
		t->ROB_Entries[(t->head_ptr)].valid = false;
		t->head_ptr = ((t->head_ptr) + 1) % NUM_ROB_ENTRIES;
		if ( t->head_ptr > 0 )
			return t->ROB_Entries[(t->head_ptr) - 1].inst;
		else
			return t->ROB_Entries[NUM_ROB_ENTRIES - 1].inst;
	}
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////
// Invalidate all entries in ROB
/////////////////////////////////////////////////////////////

void ROB_flush(ROB *t) {
	for ( int i = 0; i < NUM_ROB_ENTRIES; i++ ) {
		t->ROB_Entries[i].valid = false;
		t->ROB_Entries[i].ready = false;
		t->ROB_Entries[i].exec = false;
	}
	t->head_ptr = 0;
	t->tail_ptr = 0;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////