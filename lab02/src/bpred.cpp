#include "bpred.h"

#define TAKEN true
#define NOTTAKEN false

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

BPRED::BPRED(uint32_t policy) {
	// Panic if policy is invalid at runtime
	if ( policy < 0 || policy >= NUM_BPRED_TYPE ) {
		assert(false);
	}

	// Set instance policy
	this->policy = static_cast<BPRED_TYPE>(policy);
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

bool BPRED::GetPrediction(uint32_t PC) {
	switch ( this->policy ) {
		case BPRED_ALWAYS_TAKEN:
			return TAKEN;
			break;
		default:
			assert(false); // Panic if something is very wrong
			break;
	}
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void BPRED::UpdatePredictor(uint32_t PC, bool resolveDir) {
	switch ( this->policy ) {
		case BPRED_ALWAYS_TAKEN:
			return;		// Nothing to be done for this policy
			break;
		default:
			assert(false); // Panic if something is very wrong
			break;
	}
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
