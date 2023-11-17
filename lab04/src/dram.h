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
  private:
	// the {} makes default initilization
	uint64_t m_stat_read_access{};
	uint64_t m_stat_write_access{};
	uint64_t m_stat_buffer_miss{};
	uint64_t m_stat_read_delay{};
	uint64_t m_stat_write_delay{};

	std::vector<Row_Buffer> m_banks; // Array of 16 banks, each with a row buffer
	Page_Policy m_page_policy;       // 0:Open Page Policy, 1: Close Page Policy

  public:
	// Construct a DRAM object with 16 banks and the given page policy
	Dram(Page_Policy policy) : m_banks(DRAM_BANKS), m_page_policy(policy){};
	// Construct a DRAM object with 16 banks. 0:Open Page Policy, 1: Close Page Policy
	Dram(uint policy) : m_banks(DRAM_BANKS), m_page_policy((Page_Policy)policy){};

	// Get the delay to access cache block
	uint64_t access(Addr lineaddr, bool is_dram_write);
	// Get the delay to access cache block according to page policy
	uint64_t access_mode_CDE(Addr lineaddr, bool is_dram_write);
	// Print information about the access of this object. For grading
	void print_stats();
};

#endif // DRAM_H
