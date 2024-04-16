#include "parser.hpp"
#include "memcontrol.hpp"
#include "cache.hpp"

#include "simulator.hpp"

#define KHz(num) ((num)*1000UL)
#define MHz(num) (KHz(num)*1000UL)
#define GHz(num) (MHz(num)*1000UL)

#define KiB(num) ((num)*1024UL)
#define MiB(num) (KiB(num)*1024UL)
#define GiB(num) (MiB(num)*1024UL)


// Think of this main as the memory controller.
int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Include file name of trace to run\n");
		return -1;
	}

	// Maybe take input and output here.
	// Would be good to initialize caches from parser.
	// Memory is not byte addressable in this design. It is 64 byte addressable.
	std::vector<Cache> memory_hierarchy{(Cache l1d(KiB(32), 1, 64, ps(500), mW(500), W(1), J(0)),
			Cache l1i(KiB(32), 1, 64, ps(500), mW(500), W(1), J(0)),
			Cache l2(KiB(256), 4, 64, ns(5), mW(800), W(2), pJ(5)),
			Cache dram(GiB(8), 1, 64, ns(50), mW(800), W(4), pJ(640)))};

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
		switch (trace.instructions[trace.last_ins].op) {
			case READ: {
				mem_read(0, &energy, L1d, memory_hierarchy ,(void*)trace.instructions[trace.last_ins].value, 
							(void*)trace.instructions[trace.last_ins].address);
			}
			case WRITE: {
				mem_write(&energy, memory_hierarchy, trace);
			}

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
