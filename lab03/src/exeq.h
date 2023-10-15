#ifndef _EXEQ_H_
#define _EXEQ_H_
#include "cache.h"
#include "trace.h"
#include <assert.h>
#include <cstdlib>
#include <inttypes.h>

#define MAX_EXEQ_ENTRIES 16
extern int32_t LOAD_EXE_CYCLES;
extern int32_t CACHE;

typedef struct EXEQ_Entry_Struct {
	bool valid;
	Inst_Info inst;
} EXEQ_Entry;

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

typedef struct EXEQ {
	EXEQ_Entry EXEQ_Entries[MAX_EXEQ_ENTRIES];
} EXEQ;

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

EXEQ *EXEQ_init(void);
void EXEQ_print_state(EXEQ *t);
void EXEQ_cycle(EXEQ *t);
void EXEQ_insert(EXEQ *t, Inst_Info inst, Cache *c);
bool EXEQ_check_done(EXEQ *t);
Inst_Info EXEQ_remove(EXEQ *t);
void EXEQ_flush(EXEQ *t);

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

#endif
