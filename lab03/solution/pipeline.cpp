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
 **********************************************************************/

#include "pipeline.h"
#include <cstdlib>
#include <cstring>

extern int32_t PIPE_WIDTH;
extern int32_t SCHED_POLICY;

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
		fetch_inst->mem_addr = trace.mem_addr; // samy

		fetch_inst->dr_tag = -1;
		fetch_inst->src1_tag = -1;
		fetch_inst->src2_tag = -1;
		fetch_inst->src1_ready = false;
		fetch_inst->src2_ready = false;
		fetch_inst->exe_wait_cycles = 0;

		fetch_inst->is_exception = trace.is_exception;
		fetch_inst->exception_handler_cost = trace.is_exception ? trace.exception_handler_cost : 0;

	} else {
		fe_latch->valid = false;
	}
	return;
}

void printQueue(std::queue<Inst_Info> q) {
	while ( !q.empty() ) {
		printf("%llu\n", q.front().inst_num);
		q.pop();
	}
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
extern int32_t NUM_ROB_ENTRIES;

void pipe_cycle_fetch(Pipeline *p) {
	int ii = 0;
	Pipe_Latch fetch_latch;

	for ( ii = 0; ii < PIPE_WIDTH; ii++ ) {
		if ( (p->FE_latch[ii].stall) || (p->FE_latch[ii].valid) ) { // Stall
			continue;

		} else if ( EXCEPTIONS && p->exception_state == HANDLING_EXCEPTION ) { // Handling exception
			p->remaining_exception_handler_cycles--;
			if ( p->remaining_exception_handler_cycles == 0 ) {
				p->exception_state = STARTING_RECOVERY;
			}
			break;
		} else if ( EXCEPTIONS && (p->exception_state == STARTING_RECOVERY || p->exception_state == IN_RECOVERY) ) { // recovery
			p->exception_state = IN_RECOVERY;
			p->FE_latch[ii].inst = p->recovery_window.front();
			p->recovery_window.pop();
			p->FE_latch[ii].valid = true;
			p->FE_latch[ii].stall = false;
			p->FE_latch[ii].inst.is_exception = false;
			if ( p->recovery_window.size() == 0 ) {
				p->exception_state = FINISHED_RECOVERY;
				p->last_recovery_inst = p->FE_latch[ii].inst.inst_num;
			}
		} else { // No Stall and Latch Empty
			pipe_fetch_inst(p, &fetch_latch);

			if ( EXCEPTIONS ) {
				// add this instruction to recovery window
				p->recovery_window.push(fetch_latch.inst);
			}
			// copy the op in FE LATCH
			p->FE_latch[ii] = fetch_latch;
		}
	}
}

//--------------------------------------------------------------------//

void pipe_cycle_decode(Pipeline *p) {
	int ii = 0;

	int jj = 0;

	static uint64_t start_inst_id = 1;

	if ( EXCEPTIONS && p->exception_state == STARTING_RECOVERY )
		start_inst_id = p->recovery_window.front().inst_num;

	// Loop Over ID Latch
	for ( ii = 0; ii < PIPE_WIDTH; ii++ ) {
		if ( (p->ID_latch[ii].stall == 1) || (p->ID_latch[ii].valid) ) { // Stall
			continue;
		} else {                                    // No Stall & there is Space in Latch
			for ( jj = 0; jj < PIPE_WIDTH; jj++ ) { // Loop Over FE Latch
				if ( p->FE_latch[jj].valid ) {
					if ( p->FE_latch[jj].inst.inst_num == start_inst_id ) { // In Order Inst Found
						p->ID_latch[ii] = p->FE_latch[jj];
						p->ID_latch[ii].valid = true;
						p->FE_latch[jj].valid = false;
						start_inst_id++;
						break;
					}
				}
			}
		}
	}
}

//--------------------------------------------------------------------//

void pipe_cycle_issue(Pipeline *p) {

	// insert new instruction(s) into ROB (rename)
	// every cycle up to PIPEWIDTH instructions issued

	// TODO: Find space in ROB and transfer instruction (valid = 1, exec = 0, ready = 0)
	// TODO: If src1/src2 is not remapped, set src1ready/src2ready
	// TODO: If src1/src is remapped, set src1tag/src2tag from RAT. Set src1ready/src2ready based on ready bit from ROB entries.
	// TODO: Set dr_tag
	int pipe_number, i, tag;
	for ( i = 0; i < PIPE_WIDTH; i++ ) {
		// TODO: If src1/src2 is remapped set src1tag, src2tag
		// TODO: If src1/src2 is not remapped marked as src ready
		uint64_t dummy = 0;
		dummy--;
		for ( pipe_number = 0; pipe_number < PIPE_WIDTH; pipe_number++ ) {
			if ( (p->ID_latch[pipe_number].valid) && (p->ID_latch[pipe_number].inst.inst_num < dummy) ) dummy = p->ID_latch[pipe_number].inst.inst_num;
		}

		for ( pipe_number = 0; pipe_number < PIPE_WIDTH; pipe_number++ ) {
			if ( p->ID_latch[pipe_number].valid && (p->ID_latch[pipe_number].inst.inst_num == dummy) ) {
				// TODO: Find space in ROB and set drtag as such if successful

				if ( p->ID_latch[pipe_number].inst.src1_reg > -1 )
					p->ID_latch[pipe_number].inst.src1_tag = RAT_get_remap(p->pipe_RAT, p->ID_latch[pipe_number].inst.src1_reg);
				else
					p->ID_latch[pipe_number].inst.src1_tag = -1;
				if ( p->ID_latch[pipe_number].inst.src2_reg > -1 )
					p->ID_latch[pipe_number].inst.src2_tag = RAT_get_remap(p->pipe_RAT, p->ID_latch[pipe_number].inst.src2_reg);
				else
					p->ID_latch[pipe_number].inst.src2_tag = -1;

				// TODO: If src1/src2 remapped and the ROB (tag) is ready then mark src ready
				if ( (p->ID_latch[pipe_number].inst.src1_tag > -1) && (ROB_check_ready(p->pipe_ROB, p->ID_latch[pipe_number].inst.src1_tag)) )
					p->ID_latch[pipe_number].inst.src1_tag = -1;
				if ( (p->ID_latch[pipe_number].inst.src2_tag > -1) && (ROB_check_ready(p->pipe_ROB, p->ID_latch[pipe_number].inst.src2_tag)) )
					p->ID_latch[pipe_number].inst.src2_tag = -1;

				if ( ROB_check_space(p->pipe_ROB) ) {
					tag = ROB_insert(p->pipe_ROB, p->ID_latch[pipe_number].inst);

					if ( p->ID_latch[pipe_number].inst.dest_reg > -1 )
						RAT_set_remap(p->pipe_RAT, p->ID_latch[pipe_number].inst.dest_reg, tag);

					p->ID_latch[pipe_number].stall = false;
					p->ID_latch[pipe_number].valid = false;
				} else {
					p->ID_latch[pipe_number].stall = true;
				}
			}
		}
		// FIXME: If there is stall, we should not do rename and ROB alloc twice
	}
}

//--------------------------------------------------------------------//

void pipe_cycle_schedule(Pipeline *p) {
	int pipe_number;
	// TODO: Implement two scheduling policies (SCHED_POLICY: 0 and 1)
	for ( pipe_number = 0; pipe_number < PIPE_WIDTH; pipe_number++ ) {
		if ( SCHED_POLICY == 0 ) {
			// inorder scheduling
			// Find all valid entries, if oldest is stalled then stop
			// Else send it out and mark it as scheduled

			Inst_Info dummy;
			dummy.inst_num = 0;
			dummy.inst_num--;

			for ( int i = 0; i < NUM_ROB_ENTRIES; i++ ) {
				if ( (p->pipe_ROB->ROB_Entries[i].valid) && (!(p->pipe_ROB->ROB_Entries[i].exec)) && ((p->pipe_ROB->ROB_Entries[i].inst.inst_num) < dummy.inst_num) ) {
					dummy = p->pipe_ROB->ROB_Entries[i].inst;
				}
			}

			if ( (dummy.src1_tag == -1) && (dummy.src2_tag == -1) ) {
				ROB_mark_exec(p->pipe_ROB, dummy);
				p->SC_latch[pipe_number].inst = dummy;
				p->SC_latch[pipe_number].valid = true;
			} else {
				p->SC_latch[pipe_number].valid = false;
			}
		}

		if ( SCHED_POLICY == 1 ) {
			// out of order scheduling
			// Find valid/unscheduled/src1ready/src2ready entry in ROB
			// Transfer it to SC_latch and mark that ROB entry as executed

			Inst_Info dummy;
			bool check_flag = false;
			dummy.inst_num = 0;
			dummy.inst_num--;
			for ( int i = 0; i < NUM_ROB_ENTRIES; i++ ) {
				if ( (p->pipe_ROB->ROB_Entries[i].valid) && (!(p->pipe_ROB->ROB_Entries[i].exec)) && ((p->pipe_ROB->ROB_Entries[i].inst.inst_num) < dummy.inst_num) &&
				     (p->pipe_ROB->ROB_Entries[i].inst.src1_tag == -1) && (p->pipe_ROB->ROB_Entries[i].inst.src2_tag == -1) ) {

					dummy = p->pipe_ROB->ROB_Entries[i].inst;
					check_flag = true;
				}
			}
			if ( check_flag ) {
				ROB_mark_exec(p->pipe_ROB, dummy);
				p->SC_latch[pipe_number].inst = dummy;
				p->SC_latch[pipe_number].valid = true;
			} else {
				p->SC_latch[pipe_number].valid = false;
			}
		}
	}
}

//--------------------------------------------------------------------//

void pipe_cycle_writeback(Pipeline *p) {

	// TODO: Go through all instructions out of EXE latch
	// TODO: Writeback to ROB (using wakeup function)
	// TODO: Update the ROB, mark ready, and update Inst Info in ROB

	for ( int i = 0; i < MAX_WRITEBACKS; i++ ) {
		if ( p->EX_latch[i].valid ) {

			ROB_mark_ready(p->pipe_ROB, p->EX_latch[i].inst);

			for ( int j = 0; j < NUM_ROB_ENTRIES; j++ ) {
				if ( (p->EX_latch[i].inst.inst_num == p->pipe_ROB->ROB_Entries[j].inst.inst_num) && (p->pipe_ROB->ROB_Entries[j].valid) )
					ROB_wakeup(p->pipe_ROB, j);
			}

			// ROB_remove(p -> pipe_ROB, p -> EX_latch[i].inst);
			p->EX_latch[i].valid = false;
		}
	}
}
//--------------------------------------------------------------------//

void pipe_cycle_commit(Pipeline *p) {
	Inst_Info dummy;

	// TODO: check the head of the ROB. If ready commit (update stats)
	// TODO: Deallocate entry from ROB
	int pipe_number, tag;
	for ( pipe_number = 0; pipe_number < PIPE_WIDTH; pipe_number++ ) {
		if ( ROB_check_head(p->pipe_ROB) ) {
			dummy = ROB_remove_head(p->pipe_ROB);
			p->stat_retired_inst++;

			// TODO: Update RAT after checking if the mapping is still valid
			tag = RAT_get_remap(p->pipe_RAT, dummy.dest_reg);

			if ( (tag > -1) && (p->pipe_ROB->ROB_Entries[tag].inst.inst_num == dummy.inst_num) ) {
				RAT_reset_entry(p->pipe_RAT, dummy.dest_reg);
			}

			if ( dummy.inst_num >= p->halt_inst_num ) p->halt = true;

			if ( EXCEPTIONS ) {
				int32_t exception_cycle_count = dummy.is_exception ? dummy.exception_handler_cost : 0;

				if ( exception_cycle_count != 0 ) {
					p->stat_retired_inst--;
					// flush
					ROB_flush(p->pipe_ROB);
					RAT_flush(p->pipe_RAT);
					EXEQ_flush(p->pipe_EXEQ);
					// invalidate all instructions currently in pipeline
					for ( pipe_number = 0; pipe_number < PIPE_WIDTH; pipe_number++ ) {
						p->FE_latch[pipe_number].valid = false;
						p->FE_latch[pipe_number].stall = false;
						p->ID_latch[pipe_number].valid = false;
						p->ID_latch[pipe_number].stall = false;
						p->SC_latch[pipe_number].valid = false;
						p->SC_latch[pipe_number].stall = false;
						p->EX_latch[pipe_number].valid = false;
						p->EX_latch[pipe_number].stall = false;
					}
					p->exception_state = HANDLING_EXCEPTION;
					p->remaining_exception_handler_cycles = exception_cycle_count;
				}
				if ( (p->exception_state == FINISHED_RECOVERY && p->last_recovery_inst + 1 == p->stat_retired_inst) || (p->exception_state == TRACE_FETCH) ) {
					p->exception_state = TRACE_FETCH;
					p->recovery_window.pop();
				}
			}
		}
	}
}

//--------------------------------------------------------------------//
