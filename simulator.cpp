#include "parser.hpp"
#include "cache.hpp"
#include <cstdio>

#define KHz(num) ((num)*1000UL)
#define MHz(num) (KHz(num)*1000UL)
#define GHz(num) (MHz(num)*1000UL)

#define KiB(num) ((num)*1024UL)
#define MiB(num) (KiB(num)*1024UL)
#define GiB(num) (MiB(num)*1024UL)

using Freq = u64;
const Freq CLOCK_SPEED = GHz(2);
const Time CYCLE_TIME = s(1) / CLOCK_SPEED;

// Think of this main as the memory controller.
int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Include file name of trace to run\n");
		return -1;
	}

	// Maybe take input and output here.
	// Would be good to initialize caches from parser.
	// Memory is not byte addressable in this design. It is 64 byte addressable.
	Cache dram(GiB(8), 1, 64, ns(50), mW(800), W(4), pJ(640));
	Cache l2(KiB(256), 4, 64, ns(5), mW(800), W(2), pJ(5));
	Cache l1d(KiB(32), 1, 64, ps(500), mW(500), W(1), J(0));
	Cache l1i(KiB(32), 1, 64, ps(500), mW(500), W(1), J(0));

	Joule energy = 0;
	Time time = 0;
	Trace trace(argv[1]);
	if (trace.trace_fd == -1) {
		printf("error: invalid filename\n");
		return -1;
	}
	int nums = 0;


	trace.next_instr();
	// This implementation has a single memory controller which is omnipotent and
	// controls the flow
	while (trace.has_next_instr) {

		// Switch based on operation from parser.
		switch (trace.instructions[trace.last_ins].op) {
			case READ: {
				Eviction status = l1d.read((void*)trace.instructions[trace.last_ins].address); // Updates state of reg
				switch (status) {
					case HIT: {
						// Hit. Simple case
						energy += (l1d.running_power - l1d.idle_power) * CYCLE_TIME;
					}
					case DIRTY: {
						// TODO: I stopped here. Not thrilled with how this while loop is
						// turning out. May change Cache API to only have `is_hit`, `put`,
						// etc. 
					}
					case CLEAN:
				}
			}
			case WRITE:
			case FETCH:
			case IGNORE:
			case FLUSH:
		}

		// update state
		energy += (l1d.idle_power + l1i.idle_power + l2.idle_power + dram.idle_power) * CYCLE_TIME;
		time += CYCLE_TIME;
		trace.next_instr();
	}

	// // We're done print contents
}
