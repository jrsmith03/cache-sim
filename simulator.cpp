#include "parser.hpp"
#include "cache.hpp"
#include <cstdio>
#include "simulator.hpp"
#include <string>
#include <cstring>


int custom_assoc = 0;   
bool has_custom_assoc = false;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: csim -f <required, file name of trace> \n-a <associativity level, leave blank for default>\n");
        return -1;
    } else if (argc == 2 || argc == 4) {
        printf("Usage: csim -f <required, file name of trace> \n-a <associativity level; 2, 4, or 8; blank for default>\n");
        return -1;
    } else {
        if (argc == 5 && strncmp(argv[3], "-a", 3) == 0) {
            has_custom_assoc = true;
            if (strlen(argv[4]) > 1) {
                printf("error: please give an associativity from 1 to 8\n");
                return -1;
            }
            custom_assoc =  atoi(argv[4]);
        }
    } 
    
    int a_special =  has_custom_assoc ? custom_assoc : 4; 
    int a_regular = has_custom_assoc ? custom_assoc : 1; 
    
    Trace trace(argv[2]);

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
    CacheFlags l2_flags = CacheFlagBits::ASYNC_WRITE | CacheFlagBits::WRITE_BACK;
    CacheFlags l1_flags = CacheFlagBits::SYNC_WRITE | CacheFlagBits::WRITE_THROUGH;

    Cache dram = Cache(GiB(8), a_regular, 64, dram_time_penalty, mW(800), W(4), dram_transfer_penalty, dram_flags, machine, nullptr);
    Cache l2 = Cache(KiB(256), a_special, 64, l2_time_penalty, mW(800), W(2), l2_transfer_penalty, l2_flags, machine, &dram);
    Cache l1d = Cache(KiB(32), a_regular, 64, l1_time_penalty, mW(500), W(1), l1_transfer_penalty, l1_flags, machine, &l2);
    Cache l1i = Cache(KiB(32), a_regular, 64, l1_time_penalty, mW(500), W(1), l1_transfer_penalty, l1_flags, machine, &l2);

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

        // NOTE(Nate): Not sure from here
        // Choosing to assume that cycle penalty always applies on cache access
        // if (!machine.waited_this_access) {
        machine.advance_time(CYCLE_TIME);
        // }
        // machine.waited_this_access = false;
        // NOTE(Nate): To here. Because of writes.
        trace.next_instr();
    }
    machine.in_flight_queue.flush();

    // // We're done print contents
    Time total_time = machine.time;
    Joule total_energy = 0;
    for (Cache* cache : machine.caches) {
        total_energy += cache->calc_energy();
    }
    printf("Run complete!\nTime: %s\nEnergy: %s\n\n", 
        unit_to_string(total_time, 's', -12).c_str(),
        unit_to_string(total_energy, 'J', -15).c_str()
    );
    printf("File: %s\nL1 associativity: %d\nL2 associativity: %d\nDRAM associativity: %d\n\n", argv[2], a_regular, a_special, a_regular);
    printf("\
Cache    RHits   RMiss   WHits   WMiss Dirty_Evicts                  Time_Active                  Energy_Used\n\
L1d    %7lu %7lu %7lu %7lu %12lu %28s %28s\n\
L1i    %7lu %7lu %7lu %7lu %12lu %28s %28s\n\
L2     %7lu %7lu %7lu %7lu %12lu %28s %28s\n\
DRAM   %7lu %7lu %7lu %7lu %12lu %28s %28s\n",
        l1d.read_hits, l1d.read_misses, l1d.write_hits, l1d.write_misses, l1d.dirty_evict_count, unit_to_string(l1d.active_time, 's', -12).c_str(), unit_to_string(l1d.calc_energy(), 'J', -15).c_str(),
        l1i.read_hits, l1i.read_misses, l1i.write_hits, l1i.write_misses, l1i.dirty_evict_count, unit_to_string(l1i.active_time, 's', -12).c_str(), unit_to_string(l1i.calc_energy(), 'J', -15).c_str(),
        l2.read_hits, l2.read_misses, l2.write_hits, l2.write_misses, l2.dirty_evict_count, unit_to_string(l2.active_time, 's', -12).c_str(), unit_to_string(l2.calc_energy(), 'J', -15).c_str(),
        dram.read_hits, dram.read_misses, dram.write_hits, dram.write_misses, dram.dirty_evict_count, unit_to_string(dram.active_time, 's', -12).c_str(), unit_to_string(dram.calc_energy(), 'J', -15).c_str()
    );
    return 0;
}