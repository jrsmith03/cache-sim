#include "parser.hpp"

#define KHz(num) ((num)*1000UL)
#define MHz(num) (KHz(num)*1000UL)
#define GHz(num) (MHz(num)*1000UL)

#define L1_CYCLE_DELAY 1
#define L2_CYCLE_DELAY 10
#define DRAM_CYCLE_DELAY 100

#define CLOCK_SPEED GHz(2)

typedef unsigned long u64;

struct Line { // combine these to one field if possible
	u64 tag;
	bool dirty;
	bool valid;
};

struct Set {
	Line* lines;
};

struct Cache {
	u64 capacity, associativity, block_size, num_sets;
	u64 latency, delay;
	u64 idle_power, running_power;
	u64 transfer_penalty;
	Set* sets;

	Cache(u64 capacity, u64 associativity, u64 block_size);
	void put(void* addr);

};
// 

Cache::Cache(u64 capacity, u64 associativity, u64 block_size) {
	this->capacity = capacity;
	// malloc in order to allocate space for sets and lines
}

// Fill in the mtadata within the cache. We do not actually care about the contents of the block.
void Cache::put(void* addr) {

}

int main(int argc, char* argv[]) {
	Cache l1d(1024, 2, 64);
	Cache l1i(1024, 2, 64);
	Cache l2(1024, 2, 64);
	Cache dram(1024, 2, 64);
	l1d.put((void*)0xfff7);

	Parser parser();
	TraceLine line;
	line = parser.next_line();
	while (line.valid) {
		// Main cache logic with metadata gets updated where.
		line = parser.next_line();
	}

	// We're done
}
