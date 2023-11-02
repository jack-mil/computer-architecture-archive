#ifndef __TYPES_H__

#define HIT   1
#define MISS  0

#define MAX_CORES 2

// Precision for PrintStats
#define UNS_PREC " %8llu"
#define DBL_PREC "%9.3f"

typedef uint64_t Addr;

typedef enum Inst_Type_Enum {
    INST_TYPE_ALU=0,
    INST_TYPE_LOAD=1,
    INST_TYPE_STORE=2, 
    INST_TYPE_OTHER=3, 
} Inst_Type;

typedef enum Access_Type_Enum {
    ACCESS_TYPE_IFETCH=0,
    ACCESS_TYPE_LOAD=1,
    ACCESS_TYPE_STORE=2, 
} Access_Type;

typedef enum MODE_Enum {
    SIM_MODE_NONE=0,
    SIM_MODE_A=1,
    SIM_MODE_B=2,
    SIM_MODE_C=3,
    SIM_MODE_D=4,
    SIM_MODE_E=5,
    SIM_MODE_F=6
} MODE;

/**************************************************************************************/

#define __TYPES_H__
#endif  
