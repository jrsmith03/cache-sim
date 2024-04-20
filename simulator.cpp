#include "parser.hpp"
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
    Trace trace(argv[1]);
    if (trace.trace_fd == -1) {
        printf("error: invalid filename\n");
        return -1;
    }

    Time time = 0;

    Time l1_time_penalty = ps(500); 
    Time l2_time_penalty = ns(5) - l1_time_penalty; 
    Time dram_time_penalty = ns(50) - l2_time_penalty; 

    Joule l1_transfer_penalty = J(0);
    Joule l2_transfer_penalty = pJ(5) - l1_transfer_penalty;
    Joule dram_transfer_penalty = pJ(640) - l2_transfer_penalty;

    CacheFlags dram_flags = 0;
    CacheFlags l2_flags = CacheFlagBits::ASYNC_WRITE | CacheFlagBits::CONSISTENCY_WRITE_BACK;
    CacheFlags l1_flags = CacheFlagBits::SYNC_WRITE | CacheFlagBits::CONSISTENCY_WRITE_THROUGH;

    Cache dram = Cache(GiB(8), 1, 64, dram_time_penalty, mW(800), W(4), dram_transfer_penalty, dram_flags, time, nullptr);
    Cache l2 = Cache(KiB(256), 4, 64, l2_time_penalty, mW(800), W(2), l2_transfer_penalty, l2_flags, time, &dram);
    Cache l1d = Cache(KiB(32), 1, 64, l1_time_penalty, mW(500), W(1), l1_transfer_penalty, l1_flags, time, &l2);
    Cache l1i = Cache(KiB(32), 1, 64, l1_time_penalty, mW(500), W(1), l1_transfer_penalty, l1_flags, time, &l2);
    

    // NOTE(Nate): I believe the easiest way to have :
    // 1. Have caches pass around pointers to the timer. They will update the
    //    timers based on their individual penalties.
    
    trace.next_instr();
    // This implementation has a single memory controller which is omnipotent 
    // and controls the flow. 
    // NOTE(Nate): This is an incorrect assumption. Each cache contains its own
    // controller. If data is not in this cache, it sends it up the chain.
    while (trace.has_next_instr) {

        // Switch based on operation from parser.
        // Call into the Memory Controller to handle everything.
        switch (trace.instruction.op) {
            case READ: {
                #ifndef NDEBUG
                printf("read!\n");
                #endif
                
                l1d.read(trace.instruction.address);
                break;
            }

            case WRITE: {
                #ifndef NDEBUG
                printf("write!\n");
                #endif
                l1d.write(trace.instruction.address, trace.instruction.value);
                break;
            }

            case FETCH: {
                #ifndef NDEBUG
                printf("fetch!\n");
                #endif
                // Note(Nate): Can we attempt a refactor like so? We simply ask
                // the appropriate cache to perform the action, and it handles
                // the rest because it knows its parents. This would avoid the
                // need to pass around the memory heirarchy or the starting
                // index of the mem heirarchy.
                l1i.read(trace.instruction.address);
                break;
            }

            case IGNORE: {
                #ifndef NDEBUG
                printf("ignore!\n");
                #endif
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
    Time total_time = Cache::calc_total_time(l1d, l1i, l2, dram); 
    printf("Run complete! Total time in ns: %lu\n", total_time);
    printf("\
Cache    RHits   RMiss   WHits   WMiss\n\
L1d    %7lu %7lu %7lu %7lu\n\
L1i    %7lu %7lu %7lu %7lu\n\
L2     %7lu %7lu %7lu %7lu\n\
DRAM   %7lu %7lu %7lu %7lu\n",
        l1d.read_hits, l1d.read_misses, l1d.write_hits, l1d.write_misses,
        l1i.read_hits, l1i.read_misses, l1i.write_hits, l1i.write_misses,
        l2.read_hits, l2.read_misses, l2.write_hits, l2.write_misses,
        dram.read_hits, dram.read_misses, dram.write_hits, dram.write_misses
    );
    return 0;
}
