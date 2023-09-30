#ifndef _PIPELINE_H
#define _PIPELINE_H

#include <inttypes.h>
#include <stdio.h>
#include <assert.h>
#include <queue>

#include "trace.h"

#include "rat.h"
#include "rob.h"
#include "exeq.h"
#include "cache.h"

#define MAX_PIPE_WIDTH 8
#define MAX_WRITEBACKS 256

/*********************************************************************
* Pipeline Class & Internal Structures
**********************************************************************/

// Pipeline Latches
typedef struct Pipe_Latch_Struct {
  bool valid;
  bool stall;
  Inst_Info inst;
}Pipe_Latch;

typedef enum Exception_State_Enum{
  TRACE_FETCH,
  HANDLING_EXCEPTION,
  STARTING_RECOVERY,
  IN_RECOVERY,
  FINISHED_RECOVERY
} Exception_State;

typedef struct Pipeline {
  FILE *tr_file;
  Pipe_Latch  FE_latch[MAX_PIPE_WIDTH]; // fetch Latches
  Pipe_Latch  ID_latch[MAX_PIPE_WIDTH]; // decode Latches
  Pipe_Latch  SC_latch[MAX_PIPE_WIDTH]; // schedule Latches
  Pipe_Latch  EX_latch[MAX_WRITEBACKS]; // exe Latches (note, can be > pipe_width)

  ROB  *pipe_ROB;
  RAT  *pipe_RAT;
  EXEQ *pipe_EXEQ;  // execution Q for multicycle ops (students need not implement this object)

  uint64_t inst_num_tracker; //sequence number for inst
  uint64_t halt_inst_num;   // last inst in Trace
  bool halt;               // Pipeline is halted Flag

  // Statistics: students need to update these counters
  uint64_t stat_retired_inst;         // Total Committed Instructions
  uint64_t stat_num_cycle;            // Total Cycles

  //Exception Handling
  int32_t remaining_exception_handler_cycles;
  std::queue<Inst_Info> recovery_window;
  uint64_t last_recovery_inst;
  Exception_State exception_state;

  cache* pipeline_cache;


}Pipeline;


Pipeline* pipe_init(FILE *tr_file);        // Allocate Structures

void pipe_cycle(Pipeline *p);              // Runs one Pipeline Cycle
void pipe_cycle_fetch(Pipeline *p);        // Fetch Stage
void pipe_cycle_decode(Pipeline *p);       // Decode Stage
void pipe_cycle_issue(Pipeline *p);        // insert into ROB (rename)
void pipe_cycle_schedule(Pipeline *p);     // schedule instruction to execute
void pipe_cycle_exe(Pipeline *p);          // execute (multi-cycle)
void pipe_cycle_writeback(Pipeline *p);    // writeback and update ROB
void pipe_cycle_commit(Pipeline *p);       // commit

void pipe_print_state(Pipeline *p);        // Print Pipeline state

#endif
