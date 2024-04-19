#include "parser.hpp"
#include "memcontrol.hpp"
#include "cache.hpp"
#include "simulator.hpp"
#include <cstdio>

#include <vector>

void mem_read(int start_pos, std::vector<Cache> memory_hierarchy, u64* value, u64* addr, Joule* energy) {
    // We are going to loop until we get to the data in Cache or until we find it in DRAM
    for (int mem_index = start_pos; mem_index < DRAM; mem_index++) {
        Eviction status = memory_hierarchy[mem_index].read(addr); // Updates state of reg

        if (status == R_HIT) {
            // R1
            // *energy += (c.running_power - c.idle_power) * CYCLE_TIME;
            return;
        } 
        // energy += something because we missed at a level
    }
    
    // The read was a failure, enducing the R2 state.
    // Update all levels of the cache.
    memory_hierarchy[L2].write_miss(addr, value);
    memory_hierarchy[L1d].write_miss(addr, value);

    // todo: figure out the energy calculation here
    // energy += (c.running_power - c.idle_power) * CYCLE_TIME;
}

void mem_write(int start_idx, std::vector<Cache> memory_hierarchy, u64* value, u64* addr, Joule* energy) {
    for (int mem_index = start_idx; mem_index < DRAM; mem_index++) {
        Eviction x = memory_hierarchy[start_idx].write_hit(addr, value);
        // if (x == SUCCESS) { // NOTE(Nate): What's the intention here changing to fix compile error
        if (x != Eviction::R_HIT) {
            // update energies
            return;
        }
    }
    // This is now case W2, where we did not get a hit anywhere in the cache.
    // Bring data into the L2 cache from DRAM and trickle it down (like Reagan) to L1.
    memory_hierarchy[DRAM].write_dram(addr, value);
    memory_hierarchy[L2].write_miss(addr, value);
    memory_hierarchy[L1d].write_miss(addr, value);
}