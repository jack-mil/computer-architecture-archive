#include <assert.h>
#include <stdio.h>

#include "cache.h"
#include "exeq.h"
/////////////////////////////////////////////////////////////
// Init function initializes the EXEQ
/////////////////////////////////////////////////////////////

EXEQ *EXEQ_init(void) {
	int ii;
	EXEQ *t = (EXEQ *)calloc(1, sizeof(EXEQ));
	for ( ii = 0; ii < MAX_EXEQ_ENTRIES; ii++ ) {
		t->EXEQ_Entries[ii].valid = false;
	}
	return t;
}

/////////////////////////////////////////////////////////////
// Print State
/////////////////////////////////////////////////////////////

void EXEQ_print_state(EXEQ *t) {
	int ii = 0;
	printf("Printing EXEQ \n");
	printf("Entry  Valid  inst  Wait Cycles\n");
	for ( ii = 0; ii < MAX_EXEQ_ENTRIES; ii++ ) {
		printf("%5d ::  %d ", ii, t->EXEQ_Entries[ii].valid);
		printf("%5d \t", (int)t->EXEQ_Entries[ii].inst.inst_num);
		printf("%5d \n", t->EXEQ_Entries[ii].inst.exe_wait_cycles);
	}
	printf("\n");
}

/////////////////////////////////////////////////////////////
// Every cycle, all valid EXEQ entries undergo exe_wait--
/////////////////////////////////////////////////////////////

void EXEQ_cycle(EXEQ *t) {
	int ii;
	for ( ii = 0; ii < MAX_EXEQ_ENTRIES; ii++ ) {
		if ( t->EXEQ_Entries[ii].valid ) {
			t->EXEQ_Entries[ii].inst.exe_wait_cycles--;
		}
	}
}

/////////////////////////////////////////////////////////////
// insert entry in EXEQ, exit if no space!
/////////////////////////////////////////////////////////////

void EXEQ_insert(EXEQ *t, Inst_Info inst, Cache *c) {
	int ii;

	for ( ii = 0; ii < MAX_EXEQ_ENTRIES; ii++ ) {
		if ( !t->EXEQ_Entries[ii].valid ) {
			t->EXEQ_Entries[ii].valid = true;
			t->EXEQ_Entries[ii].inst = inst;
			t->EXEQ_Entries[ii].inst.exe_wait_cycles = 1;

			if ( CACHE ) {
				if ( t->EXEQ_Entries[ii].inst.op_type == OP_LD ) {
					t->EXEQ_Entries[ii].inst.exe_wait_cycles = cache_lookup(c, inst.mem_addr, 0);
				}

				if ( t->EXEQ_Entries[ii].inst.op_type == OP_ST ) {
					t->EXEQ_Entries[ii].inst.exe_wait_cycles = cache_lookup(c, inst.mem_addr, 1);
				}
			} else {
				if ( t->EXEQ_Entries[ii].inst.op_type == OP_LD ) {
					t->EXEQ_Entries[ii].inst.exe_wait_cycles = LOAD_EXE_CYCLES;
				}
			}

			return;
		}
	}
	printf("ERROR: Trying to install in full EXEQ. Dying...\n");
	exit(-1);
}

/////////////////////////////////////////////////////////////
// If any EXEQ entry has zero wait time return true, else false
/////////////////////////////////////////////////////////////

bool EXEQ_check_done(EXEQ *t) {
	int ii;

	for ( ii = 0; ii < MAX_EXEQ_ENTRIES; ii++ ) {
		if ( t->EXEQ_Entries[ii].valid ) {
			if ( t->EXEQ_Entries[ii].inst.exe_wait_cycles == 0 ) {
				return true;
			}
		}
	}

	return false;
}

/////////////////////////////////////////////////////////////
// Remove any finished entry from the EXEQ (call after check_done)
/////////////////////////////////////////////////////////////

Inst_Info EXEQ_remove(EXEQ *t) {
	Inst_Info retval;
	int ii;

	for ( ii = 0; ii < MAX_EXEQ_ENTRIES; ii++ ) {
		if ( t->EXEQ_Entries[ii].valid ) {
			if ( t->EXEQ_Entries[ii].inst.exe_wait_cycles == 0 ) {
				t->EXEQ_Entries[ii].valid = false;
				return t->EXEQ_Entries[ii].inst;
			}
		}
	}

	printf("ERROR: Trying to remove entry from empty EXEQ. Dying...\n");
	exit(-1);

	return retval;
}

/////////////////////////////////////////////////////////////
// Remove all entries from the EXEQ
/////////////////////////////////////////////////////////////

void EXEQ_flush(EXEQ *t) {

	for ( int ii = 0; ii < MAX_EXEQ_ENTRIES; ii++ ) {
		t->EXEQ_Entries[ii].valid = false;
	}
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
