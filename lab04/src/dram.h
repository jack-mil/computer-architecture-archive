#ifndef DRAM_H
#define DRAM_H

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "types.h"

//////////////////////////////////////////////////////////////////
// Define the Data structures here with the correct field (Refer to Appendix B for more details)
//////////////////////////////////////////////////////////////////

#define DRAM_BANKS 16
#define ROW_BUFFER_SIZE 1024

// DRAM delay latencies (cycles)
#define LATENCY_RAS 45 // (RAS) Row address strobe
#define LATENCY_CAS 45 // Column Address Strobe
#define LATENCY_PRE 45 // Pre-charge penalty
#define LATENCY_BUS 10 // Roundtrip bus latency

// 0: Open Page Policy, 1: Close Page Policy
enum Page_Policy {
	OPEN_PAGE = 0,
	CLOSED_PAGE = 1
};

// Structure to represent the row buffer for a particular DRAM bank
struct Row_Buffer {
	bool valid;
	uint64_t row_id; // Each bank has some number of rows
};

class Dram {
  public:
	uint64_t stat_read_access;
	uint64_t stat_write_access;
	uint64_t stat_read_delay;
	uint64_t stat_write_delay;

	std::vector<Row_Buffer> banks; // Array of 16 banks, each with a row buffer
	Page_Policy page_policy;       // 0:Open Page Policy, 1: Close Page Policy

	// Construct a DRAM object with 16 banks and the given page policy
	Dram(Page_Policy policy) : banks(DRAM_BANKS), page_policy(policy){};

  private:
};

Dram *dram_new(uint8_t policy);
void dram_print_stats(Dram *dram);
uint64_t dram_access(Dram *dram, Addr lineaddr, bool is_dram_write);
uint64_t dram_access_mode_CDE(Dram *dram, Addr lineaddr, bool is_dram_write);

#endif // DRAM_H
