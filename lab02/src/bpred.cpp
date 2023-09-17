#include "bpred.h"
#include <algorithm>

#define TAKEN true
#define NOTTAKEN false

const uint32_t MASK14 = (1 << 14) - 1;
const uint32_t MASK7 = (1 << 7) - 1;

// I would have made these methods part of the BPRED class,
// But I can't modify the "bpred.h" file.

bool gshare_predict(BPRED *pred, uint32_t PC) {
	// Index into PHT is 14 bits of PC XORed with the 14 bits of GHR
	uint16_t index = (PC ^ pred->global_hist_reg) & MASK14;
	// Predict TAKEN if counter is '11' or '10', NOT-TAKEN if '01' or '00'
	return pred->gshare_pattern_table[index] >= 2 ? TAKEN : NOTTAKEN;
}

void update_at_index(BPRED *pred, uint16_t index, bool resolveDir) {
	uint32_t curr_state = pred->gshare_pattern_table[index];
	uint32_t next_state;
	uint32_t next_ghr;
	if ( resolveDir == TAKEN ) {
		// Increment the state counter up to 3 (modulo 4) on a taken branch
		next_state = curr_state < 4 ? curr_state + 1 : 3;
		// Shift a 1 into GHR, then mask to 14 bits
		next_ghr = ((pred->global_hist_reg << 1) + 1u) & MASK14;
	} else {
		// Decrement the state counter down to 0 on a not-taken branch
		next_state = curr_state > 0 ? curr_state - 1 : 0;
		// Shift a 0 into GHR, then mask to 14 bits
		next_ghr = (pred->global_hist_reg << 1) & MASK14;
	}
	// Update the Pattern History Table
	pred->gshare_pattern_table[index] = next_state;
	// Update the Global History Register
	pred->global_hist_reg = next_ghr;
}
void gshare_update(BPRED *pred, uint32_t PC, bool resolveDir) {
	// Index into PHT is 14 bits of PC XORed with the 14 bits of GHR
	uint16_t index = (PC ^ pred->global_hist_reg) & MASK14;
	update_at_index(pred, index, resolveDir);
}

bool gselect_predict(BPRED *pred, uint32_t PC) {
	// Index into PHT is 7 low bits of PC concat with the 7 low bits of GHR
	uint16_t index = ((PC & MASK7) << 7) | (pred->global_hist_reg & MASK7);
	// Predict TAKEN if counter is '11' or '10', NOT-TAKEN if '01' or '00'
	return pred->gshare_pattern_table[index] >= 2 ? TAKEN : NOTTAKEN;
}
void gselect_update(BPRED *pred, uint32_t PC, bool resolveDir) {
	// Index into PHT is 7 low bits of PC concat with the 7 low bits of GHR
	uint16_t index = ((PC & MASK7) << 7) | (pred->global_hist_reg & MASK7);
	update_at_index(pred, index, resolveDir);
}
bool tournament_predict(BPRED *pred, uint32_t PC) {
	return TAKEN;
}
void tournament_update(BPRED *pred, uint32_t PC, bool resolveDir) {}

/////////////////////////////////////////////////////////////

// Construct branch predictor
BPRED::BPRED(uint32_t policy) {
	// Panic if policy is invalid at runtime
	if ( policy < 0 || policy >= NUM_BPRED_TYPE ) {
		assert(false);
	}

	// Set instance policy
	this->policy = static_cast<BPRED_TYPE>(policy);

	this->stat_num_branches = 0;
	this->stat_num_mispred = 0;

	this->global_hist_reg = 0;

	// Initilize the PHT to the weakly taken state "10" = 2;
	std::fill_n(gshare_pattern_table, 1 << 14, 2u);
	std::fill_n(this->gselect_pattern_table, 1 << 14, 2u);
	std::fill_n(this->tournament_pattern_table, 1 << 10, 2u);

	// std::cout << "Predictor Initilized with Policy [" << this->policy << "]" << std::endl;
}

/////////////////////////////////////////////////////////////

bool BPRED::GetPrediction(uint32_t PC) {
	switch ( this->policy ) {
		case BPRED_ALWAYS_TAKEN:
			return TAKEN;
			break;
		case BPRED_GSHARE:
			return gshare_predict(this, PC);
		case BPRED_GSELECT:
			return gselect_predict(this, PC);
		case BPRED_TOURNAMENT:
			return tournament_predict(this, PC);
		default:
			assert(false); // Panic if something is very wrong
			break;
	}
}

/////////////////////////////////////////////////////////////

void BPRED::UpdatePredictor(uint32_t PC, bool resolveDir) {
	switch ( this->policy ) {
		case BPRED_ALWAYS_TAKEN:
			return; // Nothing to be done for this policy
			break;
		case BPRED_GSHARE:
			return gshare_update(this, PC, resolveDir);
			break;
		case BPRED_GSELECT:
			return gselect_update(this, PC, resolveDir);
			break;
		case BPRED_TOURNAMENT:
			return tournament_update(this, PC, resolveDir);
			break;
		default:
			assert(false); // Panic if something is very wrong
			break;
	}
}
