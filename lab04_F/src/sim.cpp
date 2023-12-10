 /*************************************************************************
 * File         : sim.c
 * Author       : Moinuddin K. Qureshi
 * Date         : 23rd March 2015
 * Description  : CMP Memory System Lab 4 of ECE4100/ECE6100
 *************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include "types.h"
#include "memsys.h"
#include "core.h"

#define PRINT_DOTS   1
#define DOT_INTERVAL 100000

/***************************************************************************
 * Globals
 **************************************************************************/

MODE        SIM_MODE        = SIM_MODE_A;
uint64_t       CACHE_LINESIZE  = 64;
uint64_t       REPL_POLICY     = 0; // 0:LRU 1:LFU+MRU 2:SWP (Part E)  

uint64_t       DCACHE_SIZE     = 32*1024;
uint64_t       DCACHE_ASSOC    = 8;

uint64_t       ICACHE_SIZE     = 32*1024;
uint64_t       ICACHE_ASSOC    = 8;

uint64_t       L2CACHE_SIZE    = 1024*1024;
uint64_t       L2CACHE_ASSOC   = 16;
uint64_t       L2CACHE_REPL    = 0;

uint64_t       SWP_CORE0_WAYS  = 0;

uint64_t       NUM_CORES       = 1;

uint64_t	   COHERENCE       = 0;

bool 	    DRAM_PAGE_POLICY = 0;

/***************************************************************************************
 * Functions
 ***************************************************************************************/
void print_dots(void);
void die_usage();
void die_message(const char* msg);
void get_params(int argc, char** argv);
void print_stats();

/***************************************************************************************
 * Globals
 ***************************************************************************************/

Memsys* memsys;
Core* core[MAX_CORES];
char trace_filename[MAX_CORES][1024];
uint64_t last_printdot_cycle;
unsigned long long cycle;

/***************************************************************************************
 * Main
 ***************************************************************************************/
int main(int argc, char** argv)
{
    srand(42);

    get_params(argc, argv);

    assert(NUM_CORES<=MAX_CORES);

    //---- Initialize the system
    memsys = memsys_new();

    for (uint i=0; i<NUM_CORES; i++) {
		core[i] = core_new(memsys,trace_filename[i],i);
    }

    print_dots();

    //--------------------------------------------------------------------
    // -- Iterate until all cores are done
    //--------------------------------------------------------------------
    bool all_cores_done = 0;

    while(!all_cores_done) {
    	all_cores_done=1;
    	for (uint i=0; i<NUM_CORES; i++){
			core_cycle(core[i]);
			all_cores_done &= core[i]->done;
      	}

      	if (cycle - last_printdot_cycle >= DOT_INTERVAL) {
			print_dots();
      	}

      	cycle++;
    }

    print_stats();
    return 0;
}

//--------------------------------------------------------------------
// -- Print statistics
//--------------------------------------------------------------------

void print_stats(){

  printf("\n");
  printf("\nCYCLES      \t\t\t : %10llu", cycle);

  for (uint i=0; i<NUM_CORES; i++) {
    core_print_stats(core[i]);
  }

  memsys_print_stats(memsys);

  printf("\n\n");
}

//--------------------------------------------------------------------
// -- Print Hearbeats
//--------------------------------------------------------------------


void print_dots(){
	uint32_t LINE_INTERVAL = 50 *  DOT_INTERVAL;

	last_printdot_cycle = cycle;

	if (!PRINT_DOTS) {
		return;
	}

 	if (cycle % LINE_INTERVAL == 0) {
		printf("\n%4llu M\t", cycle/1000000);
		fflush(stdout);
	}
    else{
		printf(".");
		fflush(stdout);
    }

}


//--------------------------------------------------------------------
// -- Usage Menu
//--------------------------------------------------------------------

void die_usage(){
    printf("Usage : sim [-option <value>] trace_0 <trace_1> \n");
    printf("   Options\n");
    printf("      -mode            <num>    Set mode of the simulator[1:PartA, 2:PartB, 3:PartC 4:PartD]  (Default: 1)\n");
    printf("      -linesize        <num>    Set cache linesize for all caches (Default:64)\n");
    printf("      -repl            <num>    Set replacement policy for L1 cache [0:LRU,1:LFU+MRU] (Default:0)\n");
    printf("      -DsizeKB         <num>    Set capacity in KB of the the Level 1 DCACHE (Default:32 KB)\n");
    printf("      -Dassoc          <num>    Set associativity of the the Level 1 DCACHE (Default:8)\n");
    printf("      -L2sizeKB        <num>    Set capacity in KB of the unified Level 2 cache (Default: 512 KB)\n");
    printf("      -L2repl          <num>    Set replacement policy for L2 cache [0:LRU,1:LFU+MRU,2:SWP] (Default:0)\n");
    printf("      -SWP_core0ways   <num>    Set static quota for core_0 for SWP (Default:1)\n");
    printf("      -dram_policy     <num>    Set DRAM page policy [0:Open Page Policy, 1: Close Page Policy](Default:0)\n");
	printf("	  -coherence	   Enable MSI Coherence protocol for the given memory system.\n");
    exit(0);
}

//--------------------------------------------------------------------
// -- Print Error Message and Die
//--------------------------------------------------------------------


void die_message(const char * msg){
    printf("Error! %s. Exiting...\n", msg);
    exit(1);
}

//--------------------------------------------------------------------
// -- Read Parameters from Command Line
//--------------------------------------------------------------------

void get_params(int argc, char** argv){
	int num_trace_filename = 0;

	if (argc < 2) {
	die_usage();
	}

    //--------------------------------------------------------------------
    // -- Get command line options
    //--------------------------------------------------------------------
    for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "-help")) {
				die_usage();
			}
			else if (!strcmp(argv[i], "-mode")) {
				if (i < argc - 1) {
					SIM_MODE = (MODE)atoi(argv[i+1]);
					i++;
				}
			}
			else if (!strcmp(argv[i], "-linesize")) {
				if (i < argc - 1) {
					CACHE_LINESIZE = atoi(argv[i+1]);
					i++;
				}
			}
			else if (!strcmp(argv[i], "-repl")) {
				if (i < argc - 1) {
					REPL_POLICY = atoi(argv[i+1]);
					i++;
				}
			}
			else if (!strcmp(argv[i], "-DsizeKB")) {
				if (i < argc - 1) {
					DCACHE_SIZE = atoi(argv[i+1])*1024;
					i++;
				}
			}
			else if (!strcmp(argv[i], "-Dassoc")) {
				if (i < argc - 1) {
					DCACHE_ASSOC = atoi(argv[i+1]);
					i++;
				}
			}
			else if (!strcmp(argv[i], "-L2sizeKB")) {
				if (i < argc - 1) {
					L2CACHE_SIZE = atoi(argv[i+1])*1024;
					i++;
				}
			}
			else if (!strcmp(argv[i], "-L2repl")) {
				if (i < argc - 1) {
					L2CACHE_REPL = atoi(argv[i+1]);
					i++;
				}
			}
			else if (!strcmp(argv[i], "-SWP_core0ways")) {
				if (i < argc - 1) {
					SWP_CORE0_WAYS = atoi(argv[i+1]);
					i++;
				}
			}
			else if (!strcmp(argv[i], "-dram_policy")) {
				if (i < argc - 1) {
					DRAM_PAGE_POLICY = atoi(argv[i+1]);
					i++;
				}
			}
			else {
				char msg[256];
				sprintf(msg, "Invalid option %s", argv[i]);
				die_message(msg);
			}
		}
		else if (num_trace_filename < MAX_CORES) {
			strcpy(trace_filename[num_trace_filename], argv[i]);
			num_trace_filename++;
			NUM_CORES = num_trace_filename;
		}
		else {
			char msg[256];
			sprintf(msg, "Invalid option %s, got filename %s", argv[i], trace_filename[NUM_CORES]);
			die_message(msg);
		}
    }

    //--------------------------------------------------------------------
    // Error checking
    //--------------------------------------------------------------------
    if (num_trace_filename == 0) {
		die_message("Must provide at least one trace file");
    }

}
