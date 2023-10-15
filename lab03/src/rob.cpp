/*
 * File         : rob.cpp
 * Author       : Jackson Miller
 * Date         : 11th October 2023
 * Description  : ROB structure for out of order pipeline
 */
#include <assert.h>
#include <stdio.h>

#include "rob.h"

extern int32_t NUM_ROB_ENTRIES;

/* Initializes the ROB and set all entries to invalid */
ROB *ROB_init(void) {
	ROB *t = (ROB *)calloc(1, sizeof(ROB));
	for ( int i = 0; i < MAX_ROB_ENTRIES; i++ ) {
		t->ROB_Entries[i].valid = false;
		t->ROB_Entries[i].ready = false;
		t->ROB_Entries[i].exec = false;
	}
	t->head_ptr = 0;
	t->tail_ptr = 0;
	return t;
}

/* Print a representation of the ROB state (entry number and PR id) */
void ROB_print_state(ROB *t) {
	int ii = 0;
	printf("Printing ROB \n");
	printf("Entry  Inst   Valid   Ready Exec\n");
	for ( ii = 0; ii < NUM_ROB_ENTRIES; ii++ ) {
		printf("%5d ::  %d\t", ii, (int)t->ROB_Entries[ii].inst.inst_num);
		printf(" %5d\t", t->ROB_Entries[ii].valid);
		printf(" %5d\t", t->ROB_Entries[ii].ready);
		printf(" %5d\n", t->ROB_Entries[ii].exec);
	}
	printf("\n");
}

/* If there is space in ROB return true, else false */
bool ROB_check_space(ROB *t) {

	// The ROB is full when tail == head, and that entry is valid.
	// because tail is always incremented modulo NUM_ROB_ENTRIES,
	// there is space unless tail points to an valid entry
	if ( t->ROB_Entries[t->tail_ptr].valid ) {
		return false;
	} else {
		return true;
	}
}

/* Add a new entry at tail, increment tail. Fails if ROB is full.
 * Use ROB_check_space first
 */
int ROB_insert(ROB *t, Inst_Info inst) {
	// This should never be called on a full ROB
	assert(!t->ROB_Entries[t->tail_ptr].valid);

	// Save the tag this inst is inserted at for return
	int rob_tag = t->tail_ptr;
	t->ROB_Entries[rob_tag].inst = inst;
	t->ROB_Entries[rob_tag].valid = true;
	t->ROB_Entries[rob_tag].exec = false;
	t->ROB_Entries[rob_tag].ready = false;

	// Increment tail ptr circularly
	t->tail_ptr = (t->tail_ptr + 1) % NUM_ROB_ENTRIES;

	return rob_tag;
}

/////////////////////////////////////////////////////////////
// When an inst gets scheduled for execution, mark exec
/////////////////////////////////////////////////////////////

void ROB_mark_exec(ROB *t, Inst_Info inst) {
	// Need to search for the entry? Why not
	for ( int i = 0; i < NUM_ROB_ENTRIES; i++ ) {
		if ( t->ROB_Entries[i].valid && t->ROB_Entries[i].inst.inst_num == inst.inst_num ) {
			t->ROB_Entries[i].exec = true;
			return; // OK to return now, should only be one result
		}
	}
}

/* Search the ROB for given instruction, and mark it ready for commit
 * Broadcast completion to other entries waiting on this tag
 */
void ROB_mark_ready(ROB *t, Inst_Info inst) {
	// We have to search the table,
	// because entries can finish out of order
	for ( int tag = 0; tag < NUM_ROB_ENTRIES; tag++ ) {
		// Skip if invalid
		if ( !t->ROB_Entries[tag].valid )
			continue;
		// Use inst_num to uniquely identify instructions in the ROB (?)
		if ( t->ROB_Entries[tag].inst.inst_num == inst.inst_num ) {
			t->ROB_Entries[tag].ready = true;
			// Broadcast to other instructions that this one is ready
			ROB_wakeup(t, tag);
			// Quit searching now (should only be one result)
			return;
		}
	}
}

/////////////////////////////////////////////////////////////
// Find whether the prf (rob entry) is ready

bool ROB_check_ready(ROB *t, int tag) {
	return t->ROB_Entries[tag].ready;
}

/* Check if the oldest ROB entry is ready for commit
 * (valid and ready bits set)
 */
bool ROB_check_head(ROB *t) {
	if ( t->ROB_Entries[t->head_ptr].valid ) {
		return t->ROB_Entries[t->head_ptr].ready;
	}
	// Should never happen
	return false;
}

/* For writeback of freshly ready tags, broadcast completion to waiting inst */
void ROB_wakeup(ROB *t, int tag) {
	Inst_Info &this_inst = t->ROB_Entries[tag].inst;
	assert(this_inst.dr_tag == tag); // sanity check
	// Search the ROB for other entries waiting on this data
	for ( int i = 0; i < NUM_ROB_ENTRIES; i++ ) {
		// Skip invalid entry
		if ( !t->ROB_Entries[i].valid )
			continue;

		// Set src1/2 ready if the destination of completing instr
		// matches one of the sources (renamed tags)
		if ( this_inst.dr_tag == t->ROB_Entries[i].inst.src1_tag ) {
			t->ROB_Entries[i].inst.src1_ready = true;
		}

		if ( this_inst.dr_tag == t->ROB_Entries[i].inst.src2_tag ) {
			t->ROB_Entries[i].inst.src2_ready = true;
		}
	}
}

/* Remove oldest entry from ROB (after ROB_check_head) */
Inst_Info ROB_remove_head(ROB *t) {
	assert(t->ROB_Entries[t->head_ptr].valid);
	assert(t->ROB_Entries[t->head_ptr].ready);

	// Save the instruction for return
	Inst_Info commit_inst = t->ROB_Entries[t->head_ptr].inst;

	// Remove (invalidate) the head entry and increment
	t->ROB_Entries[t->head_ptr].valid = false;
	t->head_ptr = (t->head_ptr + 1) % NUM_ROB_ENTRIES;

	return commit_inst;
}

/* Invalidate and reset all entries in ROB */
void ROB_flush(ROB *t) {
	t->head_ptr = 0;
	t->tail_ptr = 0;
	for ( int i = 0; i < MAX_ROB_ENTRIES; i++ ) {
		t->ROB_Entries[i].valid = false;
		t->ROB_Entries[i].ready = false;
		t->ROB_Entries[i].exec = false;
	}
}
