#include "bpred.h"
#include <algorithm>

#define TAKEN true
#define NOTTAKEN false

// Masks used for calculating PHT indexes
static const uint32_t MASK14 = (1 << 14) - 1;
static const uint32_t MASK10 = (1 << 10) - 1;
static const uint32_t MASK7 = (1 << 7) - 1;

// Define the index calculation functions in only one place

// Index into PHT is 14 bits of PC XORed with the 14 bits of GHR
inline uint16_t gshare_index(uint32_t pc, uint32_t ghr) {
	return (pc ^ ghr) & MASK14;
}

// Index into PHT is 7 low bits of PC concat with the 7 low bits of GHR
inline uint16_t gselect_index(uint32_t pc, uint32_t ghr) {
	return ((pc & MASK7) << 7) | (ghr & MASK7);
}

// Index into PHT is 10 low bits of PC
inline uint16_t tourn_index(uint32_t pc) {
	return (pc & MASK10);
}

// Helper inline functions for state machine increment
inline unsigned int state_inc(unsigned int i) {
	if ( i < 3 )
		return i + 1;
	return i;
}
inline unsigned int state_dec(unsigned int i) {
	if ( i > 0 )
		return i - 1;
	return i;
}

/* Take a PHT and an index and update the 2-bit state machine */
void update_at_index(uint32_t *, uint16_t, bool);

/////////////////////////////////////////////////////////////

// Construct branch predictor
BPRED::BPRED(uint32_t policy) {
	// Panic if policy is invalid at runtime
	if ( policy < 0 || policy >= NUM_BPRED_TYPE ) {
		assert(false);
	}

	// Set instance policy
	this->policy = static_cast<BPRED_TYPE>(policy);

	// Set all statistics
	this->stat_num_branches = 0;
	this->stat_num_mispred = 0;

	// Start shift register with 0s
	this->global_hist_reg = 0;

	// Initilize the PHT(s) to the weakly taken state "10" = 2;
	std::fill_n(gshare_pattern_table, 1 << 14, 2u);
	std::fill_n(this->gselect_pattern_table, 1 << 14, 2u);
	std::fill_n(this->tournament_pattern_table, 1 << 10, 2u);

}

/////////////////////////////////////////////////////////////

bool BPRED::GetPrediction(uint32_t PC) {

	// I get all the predictions at once in order to facilitate tournament

	/* DO GSHARE */
	// Index into PHT is 14 bits of PC XORed with the 14 bits of GHR
	uint16_t gshare_ind = gshare_index(PC, this->global_hist_reg);
	// Predict TAKEN if counter is '11' or '10', NOT-TAKEN if '01' or '00'
	this->latest_gshare_pred = this->gshare_pattern_table[gshare_ind] >= 2;

	/* DO GSELECT */
	// Index into PHT is 7 low bits of PC concat with the 7 low bits of GHR
	uint16_t gselect_ind = gselect_index(PC,this->global_hist_reg);
	// Predict TAKEN if counter is '11' or '10', NOT-TAKEN if '01' or '00'
	this->latest_gselect_pred = this->gselect_pattern_table[gselect_ind] >= 2;

	/* DO TOURNAMENT */
	// Index into PHT is 10 low bits of PC
	uint16_t tourn_ind = tourn_index(PC);
	// Follow GSELECT if counter is '11' or '10', GSHARE if '01' or '00'
	bool tournament_pred = this->tournament_pattern_table[tourn_ind] >= 2 ? this->latest_gselect_pred : this->latest_gshare_pred;

	// Return the prediction according to set policy
	switch ( this->policy ) {
		case BPRED_ALWAYS_TAKEN:
			return TAKEN;
		case BPRED_GSHARE:
			return this->latest_gshare_pred;
		case BPRED_GSELECT:
			return this->latest_gselect_pred;
		case BPRED_TOURNAMENT:
			return tournament_pred;
		default:
			assert(false); // Panic if something is very wrong
			break;
	}
}

/////////////////////////////////////////////////////////////

void BPRED::UpdatePredictor(uint32_t PC, bool resolveDir) {

	// Return early to save work if always taken
	if ( this->policy == BPRED_ALWAYS_TAKEN )
		return;

	// THESE NEED TO BE CONDITIONAL (??)
	/* DO GSHARE */
	// Index into PHT is 14 bits of PC XORed with the 14 bits of GHR
	uint16_t gshare_ind = gshare_index(PC, this->global_hist_reg);
	update_at_index(gshare_pattern_table, gshare_ind, resolveDir);

	/* DO GSELECT */
	// Index into PHT is 7 low bits of PC concat with the 7 low bits of GHR
	uint16_t gselect_ind = gselect_index(PC,this->global_hist_reg);
	update_at_index(gselect_pattern_table, gselect_ind, resolveDir);

	/* DO TOURNAMENT */
	// Index into PHT is 10 low bits of PC
	uint16_t tourn_ind = tourn_index(PC);
	// If the predictions differ
	if ( this->latest_gselect_pred != this->latest_gshare_pred ) {
		if ( this->latest_gselect_pred == resolveDir ) {
			update_at_index(tournament_pattern_table, tourn_ind, true);
		} else if ( this->latest_gshare_pred == resolveDir ) { // I think this condition is inherent
			update_at_index(tournament_pattern_table, tourn_ind, false);
		} else {
			// this should never occur?
			assert(false);
		}
	}

	/* Update GHR (Importantly after the table(s) are indexed */
	uint32_t next_ghr;
	if ( resolveDir == TAKEN ) {
		// Shift a 1 into GHR, then mask to 14 bits
		next_ghr = ((this->global_hist_reg << 1) + 1u) & MASK14;
	} else {
		// Shift a 0 into GHR, then mask to 14 bits
		next_ghr = (this->global_hist_reg << 1) & MASK14;
	}
	// Update the Global History Register
	this->global_hist_reg = next_ghr;
}

/* Take a PHT and an index and update the 2-bit state machine */
void update_at_index(uint32_t *table, uint16_t index, bool resolveDir) {
	uint32_t curr_state = table[index];
	// Update the Pattern History Table
	if ( resolveDir == TAKEN ) {
		table[index] = state_inc(curr_state);
	} else {
		table[index] = state_dec(curr_state);
	}
}
