/*
 * File         : rat.cpp
 * Author       : Jackson Miller
 * Date         : 11th October 2023
 * Description  : RAT structure for out of order pipeline
 */
#include <assert.h>
#include <stdio.h>

#include "rat.h"

/* Initializes the RAT and set all entries to invalid */
RAT *RAT_init(void) {
	// Allocate space for RAT object
	RAT *t = (RAT *)calloc(1, sizeof(RAT));
	for ( int i = 0; i < MAX_ARF_REGS; i++ ) {
		// Invalidate all inital entries
		t->RAT_Entries[i].valid = false;
	}
	return t;
}

/* Print a representation of the RAT state (entry number and PR id) */
void RAT_print_state(RAT *t) {
	printf("Printing RAT \n");
	printf("Entry  Valid  prf_id\n");
	for ( int i = 0; i < MAX_ARF_REGS; i++ ) {
		printf("%5d ::  %d \t\t", i, t->RAT_Entries[i].valid);
		printf("%5d \n", (int)t->RAT_Entries[i].prf_id);
	}
	printf("\n");
}

/*------- DO NOT CHANGE THE CODE ABOVE THIS LINE -----------*/

/*
 * The RAT will return which ROB entry (PRF_ID) maps to arf_id.
 * Return -1 if the entry is invalid (not remapped).
 */
int RAT_get_remap(RAT *t, int arf_id) {

	// Error state if arf_id is out of range
	// this can occur if src is not needed
	if ( arf_id < 0 || arf_id >= MAX_ARF_REGS ) {
		return -1;
	}

	// Return the renamed ROB tag where data can be found
	if ( t->RAT_Entries[arf_id].valid ) {
		return t->RAT_Entries[arf_id].prf_id;
	}
	// If a entry is invalid, the register is not renamed,
	// and value is present in the ARF
	return -1;
}

/* For destination regs, remap ARF to PRF (ROB tag)
 * Fails silently if arf_id is out of range (-1)
 */
void RAT_set_remap(RAT *t, int arf_id, int prf_id) {
	// Error state if arf_id is out of range
	// this can occur if dst is not needed
	if ( arf_id < 0 || arf_id >= MAX_ARF_REGS ) {
		// Fail silently
		return;
	}
	t->RAT_Entries[arf_id].prf_id = prf_id;
	t->RAT_Entries[arf_id].valid = true;
}

/* On commit, invalidate (reset) RAT information for a particular entry */
void RAT_reset_entry(RAT *t, int arf_id) {
	t->RAT_Entries[arf_id].valid = false;
}

/* Flush (invalidate) all RAT entries */
void RAT_flush(RAT *t) {
	for ( int i = 0; i < MAX_ARF_REGS; i++ ) {
		t->RAT_Entries[i].valid = false;
	}
}

/***********************************************************/
