#include "parser.hpp"
#include "mem_control.cpp"

enum mem_index {
    L1d,
    L1i,
    L2,
    DRAM
}
void mem_read(std::vector memory_hierarchy, uint64_t value, uint64_t addr) {
    // We are going to loop until we get to the data in Cache or until we find it in DRAM
    for (int mem_index = 0; mem_index < memory_hierarchy.size(); mem_index++) {
        Cache c = memory_hierarchy[mem_index];
        Eviction status == c.read(addr); // Updates state of reg
        switch (status) {
            // R1
            case HIT: {
                // Hit. Simple case
                energy += (c.running_power - c.idle_power) * CYCLE_TIME;
                break;
            }
            // R2a - Fetch memory block from DRAM and place it in the available line set.
            case MISS_VALID: {
                // Means that we need to see if the data is in the next memory level
                // If so, write 

                // We can assume it's always present in DRAM
                if (mem_index == DRAM) {
                    c.
                }
                Eviction next_level == memory_hierarchy[mem_index+1].read(address); 

            }

            // R2b
            case INVALID_DIRTY: {
                // TODO: I stopped here. Not thrilled with how this while loop is
                // turning out. May change Cache API to only have `is_hit`, `put`,
                // etc. 
            }
            case INVALID_CLEAN: {

            }
        }
    }
}