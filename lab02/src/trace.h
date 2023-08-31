#ifndef _TRACE_H
#define _TRACE_H

#include <stdio.h>
#include <iostream>
#include <inttypes.h>

typedef enum Op_Type_Enum{
    OP_ALU,             // ALU(ADD/ SUB/ MUL/ DIV) operation
    OP_LD,              // load operation
    OP_ST,              // store operation
    OP_CBR,             // Conditional Branch
    OP_OTHER,           // Other Ops
    NUM_OP_TYPE
} Op_Type;

/* Data structure for Trace Record */
typedef struct Trace_Rec_Struct {
    uint64_t inst_addr;   // instruction address
    uint8_t  op_type;     // optype
    uint8_t  dest;        // Destination - valid if dest_needed is set
    uint8_t  dest_needed; // Indicates whether this instruction uses a destination register
    uint8_t  src1_reg;    // Source Register 1 - valid if src1_needed is set
    uint8_t  src2_reg;    // Source Register 2 - valid if src2_needed is set
    uint8_t  src1_needed; // Source Register 1 needed by this instruction
    uint8_t  src2_needed; // Source Register 2 needed by this instruction
    uint8_t  cc_read;     // Conditional Code Read - set if this instruction reads
                          // the condition status register (i.e., it's a branch)
    uint8_t  cc_write;    // Conditional Code Write - set if this instruction
                          // updates the condition status register
    uint64_t mem_addr;    // Load / Store Memory Address
    uint8_t  mem_write;   // Write
    uint8_t  mem_read;    // Read
    uint8_t  br_dir;      // Branch Direction Taken / Not Taken
    uint64_t br_target;   // Target Address of Branch
    uint8_t  src3_reg;    // Source Register 3 - valid if src3_needed is set
    uint8_t  src3_needed; // Source Register 3 needed by this instruction
} Trace_Rec;

#endif
