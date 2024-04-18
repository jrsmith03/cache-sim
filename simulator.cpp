#include "parser.hpp"
#include "memcontrol.hpp"
#include "cache.hpp"
#include <cstdio>

#include "simulator.hpp"

// Think of this main as the memory controller.
int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Include file name of trace to run\n");
		return -1;
	}

	// Maybe take input and output here.
	// Would be good to initialize caches from parser.
	// Memory is not byte addressable in this design. It is 64 byte addressable.
	std::vector<Cache> memory_hierarchy{
		Cache(KiB(32), 1, 64, ps(500), mW(500), W(1), J(0)), // l1d
		Cache(KiB(32), 1, 64, ps(500), mW(500), W(1), J(0)), // l1i
		Cache(KiB(256), 4, 64, ns(5), mW(800), W(2), pJ(5)), // l2
		Cache(GiB(8), 1, 64, ns(50), mW(800), W(4), pJ(640)) // dram
	};

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
		// Call into the Memory Controller to handle everything.
		switch (trace.instructions[--trace.last_ins].op) {
			case READ: {
				printf("read!\n");

				mem_read(0, memory_hierarchy, &trace.instructions[trace.last_ins].value, &trace.instructions[trace.last_ins].address, &energy);
				break;
			}

			case WRITE: {
				printf("write!\n");

				mem_write(0, memory_hierarchy, &trace.instructions[trace.last_ins].value, &trace.instructions[trace.last_ins].address, &energy);
				break;
			}

			case FETCH: {
				printf("fetch!\n");
				break;
			}

			case IGNORE: {
				printf("ignore!\n");
				break;
			}

			case FLUSH: {
				printf("flush!\n");
				break;
			}

		}

		// update state
		// energy += (l1d.idle_power + l1i.idle_power + l2.idle_power + dram.idle_power) * CYCLE_TIME;
		// time += CYCLE_TIME;
		trace.next_instr();
	}

	// // We're done print contents
}
