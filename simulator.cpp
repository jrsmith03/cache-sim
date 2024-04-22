#include "parser.hpp"
#include "cache.hpp"
#include <cstdio>

#include "simulator.hpp"
#include <string>
#include <cstring>


std::string trace_name;
int custom_assoc = 0;
bool has_custom_assoc = false;
int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: csim -f <required, file name of trace> \n-a <associativity level, leave blank for default>\n");
        return -1;
    } else if (argc == 2 || argc == 4) {
        printf("Usage: csim -f <required, file name of trace> \n-a <associativity level, leave blank for default>\n");
        return -1;
    } else {
        if (argc == 3 || strncmp(argv[1], "-f", 3) == 0) {
            trace_name = argv[1];
        }

        if (argc == 5 && strncmp(argv[3], "-a", 3) == 0) {
            has_custom_assoc = true;
            custom_assoc =  atoi(argv[4]);
        }
    } 
    }
    int al2 =  has_custom_assoc ? custom_assoc : 4; 
    int al1i, al1d, adram = has_custom_assoc ? custom_assoc : 1; 

    // Maybe take input and output here.
    // Would be good to initialize caches from parser.
    // Memory is not byte addressable in this design. It is 64 byte addressable.
    Trace trace(trace_name);
    if (trace.trace_fd == -1) {
        printf("error: invalid filename\n");
        return -1;
    }

    Machine machine = Machine();

    Time l1_time_penalty = ps(500); 
    Time l2_time_penalty = ns(5) - l1_time_penalty; 
    Time dram_time_penalty = ns(50) - l2_time_penalty; 

    Joule l1_transfer_penalty = J(0);
    Joule l2_transfer_penalty = pJ(5) - l1_transfer_penalty;
    Joule dram_transfer_penalty = pJ(640) - l2_transfer_penalty;

    CacheFlags dram_flags = 0;
    CacheFlags l2_flags = CacheFlagBits::SYNC_WRITE | CacheFlagBits::WRITE_BACK;
    CacheFlags l1_flags = CacheFlagBits::ASYNC_WRITE | CacheFlagBits::WRITE_THROUGH;

    Cache dram = Cache(GiB(8), adram, 64, dram_time_penalty, mW(800), W(4), dram_transfer_penalty, dram_flags, machine, nullptr);
    Cache l2 = Cache(KiB(256), al2, 64, l2_time_penalty, mW(800), W(2), l2_transfer_penalty, l2_flags, machine, &dram);
    Cache l1d = Cache(KiB(32), al1d, 64, l1_time_penalty, mW(500), W(1), l1_transfer_penalty, l1_flags, machine, &l2);
    Cache l1i = Cache(KiB(32), al1i, 64, l1_time_penalty, mW(500), W(1), l1_transfer_penalty, l1_flags, machine, &l2);

    machine.caches.push_back(&dram);
    machine.caches.push_back(&l2);
    machine.caches.push_back(&l1d);
    machine.caches.push_back(&l1i);
    
    trace.next_instr();
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
                l1i.read(trace.instruction.address);
                break;
            }

            case IGNORE: {
                #ifndef NDEBUG
                printf("ignore!\n");
                #endif
                break;
            }

            case FLUSH: { 
                printf("This is a flush! This case should never be tested!. \
                    something has gone horribly wrong!\n");
                break;
            }

        }

        //NOTE(Nate): Not sure from here
        if (!machine.waited_this_access) {
            machine.advanceTime(CYCLE_TIME);
        }
        machine.waited_this_access = false;
        //NOTE(Nate): To here. Because of writes.
        trace.next_instr();
    }
    machine.in_flight_queue.flush();

    // // We're done print contents
    Time total_time = machine.time;
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