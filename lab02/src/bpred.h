#ifndef _BPRED_H_
#define _BPRED_H_
#include <inttypes.h>
#include <cassert>


static inline uint32_t SatIncrement(uint32_t x, uint32_t max)
{
    if(x < max) 
        return x+1;
    return x;
}

static inline uint32_t SatDecrement(uint32_t x)
{
    if(x > 0)
        return x-1;
    return x;
}

typedef enum BPRED_TYPE_ENUM {
    BPRED_PERFECT=0,
    BPRED_ALWAYS_TAKEN=1,
    BPRED_GSHARE=2,
    BPRED_GSELECT=3,
    BPRED_TOURNAMENT=4,
    NUM_BPRED_TYPE=5
} BPRED_TYPE;

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

class BPRED{
  BPRED_TYPE policy;

public:
  uint64_t stat_num_branches;
  uint64_t stat_num_mispred;
  
  uint32_t global_hist_reg;
  uint32_t gshare_pattern_table[1 << 14];
  uint32_t gselect_pattern_table[1 << 14];
  uint32_t tournament_pattern_table[1 << 10];
  bool latest_gshare_pred;
  bool latest_gselect_pred;
  
// The interface to the three functions below CAN NOT be changed
    BPRED(uint32_t policy);
    bool GetPrediction(uint32_t PC);  
    void UpdatePredictor(uint32_t PC, bool resolveDir);
};

/***********************************************************/
#endif

