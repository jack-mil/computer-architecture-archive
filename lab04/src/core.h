#ifndef CORE_H
#define CORE_H

#include "types.h"
#include "memsys.h"

typedef struct Core Core;



////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////


struct Core {
  uint32_t core_id;

  Memsys* memsys;
    
  char trace_fname[1024];
  FILE* trace;
    
  uint32_t done;

  uint64_t trace_inst_addr;
  uint64_t trace_inst_type;
  uint64_t trace_ldst_addr;
  
  uint64_t snooze_end_cycle; // when waiting for data to return

  unsigned long long inst_count;
  unsigned long long done_inst_count;
  unsigned long long done_cycle_count;
};



//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

Core* core_new(Memsys* memsys, char* trace_fname, uint32_t core_id);
void core_cycle(Core* core);
void core_print_stats(Core* c);
void core_read_trace(Core* c);
void core_init_trace(Core* c);

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#endif // CORE_H
