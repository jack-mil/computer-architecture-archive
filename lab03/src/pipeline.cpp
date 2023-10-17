/***********************************************************************
 * File         : pipeline.cpp
 * Author       : Moinuddin K. Qureshi
 * Date         : 19th February 2014
 * Description  : Out of Order Pipeline for Lab3 ECE 6100

 * Update       : Shravan Ramani, Tushar Krishna, 27th Sept, 2015
 *
 * Version 2.0 (Updates for precise exceptions)
 * Author       : Shaan Dhawan
 * Date         : 23rd September 2022
 * Description  : Out of Order Pipeline for Lab3 CS4290/CS6290/ECE4100/ECE6100 [Fall 2022]
 *
 * Version 3.0 (Updates for FA-LRU Cache)
 * Author       : Samy Amer
 * Date         : 19 September 2023
 * Description  : Out of Order Pipeline for Lab3 CS4290/CS6290/ECE4100/ECE6100 [Fall 2023]
 *
 * Lab 03 Assignment Submission
 * Author       : Jackson Miller
 * Data         : 14 October 2023
 * Description  : Completed Assignment
 **********************************************************************/

#include "pipeline.h"
#include <cstdlib>
#include <cstring>

extern int32_t PIPE_WIDTH;
extern int32_t SCHED_POLICY;

extern int32_t NUM_ROB_ENTRIES;
extern int32_t EXCEPTIONS;

/**********************************************************************
 * Support Function: Read 1 Trace Record From File and populate Fetch Inst
 **********************************************************************/

void pipe_fetch_inst(Pipeline *p, Pipe_Latch *fe_latch) {
	static int halt_fetch = 0;
	uint8_t bytes_read = 0;
	Trace_Rec trace;
	if ( halt_fetch != 1 ) {
		bytes_read = fread(&trace, 1, sizeof(Trace_Rec), p->tr_file);
		Inst_Info *fetch_inst = &(fe_latch->inst);
		// check for end of trace
		// Send out a dummy terminate op
		if ( bytes_read < sizeof(Trace_Rec) ) {
			p->halt_inst_num = p->inst_num_tracker;
			halt_fetch = 1;
			fe_latch->valid = true;
			fe_latch->inst.dest_reg = -1;
			fe_latch->inst.src1_reg = -1;
			fe_latch->inst.src1_reg = -1;
			fe_latch->inst.inst_num = -1;
			fe_latch->inst.op_type = 4;
			return;
		}

		// got an instruction ... hooray!
		fe_latch->valid = true;
		fe_latch->stall = false;
		p->inst_num_tracker++;
		fetch_inst->inst_num = p->inst_num_tracker;
		fetch_inst->op_type = trace.op_type;

		fetch_inst->dest_reg = trace.dest_needed ? trace.dest : -1;
		fetch_inst->src1_reg = trace.src1_needed ? trace.src1_reg : -1;
		fetch_inst->src2_reg = trace.src2_needed ? trace.src2_reg : -1;

		fetch_inst->mem_addr = trace.mem_addr;

		fetch_inst->dr_tag = -1;
		fetch_inst->src1_tag = -1;
		fetch_inst->src2_tag = -1;
		fetch_inst->src1_ready = false;
		fetch_inst->src2_ready = false;
		fetch_inst->exe_wait_cycles = 0;
	} else {
		fe_latch->valid = false;
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

	if ( EXCEPTIONS ) {
		p->exception_state = TRACE_FETCH;
		new (&p->recovery_window) std::queue<Inst_Info>;
	}

	p->pipe_RAT = RAT_init();
	p->pipe_ROB = ROB_init();
	p->pipe_EXEQ = EXEQ_init();
	p->pipeline_cache = init_cache();
	p->tr_file = tr_file_in;
	p->halt_inst_num = ((uint64_t)-1) - 3;
	int ii = 0;
	for ( ii = 0; ii < PIPE_WIDTH; ii++ ) { // Loop over No of Pipes
		p->FE_latch[ii].valid = false;
		p->ID_latch[ii].valid = false;
		p->EX_latch[ii].valid = false;
		p->SC_latch[ii].valid = false;
	}
	return p;
}

/**********************************************************************
 * Print the pipeline state (useful for debugging)
 **********************************************************************/

void pipe_print_state(Pipeline *p) {
	std::cout << "--------------------------------------------" << std::endl;
	std::cout << "cycle count : " << p->stat_num_cycle << " retired_instruction : " << p->stat_retired_inst << std::endl;
	uint8_t latch_type_i = 0;
	uint8_t width_i = 0;
	for ( latch_type_i = 0; latch_type_i < 4; latch_type_i++ ) {
		switch ( latch_type_i ) {
			case 0:
				printf(" FE: ");
				break;
			case 1:
				printf(" ID: ");
				break;
			case 2:
				printf(" SCH: ");
				break;
			case 3:
				printf(" EX: ");
				break;
			default:
				printf(" -- ");
		}
	}
	printf("\n");
	for ( width_i = 0; width_i < PIPE_WIDTH; width_i++ ) {
		if ( p->FE_latch[width_i].valid == true ) {
			printf("  %d  ", (int)p->FE_latch[width_i].inst.inst_num);
		} else {
			printf(" --  ");
		}
		if ( p->ID_latch[width_i].valid == true ) {
			printf("  %d  ", (int)p->ID_latch[width_i].inst.inst_num);
		} else {
			printf(" --  ");
		}
		if ( p->SC_latch[width_i].valid == true ) {
			printf("  %d  ", (int)p->SC_latch[width_i].inst.inst_num);
		} else {
			printf(" --  ");
		}
		if ( p->EX_latch[width_i].valid == true ) {
			for ( int ii = 0; ii < MAX_WRITEBACKS; ii++ ) {
				if ( p->EX_latch[ii].valid )
					printf("  %d  ", (int)p->EX_latch[ii].inst.inst_num);
			}
		} else {
			printf(" --  ");
		}
		printf("\n");
	}
	printf("\n");

	RAT_print_state(p->pipe_RAT);
	EXEQ_print_state(p->pipe_EXEQ);
	ROB_print_state(p->pipe_ROB);
}

/**********************************************************************
 * Pipeline Main Function: Every cycle, cycle the stage
 **********************************************************************/

void pipe_cycle(Pipeline *p) {
	p->stat_num_cycle++;

	pipe_cycle_commit(p);
	pipe_cycle_writeback(p);
	pipe_cycle_exe(p);
	pipe_cycle_schedule(p);
	pipe_cycle_issue(p);
	pipe_cycle_decode(p);
	pipe_cycle_fetch(p);

	// pipe_print_state(p);
	// printf("");
}

//--------------------------------------------------------------------//

void pipe_cycle_exe(Pipeline *p) {

	int ii;

	//---------Handling exe for multicycle operations is complex, and uses EXEQ

	// All valid entries from SC get into exeq

	for ( ii = 0; ii < PIPE_WIDTH; ii++ ) {
		if ( p->SC_latch[ii].valid ) {
			EXEQ_insert(p->pipe_EXEQ, p->SC_latch[ii].inst, p->pipeline_cache);
			p->SC_latch[ii].valid = false;
		}
	}

	// Cycle the exeq, to reduce wait time for each inst by 1 cycle
	EXEQ_cycle(p->pipe_EXEQ);

	// Transfer all finished entries from EXEQ to EX_latch
	int index = 0;

	while ( 1 ) {
		if ( EXEQ_check_done(p->pipe_EXEQ) ) {
			p->EX_latch[index].valid = true;
			p->EX_latch[index].stall = false;
			p->EX_latch[index].inst = EXEQ_remove(p->pipe_EXEQ);
			index++;
		} else { // No More Entry in EXEQ
			break;
		}
	}
}

/**********************************************************************
 * -----------  DO NOT MODIFY THE CODE ABOVE THIS LINE ----------------
 **********************************************************************/

void pipe_cycle_fetch(Pipeline *p) {
	int ii = 0;
	Pipe_Latch fetch_latch;

	for ( ii = 0; ii < PIPE_WIDTH; ii++ ) {
		if ( (p->FE_latch[ii].stall) || (p->FE_latch[ii].valid) ) { // Stall
			continue;

		}

		// TODO: Conditional Logic for Exception Handling

		else { // No Stall and Latch Empty
			pipe_fetch_inst(p, &fetch_latch);

			if ( EXCEPTIONS ) {
			}
			// copy the op in FE LATCH
			p->FE_latch[ii] = fetch_latch;
		}
	}
}

//--------------------------------------------------------------------//

void pipe_cycle_decode(Pipeline *p) {

	// Static means this variable will keep value
	// across function calls. Ensures that all instructions
	// are decoded in order.
	static uint64_t start_inst_id = 1;

	// Loop Over ID Latch
	for ( int lane = 0; lane < PIPE_WIDTH; lane++ ) {
		if ( (p->ID_latch[lane].stall == 1) || (p->ID_latch[lane].valid) ) { // Stall
			continue;
		}
		// No Stall & there is Space in Latch
		for ( int jj = 0; jj < PIPE_WIDTH; jj++ ) {
			// Loop over all full FE Latches
			if ( !p->FE_latch[jj].valid ) {
				continue;
			}
			// Find the next in order instruction
			if ( p->FE_latch[jj].inst.inst_num != start_inst_id ) {
				continue;
			}
			p->ID_latch[lane] = p->FE_latch[jj];
			p->ID_latch[lane].valid = true;
			p->FE_latch[jj].valid = false;
			start_inst_id++; // Increment the static counter. Persists across function calls
			break;
		}
	}
}

//--------------------------------------------------------------------//

void pipe_cycle_issue(Pipeline *p) {

	// insert new instruction(s) into ROB (rename)
	// every cycle up to PIPEWIDTH instructions issued

	// TODO: Find space in ROB and transfer instruction (valid = 1, exec = 0, ready = 0)
	// TODO: If src1/src2 is not remapped, set src1ready/src2ready
	// TODO: If src1/src2 is remapped, set src1tag/src2tag from RAT. Set src1ready/src2ready based on ready bit from ROB entries.
	// TODO: Set dr_tag

	for ( int lane = 0; lane < PIPE_WIDTH; lane++ ) {
		// Skip this lane if the decode latch is invalid
		if ( !p->ID_latch[lane].valid ) {
			continue;
		}
		if ( !ROB_check_space(p->pipe_ROB) ) {
			// ROB is full. Have to stall the decode stage (?)
			// p->ID_latch[lane].stall = true;
			// Decode stage will stall because latch will be full (valid)
			continue;
		}

		Inst_Info inst = p->ID_latch[lane].inst;

		int src1_map = RAT_get_remap(p->pipe_RAT, inst.src1_reg);
		if ( src1_map < 0 ) {
			// When there is no existing remapped name, then assume data is present in ARF
			inst.src1_ready = true;
		} else {
			// Get the ready bit from remapped ROB entry
			inst.src1_ready = p->pipe_ROB->ROB_Entries[src1_map].ready;
		}
		// tag always set to the remap return, even if ready or not needed (-1)
		inst.src1_tag = src1_map;

		int src2_map = RAT_get_remap(p->pipe_RAT, inst.src2_reg);
		if ( src2_map < 0 ) {
			// When there is no existing remapped name, then assume data is present in ARF
			inst.src2_ready = true;
		} else {
			// Get the ready bit from remapped ROB entry
			inst.src2_ready = p->pipe_ROB->ROB_Entries[src2_map].ready;
		}
		// tag always set to the remap return, even if ready or not needed (-1)
		inst.src2_tag = src2_map;

		// Insert the instruction into free space in ROB, save dest tag
		int rob_tag = ROB_insert(p->pipe_ROB, inst);

		// And rename the destination register to rob tag
		if(inst.dest_reg >= 0){
			RAT_set_remap(p->pipe_RAT, inst.dest_reg, rob_tag);
			p->pipe_ROB->ROB_Entries[rob_tag].inst.dr_tag = rob_tag;
		}

		p->ID_latch[lane].valid = false; // "Removes" the inst from the latch
	}
}

//--------------------------------------------------------------------//

void pipe_cycle_schedule(Pipeline *p) {

	// select instruction(s) to Execute
	// every cycle up to PIPEWIDTH instructions scheduled

	// TODO: Implement two scheduling policies (SCHED_POLICY: 0 and 1)

	if ( SCHED_POLICY == 0 ) {
		// inorder scheduling
		// Find all valid entries, if oldest is stalled then stop
		// Else mark it as ready to execute and send to SC_latch

		// Start out search for ready instructions at ROB head (oldest instruction)
		// Remember progress while looping over pipeline lanes
		int tag;

		// Try to fill SC_Latch for each lane of the pipeline
		for ( int lane = 0; lane < PIPE_WIDTH; lane++ ) {

			// Starting at the oldest, search for an eligible instruction
			for ( tag = p->pipe_ROB->head_ptr; tag < NUM_ROB_ENTRIES; tag = (tag + 1) % NUM_ROB_ENTRIES ) {
				// Quit search immediately if head (oldest instruction) is not valid
				if ( !p->pipe_ROB->ROB_Entries[tag].valid ) {
					p->SC_latch[lane].valid = false;
					break;
				}

				// Skip over instructions currently executing, might be case for superscalar
				if ( p->pipe_ROB->ROB_Entries[tag].exec )
					continue;

				// src1 and src2 must be ready in order to execute
				if ( !p->pipe_ROB->ROB_Entries[tag].inst.src1_ready ||
				     !p->pipe_ROB->ROB_Entries[tag].inst.src2_ready ) {
					p->SC_latch[lane].valid = false; // Insert a bubble (stall) this lane
					break;
				}

				// Set this entry as executing, and fill the next schedule latch
				p->pipe_ROB->ROB_Entries[tag].exec = true;
				p->SC_latch[lane].inst = p->pipe_ROB->ROB_Entries[tag].inst;
				p->SC_latch[lane].valid = true;

				// tag = (tag + 1) % NUM_ROB_ENTRIES; // need to manually increment the counter...
				break;                             // so search resumes on correct entry (break skips update action)
			}

			// Try to fill next lane of superscalar
		}
	}

	if ( SCHED_POLICY == 1 ) {
		// out of order scheduling
		// Find valid + src1ready + src2ready + !exec entries in ROB
		// Mark ROB entry as ready to execute  and transfer instruction to SC_latch

		/* for ( int tag = 0; tag < NUM_ROB_ENTRIES; tag++ ) {
		    // Skip empty (invalid) and already executed instructions
		    if ( !p->pipe_ROB->ROB_Entries[tag].valid ) {
		        continue;
		    }
		    if ( p->pipe_ROB->ROB_Entries[tag].exec ) {
		        continue;
		    }

		    if ( p->pipe_ROB->ROB_Entries[tag].inst.src1_ready &&
		         p->pipe_ROB->ROB_Entries[tag].inst.src2_ready ) {
		                p->pipe_ROB->ROB_Entries[tag].exec = true;
		                p->SC_latch[??]
		    }
		}

		ROB_mark_exec(p->pipe_ROB, inst) */
	}
}

//--------------------------------------------------------------------//

void pipe_cycle_writeback(Pipeline *p) {

	for ( int lane = 0; lane < MAX_WRITEBACKS; lane++ ) {
		// Skip invalid (empty) latches
		if ( !p->EX_latch[lane].valid ) {
			continue;
		}

		// Set this instruction as ready
		// Also broadcasts results to other waiting entries
		ROB_mark_ready(p->pipe_ROB, p->EX_latch[lane].inst);

		// Remove the instruction from this latch (needed)?
		p->EX_latch[lane].valid = false;
	}

	// TODO: Go through all instructions out of EXE latch
	// TODO: Writeback to ROB (using wakeup function)
	// TODO: Update the ROB, mark ready, and update Inst Info in ROB
	// * (What inst info needs updating here? just the part covered in writeback?)
}

//--------------------------------------------------------------------//

void pipe_cycle_commit(Pipeline *p) {

	// TODO: Flush pipeline when exception is found

	// Can commit upto PIPE_WIDTH instructions per cycle
	for ( int i = 0; i < PIPE_WIDTH; i++ ) {

		// Ensure that the ROB contains a instruction ready to commit
		if ( !ROB_check_head(p->pipe_ROB) ) {
			break;
		}

		// Deallocate and free space in the ROB, increments head_ptr
		Inst_Info commited = ROB_remove_head(p->pipe_ROB);

		// Instruction is now considered 'commited'
		p->stat_retired_inst++;

		// Reset the RAT entry for this rename, because data is in ARF now
		if ( commited.dest_reg >= 0 && RAT_get_remap(p->pipe_RAT, commited.dest_reg) == commited.dr_tag ) {
			RAT_reset_entry(p->pipe_RAT, commited.dest_reg);
		}

		// If this was the last instruction, set the halt condition
		if ( commited.inst_num >= p->halt_inst_num ) {
			p->halt = true;
			break;
		}
	}
}
