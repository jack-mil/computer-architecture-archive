/***********************************************************************
 * File         : pipeline.cpp
 * Author       : Jackson Miller
 * Date         : Sep. 22, 2023
 * Description  : Superscalar Pipeline for Lab2 ECE 6100
 **********************************************************************/

#include "pipeline.h"
#include <cstdlib>

// Definitions for branch prediction
#define TAKEN true
#define NOTTAKEN false

extern int32_t PIPE_WIDTH;
extern int32_t ENABLE_MEM_FWD;
extern int32_t ENABLE_EXE_FWD;
extern int32_t BPRED_POLICY;

bool VERBOSE = false;

// These definitions would normally be placed in the header but we can only
// modify and submit pipeline.cpp.

/* Structure to record information about dependencies on other instructions */
typedef struct Data_Dependency_Struct {
	bool exists{false};        // Indicates this dependency was found
	Op_Type inst_op_type;      // The OP TYPE of the dependent instruction
	uint64_t inst_op_id;       // OP ID of dep inst. used to determine youngest dep
	Latch_Type inst_latch_loc; // Pipeline stage the dep inst. is located
} Data_Dependency;

// Fills out the struct when a dep is detected
void record_dependancy(Data_Dependency *, Pipeline_Latch, Latch_Type);
// Helper function to determine if a given dep should stall.
// Uses ENABLE_MEM_FWD and ENABLE_EXE_FWD globals
bool does_dep_cause_stall(Data_Dependency);
/**********************************************************************
 * Support Function: Read 1 Trace Record From File and populate Fetch Op
 **********************************************************************/

uint8_t num_alu_inst = 0;
uint8_t prev_dest_reg = 0;
void pipe_get_fetch_op(Pipeline *p, Pipeline_Latch *fetch_op) {
	uint8_t bytes_read = 0;
	bytes_read = fread(&fetch_op->tr_entry, 1, sizeof(Trace_Rec) - 8, p->tr_file);

	// check for end of trace
	if ( bytes_read < sizeof(Trace_Rec) - 8 ) {
		fetch_op->valid = false;
		p->halt_op_id = p->op_id_tracker;
		return;
	}

	// got an instruction ... hooray!
	fetch_op->valid = true;
	fetch_op->stall = false;
	fetch_op->is_mispred_cbr = false;
	p->op_id_tracker++;
	fetch_op->op_id = p->op_id_tracker;
	fetch_op->tr_entry.src3_needed = 0;

	if ( fetch_op->tr_entry.op_type == OP_ALU ) {
		num_alu_inst++;
		if ( num_alu_inst == 5 ) {
			fetch_op->tr_entry.src3_needed = 1;
			fetch_op->tr_entry.src3_reg = prev_dest_reg;
			if ( !fetch_op->tr_entry.src1_needed ) {
				fetch_op->tr_entry.src1_needed = 1;
				fetch_op->tr_entry.src1_reg = prev_dest_reg;
			}
			if ( !fetch_op->tr_entry.src2_needed ) {
				fetch_op->tr_entry.src2_needed = 1;
				fetch_op->tr_entry.src2_reg = prev_dest_reg;
			}
			num_alu_inst = 0;
		}
	}

	if ( fetch_op->tr_entry.dest_needed ) {
		prev_dest_reg = fetch_op->tr_entry.dest;
	}
	return;
}

/**********************************************************************
 * Pipeline Class Member Functions
 **********************************************************************/

Pipeline *pipe_init(FILE *tr_file_in) {
	printf("\n** PIPELINE IS %d WIDE **\n\n", PIPE_WIDTH);
	// Initialize Pipeline Internals
	Pipeline *p = (Pipeline *)calloc(1, sizeof(Pipeline));
	p->tr_file = tr_file_in;
	p->halt_op_id = ((uint64_t)-1) - 3;

	// Allocated Branch Predictor
	p->fetch_cbr_stall = false;

	if ( BPRED_POLICY != -1 ) {
		p->b_pred = new BPRED(BPRED_POLICY);
		p->b_pred->stat_num_branches = 0;
		p->b_pred->stat_num_mispred = 0;
	}
	return p;
}

/**********************************************************************
 * Print the pipeline state (useful for debugging)
 **********************************************************************/

void pipe_print_state(Pipeline *p) {
	std::cout << "--------------------------------------------" << std::endl;
	std::cout << "cycle count : " << p->stat_num_cycle << " retired_instruction : " << p->stat_retired_inst << std::endl;

	uint8_t latch_type_i = 0; // Iterates over Latch Types
	uint8_t width_i = 0;      // Iterates over Pipeline Width
	for ( latch_type_i = 0; latch_type_i < NUM_LATCH_TYPES; latch_type_i++ ) {
		switch ( latch_type_i ) {
			case 0:
				printf("\t IF: ");
				break;
			case 1:
				printf("\t ID: ");
				break;
			case 2:
				printf("\t EX: ");
				break;
			case 3:
				printf("\t MA: ");
				break;
			default:
				printf("\t ---- ");
		}
	}
	printf("\n");
	for ( width_i = 0; width_i < PIPE_WIDTH; width_i++ ) {
		for ( latch_type_i = 0; latch_type_i < NUM_LATCH_TYPES; latch_type_i++ ) {
			if ( latch_type_i == 0 )
				printf("\t");
			if ( p->pipe_latch[latch_type_i][width_i].valid == true ) {
				printf(" %6u ", (uint32_t)(p->pipe_latch[latch_type_i][width_i].op_id));
			} else {
				printf(" ------ ");
			}
		}
		if ( VERBOSE ) {
			printf("\n");
			printf(" %u ", (uint32_t)(p->pipe_latch[0][width_i].tr_entry.op_type));
			printf(",");
			printf(" %u ", (uint32_t)(p->pipe_latch[0][width_i].tr_entry.dest));
			printf(",");
			printf(" %u ", (uint32_t)(p->pipe_latch[0][width_i].tr_entry.dest_needed));
			printf("<-");
			printf(" %u ", (uint32_t)(p->pipe_latch[0][width_i].tr_entry.src1_reg));
			printf(",");
			printf(" %u ", (uint32_t)(p->pipe_latch[0][width_i].tr_entry.src2_reg));
			printf(",");
			printf(" %u ", (uint32_t)(p->pipe_latch[0][width_i].tr_entry.src1_needed));
			printf(",");
			printf(" %u ", (uint32_t)(p->pipe_latch[0][width_i].tr_entry.src2_needed));
			printf(",");
			printf(" %u ", (uint32_t)(p->pipe_latch[0][width_i].tr_entry.cc_read));
			printf(",");
			printf(" %u ", (uint32_t)(p->pipe_latch[0][width_i].tr_entry.cc_write));

			printf("\n");
		}
	}
	printf("\n");
}

/**********************************************************************
 * Pipeline Main Function: Every cycle, cycle the stage
 **********************************************************************/

void pipe_cycle(Pipeline *p) {
	if ( VERBOSE )
		pipe_print_state(p);

	p->stat_num_cycle++;

	pipe_cycle_WB(p);
	pipe_cycle_MA(p);
	pipe_cycle_EX(p);
	pipe_cycle_ID(p);
	pipe_cycle_IF(p);

	if ( VERBOSE ) {
		if ( p->stat_num_cycle == 1000000 )
			exit(-1);
	}
}
/**********************************************************************
 * -----------  DO NOT MODIFY THE CODE ABOVE THIS LINE ----------------
 **********************************************************************/

void pipe_cycle_WB(Pipeline *p) {
	// TODO: UPDATE HERE (Part B)
	for ( int lane = 0; lane < PIPE_WIDTH; lane++ ) {
		Pipeline_Latch this_latch = p->pipe_latch[MA_LATCH][lane];
		if ( this_latch.valid ) {
			p->stat_retired_inst++;
			if ( this_latch.op_id >= p->halt_op_id ) {
				p->halt = true;
			}
		}

		// Check to unstall pipeline once branch has been resolved
		if ( p->fetch_cbr_stall && this_latch.is_mispred_cbr ) {
			p->fetch_cbr_stall = false;
		}
	}
}

//--------------------------------------------------------------------//

void pipe_cycle_MA(Pipeline *p) {
	int ii;
	for ( ii = 0; ii < PIPE_WIDTH; ii++ ) {
		p->pipe_latch[MA_LATCH][ii] = p->pipe_latch[EX_LATCH][ii];
	}
}

//--------------------------------------------------------------------//

void pipe_cycle_EX(Pipeline *p) {
	int ii;
	for ( ii = 0; ii < PIPE_WIDTH; ii++ ) {
		p->pipe_latch[EX_LATCH][ii] = p->pipe_latch[ID_LATCH][ii];
	}
}

//--------------------------------------------------------------------//

void pipe_cycle_ID(Pipeline *p) {
	// TODO: UPDATE HERE (B)

	// First copy each incoming instruction into all ID latches
	// So we can check across pipeline lanes for dep later.
	for ( int32_t lane = 0; lane < PIPE_WIDTH; lane++ ) {
		p->pipe_latch[ID_LATCH][lane] = p->pipe_latch[IF_LATCH][lane];
		// clear any previous stalls, will be set again if needed
		p->pipe_latch[IF_LATCH][lane].stall = false;
	}

	// Now for every lane in the superscalar pipeline
	for ( int32_t lane = 0; lane < PIPE_WIDTH; lane++ ) {

		// We are only concerned with Read After Write dependencies.
		// Dependencies could exist between reading src1,src2,src3 or the status register
		// and some other instruction's write destination register.
		Data_Dependency src1_raw_dep = {};
		Data_Dependency src2_raw_dep = {};
		Data_Dependency src3_raw_dep = {};
		Data_Dependency cc_raw_dep = {};

		// Save the current latch state for readability
		Pipeline_Latch &this_latch = p->pipe_latch[ID_LATCH][lane];

		// Skip all checks for this lane if latch is "empty" (bubble)
		if ( !this_latch.valid ) {
			continue;
		}

		for ( int32_t lane_j = 0; lane_j < PIPE_WIDTH; lane_j++ ) {

			// Check Latches EX, MA and ID (in other lanes) for colliding instructions
			for ( const auto latch_type : {Latch_Type::EX_LATCH, Latch_Type::MA_LATCH, Latch_Type::ID_LATCH} ) {
				Pipeline_Latch &other_latch = p->pipe_latch[latch_type][lane_j];
				// Skip invalid instructions, or younger instructions (in the case of ID_LATCH)
				// (will execute after this one, so no hazard)
				if ( !other_latch.valid ||
				     other_latch.op_id >= this_latch.op_id ) {
					continue;
				}
				// RAW dep between src1 reg and other_latch dest
				if ( other_latch.tr_entry.dest_needed &&
				     this_latch.tr_entry.src1_needed &&
				     this_latch.tr_entry.src1_reg == other_latch.tr_entry.dest ) {

					// Only identify the *youngest* dependency. Older ones will finish first anyway
					if ( !src1_raw_dep.exists || src1_raw_dep.inst_op_id < other_latch.op_id ) {
						record_dependancy(&src1_raw_dep, other_latch, latch_type);
					}
				}

				// RAW dep between src2 reg and other_latch dest
				if ( other_latch.tr_entry.dest_needed &&
				     this_latch.tr_entry.src2_needed &&
				     this_latch.tr_entry.src2_reg == other_latch.tr_entry.dest ) {

					if ( !src2_raw_dep.exists || src2_raw_dep.inst_op_id < other_latch.op_id ) {
						record_dependancy(&src2_raw_dep, other_latch, latch_type);
					}
				}

				// RAW dep between src3 reg and other_latch dest
				if ( other_latch.tr_entry.dest_needed &&
				     this_latch.tr_entry.src3_needed &&
				     this_latch.tr_entry.src3_reg == other_latch.tr_entry.dest ) {

					if ( !src3_raw_dep.exists || src3_raw_dep.inst_op_id < other_latch.op_id ) {
						record_dependancy(&src3_raw_dep, other_latch, latch_type);
					}
				}

				// RAW dep between CC reg and other_latch CC write
				if ( other_latch.tr_entry.cc_write &&
				     this_latch.tr_entry.cc_read ) {

					if ( cc_raw_dep.exists || cc_raw_dep.inst_op_id < other_latch.op_id ) {
						record_dependancy(&cc_raw_dep, other_latch, latch_type);
					}
				}
			}
		}

		// If we need to stall for any reason
		if ( does_dep_cause_stall(src1_raw_dep) || does_dep_cause_stall(src2_raw_dep) ||
		     does_dep_cause_stall(src3_raw_dep) || does_dep_cause_stall(cc_raw_dep) ) {

			this_latch.valid = false;                   // make this stage a bubble
			p->pipe_latch[IF_LATCH][lane].stall = true; // prevent fetch stage from grabbing more instructions

			for ( int32_t lane_i = 0; lane_i < PIPE_WIDTH; lane_i++ ) {
				// Since this instruction is stalled, we need to stall any instructions that follow it
				// in the other lanes. This ensures in-order execution.
				if ( p->pipe_latch[ID_LATCH][lane_i].op_id > this_latch.op_id ) {
					p->pipe_latch[ID_LATCH][lane_i].valid = false; // make that stage a bubble
					p->pipe_latch[IF_LATCH][lane_i].stall = true;  // and stall the incoming instructions
				}
			}
		}
	}
}

//--------------------------------------------------------------------//

void pipe_cycle_IF(Pipeline *p) {
	// TODO: UPDATE HERE (Part A, B)
	Pipeline_Latch fetch_op;

	// Fetch a new instruction for each pipeline lane
	for ( int lane = 0; lane < PIPE_WIDTH; lane++ ) {
		// Unless the respective ID stage has caused a stall
		if ( p->pipe_latch[IF_LATCH][lane].stall ) {
			continue;
		}

		// Check if fetching should be stalled due to branch mis-prediction
		if ( !p->fetch_cbr_stall ) {
			pipe_get_fetch_op(p, &fetch_op);
			if ( BPRED_POLICY != -1 && fetch_op.tr_entry.op_type == OP_CBR ) {
				pipe_check_bpred(p, &fetch_op);
			}

			// Insert the instruction, overwriting the previous value
			// (hopefully already copied to ID_LATCH)
			p->pipe_latch[IF_LATCH][lane] = fetch_op;
		} else {
			// In the case that fetch is currently stalled, insert a bubble
			p->pipe_latch[IF_LATCH][lane].valid = false;
		}
	}
}

//--------------------------------------------------------------------//

// This function only called for OP_CBR type
void pipe_check_bpred(Pipeline *p, Pipeline_Latch *fetch_op) {
	// call branch predictor here, if mispred then mark in fetch_op
	// update the predictor instantly
	// stall fetch using the flag p->fetch_cbr_stall

	// Only update for CBR op types
	if ( fetch_op->tr_entry.op_type != OP_CBR ) {
		return;
	}

	// Count the number of branches encountered
	p->b_pred->stat_num_branches++;

	// This is the instruction PC
	uint64_t pc = fetch_op->tr_entry.inst_addr;
	// This is the true direction (TAKEN/NOTTAKEN)
	bool resolved_dir = fetch_op->tr_entry.br_dir == 1 ? TAKEN : NOTTAKEN;
	// This will get the prediction fot that PC
	bool prediction = p->b_pred->GetPrediction(pc);

	// Check for mispredicton
	if ( prediction != resolved_dir ) {
		fetch_op->is_mispred_cbr = true;
		p->fetch_cbr_stall = true;
		// Count the number of mispredictions encountered
		p->b_pred->stat_num_mispred++;
	}
	// Always update the predictor immediately
	p->b_pred->UpdatePredictor(pc, resolved_dir);
}

//--------------------------------------------------------------------//

// Helper Functions for Part A

// Determine if a given dep should stall.
// Uses ENABLE_MEM_FWD and ENABLE_EXE_FWD globals
bool does_dep_cause_stall(Data_Dependency dep) {
	// Obviously shouldn't stall if there is no dependency
	if ( !dep.exists )
		return false;
	// Otherwise, stall condition controlled by cmd line arguments (GLOBALS, eww)
	// and the latch location of the dependent instruction
	switch ( dep.inst_latch_loc ) {
		case ID_LATCH:
			// Instructions in other pipeline lane's ID_LATCH should always stall
			return true;
			break;
		case EX_LATCH:
			// Load instructions cannot be forwarded from EX_LATCH to ID_LATCH
			// All others can
			return (dep.inst_op_type == OP_LD) || !ENABLE_EXE_FWD;
			break;
		case MA_LATCH:
			// Instructions in the MA Latch can always be forwarded
			return !ENABLE_MEM_FWD;
			break;
		default:
			// This is bad and should never happen
			assert(false);
			break;
	}
}

// Fills out the dependency struct to remove repetitive code above
void record_dependancy(Data_Dependency *dep, Pipeline_Latch latch, Latch_Type latch_type) {
	dep->exists = true;
	dep->inst_latch_loc = latch_type;
	dep->inst_op_type = (Op_Type)latch.tr_entry.op_type;
	dep->inst_op_id = latch.op_id;
}