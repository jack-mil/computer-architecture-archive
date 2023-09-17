#include "bpred.h"
#include <iostream>

#define TAKEN true
#define NOTTAKEN false

const uint32_t mask_14 = (1 << 14) - 1;

// I would have made these methods part of the BPRED class,
// But I can't modify the "bpred.h" file.

bool gshare_predict(BPRED *pred, uint32_t PC) {
	// Index into PHT is 14 bits of PC XORed with the 14 bits of GHR
	uint16_t index = (PC ^ pred->global_hist_reg) & mask_14;
	// Predict TAKEN if counter is '11' or '10', NOT-TAKEN if '01' or '00'
	return pred->gshare_pattern_table[index] >= 2 ? TAKEN : NOTTAKEN;
}

void gshare_update(BPRED *pred, uint32_t PC, bool resolveDir) {
	// Index into PHT is 14 bits of PC XORed with the 14 bits of GHR
	uint16_t index = (PC ^ pred->global_hist_reg) & mask_14;
	uint32_t curr_state = pred->gshare_pattern_table[index];
	uint32_t next_state;
	uint32_t next_ghr;
	if ( resolveDir == TAKEN ) {
		// Increment the state counter up to 3 (modulo 4) on a taken branch
		next_state = (curr_state + 1) % 4;
		// Shift a 1 into GHR, then mask to 14 bits
		next_ghr = ((pred->global_hist_reg << 1) + 1) & mask_14;
	} else {
		// Decrement the state counter down to 0 on a not-taken branch
		next_state = curr_state > 0 ? curr_state - 1 : 0;
		// Shift a 0 into GHR, then mask to 14 bits
		next_ghr = (pred->global_hist_reg << 1) & mask_14;
	}
	// Update the Pattern History Table
	pred->gshare_pattern_table[index] = next_state;
	// Update the Global History Register
	pred->global_hist_reg = next_ghr;
}
bool gselect_predict(BPRED *pred, uint32_t PC) {
	return TAKEN;
}
void gselect_update(BPRED *pred, uint32_t PC, bool resolveDir) {
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

	//std::cout << "Predictor Initilized with Policy [" << this->policy << "]" << std::endl;
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
