/***********************************************************************
 * File         : pipeline.cpp
 * Author       : Soham J. Desai
 * Date         : 14th January 2014
 * Description  : Superscalar Pipeline for Lab2 ECE 6100
 *
 * Version 2.0 (Updates for 6-stage pipeline)
 * Author       : Geonhwa Jeong
 * Date         : 30th August 2022
 * Description  : Superscalar Pipeline for Lab2 CS4290/CS6290/ECE4100/ECE6100 [Fall 2022]
 *
 * Version 5.0 (Updates for 5-stage pipeline with 3-src ALU instructions and Tournament Branch Predictor)
 * Author       : Samarth Chandna
 * Date         : 27th August 2023
 * Description  : Superscalar Pipeline for Lab2 CS4290/CS6290/ECE4100/ECE6100 [Fall 2023]
 *
 **********************************************************************/

#include "pipeline.h"
#include <cstdlib>

extern int32_t PIPE_WIDTH;
extern int32_t ENABLE_MEM_FWD;
extern int32_t ENABLE_EXE_FWD;
extern int32_t BPRED_POLICY;

bool VERBOSE = false;

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
	int ii;
	for ( ii = 0; ii < PIPE_WIDTH; ii++ ) {
		if ( p->pipe_latch[MA_LATCH][ii].valid ) {
			p->stat_retired_inst++;
			if ( p->pipe_latch[MA_LATCH][ii].op_id >= p->halt_op_id ) {
				p->halt = true;
			}
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
	// TODO: UPDATE HERE (Part A, B)

	// USED LATER for SUPERSCALAR
	bool pipeline_stalled = false;
	uint64_t oldest_id_stalled = 0;

	/* Structure to record information about dependencies on other instructions */
	typedef struct Data_Dependency_Struct {
		bool exists{false};        // Indicates this dependency was found
		Op_Type inst_op_type;      // The OP TYPE of the dependant instruction
		uint64_t inst_op_id;       // OP ID of dep inst. used to determine youngest dep
		Latch_Type inst_latch_loc; // Pipeline stage the dep inst. is located
	} Data_Dependency;

	// We are only concered with Read After Write dependencies
	Data_Dependency src1_raw_dep = {};
	Data_Dependency src2_raw_dep = {};
	Data_Dependency src3_raw_dep = {};
	Data_Dependency cc_raw_dep = {};

	// First copy each incoming instruction into all ID latches
	// So we can check accross pipeline lanes for dep later
	for ( int32_t lane = 0; lane < PIPE_WIDTH; lane++ ) {
		p->pipe_latch[ID_LATCH][lane] = p->pipe_latch[IF_LATCH][lane];
		// clear any previous stalls, will be set again if needed
		p->pipe_latch[IF_LATCH][lane].stall = false;
	}

	for ( int32_t ii = 0; ii < PIPE_WIDTH; ii++ ) {

		// Copy potential next instruction from previous latch
		p->pipe_latch[ID_LATCH][ii] = p->pipe_latch[IF_LATCH][ii];
		Pipeline_Latch &this_inst = p->pipe_latch[ID_LATCH][ii];
		Pipeline_Latch &ex_inst = p->pipe_latch[EX_LATCH][ii];
		Pipeline_Latch &ma_inst = p->pipe_latch[MA_LATCH][ii];

		// Skip all checks for this lane if latch is "empy" (bubble)
		if ( !this_inst.valid ) {
			continue;
		}

		/* CHECK EX_LATCH for Dependencies*/
		if ( ex_inst.valid ) { // invalid instructions are "bubbles"
			// RAW dep between src1 reg and EX_LATCH dest
			if ( ex_inst.tr_entry.dest_needed &&
			     this_inst.tr_entry.src1_needed &&
			     this_inst.tr_entry.src1_reg == ex_inst.tr_entry.dest ) {
				// Conditionally overwrite below info if a YOUNGER dependance is found (SUPERSCALAR)
				// Means largest op_id
				src1_raw_dep.exists = true;
				src1_raw_dep.inst_latch_loc = EX_LATCH;
				src1_raw_dep.inst_op_type = (Op_Type)ex_inst.tr_entry.op_type;
				src1_raw_dep.inst_op_id = ex_inst.op_id;
			}

			// RAW dep between src2 reg and EX_LATCH dest
			if ( ex_inst.tr_entry.dest_needed &&
			     this_inst.tr_entry.src2_needed &&
			     this_inst.tr_entry.src2_reg == ex_inst.tr_entry.dest ) {

				src2_raw_dep.exists = true;
				src2_raw_dep.inst_latch_loc = EX_LATCH;
				src2_raw_dep.inst_op_type = (Op_Type)ex_inst.tr_entry.op_type;
				src2_raw_dep.inst_op_id = ex_inst.op_id;
			}

			// RAW dep between src3 reg and EX_LATCH dest
			if ( ex_inst.tr_entry.dest_needed &&
			     this_inst.tr_entry.src3_needed &&
			     this_inst.tr_entry.src3_reg == ex_inst.tr_entry.dest ) {
				src3_raw_dep.exists = true;
				src3_raw_dep.inst_latch_loc = EX_LATCH;
				src3_raw_dep.inst_op_type = (Op_Type)ex_inst.tr_entry.op_type;
				src3_raw_dep.inst_op_id = ex_inst.op_id;
			}

			// RAW dep between CC reg and EX_LATCH CC write
			if ( ex_inst.tr_entry.cc_write &&
			     this_inst.tr_entry.cc_read ) {
				cc_raw_dep.exists = true;
				cc_raw_dep.inst_latch_loc = EX_LATCH;
				cc_raw_dep.inst_op_type = (Op_Type)ex_inst.tr_entry.op_type;
				cc_raw_dep.inst_op_id = ex_inst.op_id;
			}
		}
		/* CHECK MA_LATCH for Dependencies*/
		if ( ma_inst.valid ) { // invalid instructions are "bubbles"
			// RAW dep between src1 reg and MA_LATCH dest
			if ( ma_inst.tr_entry.dest_needed &&
			     this_inst.tr_entry.src1_needed &&
			     this_inst.tr_entry.src1_reg == ma_inst.tr_entry.dest ) {
				// Conditionally overwrite below info if a YOUNGER dependance is found (SUPERSCALAR)
				// Means largest op_id
				src1_raw_dep.exists = true;
				src1_raw_dep.inst_latch_loc = MA_LATCH;
				src1_raw_dep.inst_op_type = (Op_Type)ma_inst.tr_entry.op_type;
				src1_raw_dep.inst_op_id = ma_inst.op_id;
			}

			// RAW dep between src2 reg and MA_LATCH dest
			if ( ma_inst.tr_entry.dest_needed &&
			     this_inst.tr_entry.src2_needed &&
			     this_inst.tr_entry.src2_reg == ma_inst.tr_entry.dest ) {
				src2_raw_dep.exists = true;
				src2_raw_dep.inst_latch_loc = MA_LATCH;
				src2_raw_dep.inst_op_type = (Op_Type)ma_inst.tr_entry.op_type;
				src2_raw_dep.inst_op_id = ma_inst.op_id;
			}

			// RAW dep between src3 reg and MA_LATCH dest
			if ( ma_inst.tr_entry.dest_needed &&
			     this_inst.tr_entry.src3_needed &&
			     this_inst.tr_entry.src3_reg == ma_inst.tr_entry.dest ) {
				src3_raw_dep.exists = true;
				src3_raw_dep.inst_latch_loc = MA_LATCH;
				src3_raw_dep.inst_op_type = (Op_Type)ma_inst.tr_entry.op_type;
				src3_raw_dep.inst_op_id = ma_inst.op_id;
			}
			// RAW dep between CC reg and MA_LATCH CC write
			if ( ma_inst.tr_entry.cc_write &&
			     this_inst.tr_entry.cc_read ) {
				cc_raw_dep.exists = true;
				cc_raw_dep.inst_latch_loc = MA_LATCH;
				cc_raw_dep.inst_op_type = (Op_Type)ma_inst.tr_entry.op_type;
				cc_raw_dep.inst_op_id = ma_inst.op_id;
			}
		}

		// Determine if stalling is necessary, or if we can forward
		bool src1_dep_causes_stall = false;
		bool src2_dep_causes_stall = false;
		bool src3_dep_causes_stall = false;
		bool cc_dep_causes_stall = false;

		if ( src1_raw_dep.exists ) {
			if ( src1_raw_dep.inst_latch_loc == EX_LATCH ) {
				src1_dep_causes_stall = !ENABLE_EXE_FWD;
			} else if ( src1_raw_dep.inst_latch_loc == MA_LATCH ) {
				src1_dep_causes_stall = !ENABLE_MEM_FWD;
			}
		}
		if ( src2_raw_dep.exists ) {
			if ( src2_raw_dep.inst_latch_loc == EX_LATCH ) {
				src2_dep_causes_stall = !ENABLE_EXE_FWD;
			} else if ( src2_raw_dep.inst_latch_loc == MA_LATCH ) {
				src2_dep_causes_stall = !ENABLE_MEM_FWD;
			}
		}
		if ( src3_raw_dep.exists ) {
			if ( src3_raw_dep.inst_latch_loc == EX_LATCH ) {
				src3_dep_causes_stall = !ENABLE_EXE_FWD;
			} else if ( src3_raw_dep.inst_latch_loc == MA_LATCH ) {
				src3_dep_causes_stall = !ENABLE_MEM_FWD;
			}
		}
		if ( cc_raw_dep.exists ) {
			if ( cc_raw_dep.inst_latch_loc == EX_LATCH ) {
				cc_dep_causes_stall = !ENABLE_EXE_FWD;
			} else if ( cc_raw_dep.inst_latch_loc == MA_LATCH ) {
				cc_dep_causes_stall = !ENABLE_MEM_FWD;
			}
		}

		// If we need to stall for any reason
		if ( src1_dep_causes_stall ||
		     src2_dep_causes_stall ||
		     src3_dep_causes_stall ||
		     cc_dep_causes_stall ) {
			p->pipe_latch[ID_LATCH][ii].valid = false; // insert a bubble in this stage
			p->pipe_latch[IF_LATCH][ii].stall = true;  // prevent fetch stage from grabbing more instructions
		}
	}
}

//--------------------------------------------------------------------//

void pipe_cycle_IF(Pipeline *p) {
	// TODO: UPDATE HERE (Part A, B)
	int ii, jj;
	Pipeline_Latch fetch_op;
	bool tr_read_success;

	for ( ii = 0; ii < PIPE_WIDTH; ii++ ) {
		// Prevent the next instruction fetch if ID stage has causes a stall
		if ( p->pipe_latch[IF_LATCH][ii].stall ) {
			continue;
		}

		pipe_get_fetch_op(p, &fetch_op);

		if ( BPRED_POLICY != -1 ) {
			pipe_check_bpred(p, &fetch_op);
		}
		p->pipe_latch[IF_LATCH][ii] = fetch_op;
	}
}

//--------------------------------------------------------------------//

void pipe_check_bpred(Pipeline *p, Pipeline_Latch *fetch_op) {
	// call branch predictor here, if mispred then mark in fetch_op
	// update the predictor instantly
	// stall fetch using the flag p->fetch_cbr_stall

	// TODO: UPDATE HERE (Part B)
}

//--------------------------------------------------------------------//