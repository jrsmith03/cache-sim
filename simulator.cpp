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
    Joule energy = 0;
    Time time = 0; // NOTE(Nate): Needed, but commented to solve compile err
    Trace trace(argv[1]);
    if (trace.trace_fd == -1) {
        printf("error: invalid filename\n");
        return -1;
    }
    Cache dram = Cache(GiB(8), 1, 64, ns(50), mW(800), W(4), pJ(640), nullptr);
    Cache l2 = Cache(KiB(256), 4, 64, ns(5), mW(800), W(2), pJ(5), &dram);
    Cache l1d = Cache(KiB(32), 1, 64, ps(500), mW(500), W(1), J(0), &l2);
    Cache l1i = Cache(KiB(32), 1, 64, ps(500), mW(500), W(1), J(0), &l2);
    

    // NOTE(Nate): I believe the easiest way to have :
    // 1. Have caches pass around pointers to the timer. They will update the
    //    timers based on their individual penalties.
    
    // int nums = 0; // NOTE(Nate): What is this used for?

    trace.next_instr();
    // This implementation has a single memory controller which is omnipotent 
    // and controls the flow. 
    // NOTE(Nate): This is an incorrect assumption. Each cache contains its own
    // controller. If data is not in this cache, it sends it up the chain.
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
                // Note(Nate): Can we attempt a refactor like so? We simply ask
                // the appropriate cache to perform the action, and it handles
                // the rest because it knows its parents. This would avoid the
                // need to pass around the memory heirarchy or the starting
                // index of the mem heirarchy.
                l1i.read((u64*) trace.instructions[trace.last_ins].value);

                // mem_read(0, )
                break;
            }

            case IGNORE: {
                printf("ignore!\n");
                break;
            }

            case FLUSH: { // NOTE(Nate): This is never used!
                printf("This is a flush! This case should never be tested!. \
                    something has gone horribly wrong!\n");
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
