#include "cache.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>

Line::Line() : valid(false) {}

bool Line::is_valid() {
    return tag >> 63;
}

void Line::set_valid() {
    this->tag |= 1UL << 63;
    return;
}

void Line::set_invalid() {
    this->tag &= ~(1UL << 63);
    return;
}

Cache::Cache(u64 capacity, u64 associativity, u64 block_size, Time latency,
    Watt idle_power, Watt running_power, Joule transfer_penalty,
    Cache* parent)
    : capacity(capacity)
    , associativity(associativity)
    , block_size(block_size)
    , num_sets(capacity / (associativity * block_size))
    , block_bits(static_cast<u64>(log2(static_cast<double>(block_size))))
    , set_bits(static_cast<u64>(log2(static_cast<double>(num_sets))))
    , assoc_bits(static_cast<u64>(log2(static_cast<double>(associativity))))
    , tag_bits(64 - block_bits - assoc_bits - set_bits)
    , lines(new Line[associativity * num_sets])
    , parent(parent)
    , read_hits(0)
    , read_misses(0)
    , write_hits(0)
    , write_misses(0)
    , transfer_penalty(transfer_penalty)
    , idle_power(idle_power)
    , running_power(running_power)
{
#ifndef NDEBUG
    printf("Cache b %lu a %lu s %lu t: %lu\n", block_bits, assoc_bits, 
            set_bits, tag_bits);
#endif
}

Cache::~Cache()
{
    delete[] this->lines;
}

void Cache::read(address addr, Line* returned_line)
{
    // u64 offset = (addr & ((1UL << this->block_bits) - 1UL));
    u64 index = (addr >> this->block_bits) & ((1 << (this->set_bits)) - 1UL);
    u64 tag = addr >> (this->set_bits + this->block_bits);

    // printf("offset: %lu index %lu tag: %lu\n", offset, index, tag);

    // 1. Iterate through each tag of the cache's set to see whether the cache 
    //    contains this value.
    // 2. If cache contains value: update time & energy and return.
    // 3. If cache does not contain value, read from parent cache.

    // Hit condition
    bool is_dram = !this->parent;
    for (u64 i = 0; i < associativity; i++) {
        if (is_dram || (this->lines[index + i].tag == tag)) {
            this->read_hits++;
            // memcpy(returned_line, &lines[index + i], sizeof(Line));
            *returned_line = this->lines[index + i];
            return;
        }
    }

    // Miss condition
    this->read_misses++;
    this->parent->read(addr, returned_line);
    this->put(returned_line, index);
    return;
}   


void Cache::write(address addr, value val, Line* returned_line)
{
    // u64 offset = (addr & ((1UL << this->block_bits) - 1UL));
    u64 index = (addr >> this->block_bits) & ((1 << (this->set_bits)) - 1UL);
    u64 tag = addr >> (this->set_bits + this->block_bits);
    bool is_dram = !this->parent;

    for (size_t i = 0; i < associativity; i++) {
        if (is_dram || lines[index + i].tag == tag) {
            // Write hit
            this->write_hits++;
            // memcpy(&val, &this->lines[index + i].data, this->block_size);
            this->lines[index + i].set_valid();

            if (!is_dram) {
                parent->write(addr, val, nullptr); 
            }    
            if (returned_line) {
                // If returned_line is null, that means the cache below has
                // hit. Therefore, there is no need to return a value, because
                // the cache below already has it! We are only calling write
                // on its parent because we would like to update the value
                *returned_line = this->lines[index + i];
            }
            return;
        }
    }
    
    this->write_misses++;
    this->parent->write(addr, val, returned_line);
    this->put(returned_line, index);
    return;
}

// Place a line into the cache at a particular set index. Should the tags
// not match AND there be no free lines in the cache, put will also cause
// the cache to have an eviction and call put on the parent.
void Cache::put(Line* line, u64 set_index)
{
    for (size_t i = 0; i < associativity; i++) {
        if (!lines[set_index + i].is_valid()) {
            // Simply replace the lines.
            lines[set_index + i] = *line;
            lines[set_index + i].set_valid();
            return;
        } 
    }

    // We must apply a Random eviction scheme.
#ifndef NDEBUG /* DEBUG */
    u64 victim_index = set_index;
#else 
    u64 victim_index = set_index + (rand() % associativity);
#endif /* NDEBUG */

    // Update the line.
    lines[victim_index] = *line;
    lines[victim_index].set_valid();
    // No need to update parent on read, since this is a write-through. Parent
    // will already be updated
    // this->parent->put(&lines[victim_index], set_index);
    return;
}

u64 Cache::calcTotalTime(Cache& l1d, Cache& l1i, Cache& l2, Cache& dram) {
    // TODO(Nate): WRITE ME!
    return 0;
}