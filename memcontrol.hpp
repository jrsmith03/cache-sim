enum mem_index {
    L1d,
    L1i,
    L2,
    DRAM
}

void mem_read(int start_pos, std::vector memory_hierarchy, uint64_t value, uint64_t addr);
void mem_write(int start_idx, std::vector memory_hierarchy, void* value, void* addr, Joule* energy);