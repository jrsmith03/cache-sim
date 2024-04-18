#include <vector>
#include "cache.hpp"
enum mem_index {
    L1d,
    L1i,
    L2,
    DRAM
};

void mem_read(int start_pos, std::vector<Cache> memory_hierarchy, u64* value, u64* addr, Joule* energy);

void mem_write(int start_idx, std::vector<Cache> memory_hierarchy, u64* value, u64* addr, Joule* energy);
