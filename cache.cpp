#include "cache.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>

#ifndef NDEBUG
static u32 evict_index = 0;
#endif

Line::Line() : tag(0) {}

bool Line::is_valid() const {
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

u64 Line::get_tag() const {
    // printf("TAG: %lx\n", this->tag & ~(1UL << 63));
    return this->tag & ~(1UL << 63);
}

void Line::set_tag(u64 tag) {
    this->tag &= ~(1UL << 63);
    this->tag |= tag;
    return;
}

void Line::set(u64 tag, bool is_valid) {
    this->tag = ((u64)is_valid << 63) | (tag & ~(1UL << 63));
}

Cache::Cache(u64 capacity, u64 associativity, u64 block_size, Time latency,
    Watt idle_power, Watt running_power, Joule transfer_penalty,
    CacheFlags flags, Time& time, Cache* parent)
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
    , flags(flags)
    , time(time)
    , read_hits(0)
    , read_misses(0)
    , write_hits(0)
    , write_misses(0)
    , transfer_penalty(transfer_penalty)
    , latency(latency)
    , idle_power(idle_power)
    , running_power(running_power)
{
    if (!this->parent) {
        for (u64 i = 0; i < this->associativity * this->num_sets; i++) {
            this->lines[i].set_valid();
            this->lines[i].set_tag(i);
        }
    }
#ifndef NDEBUG
    printf("Cache b %lu a %lu s %lu t: %lu\n", block_bits, assoc_bits, 
            set_bits, tag_bits);
#endif
}

Cache::~Cache()
{
    delete[] this->lines;
}

const Line& Cache::read(const address addr)
{
    // u64 offset = (addr & ((1UL << this->block_bits) - 1UL));
    u64 set_index = (addr >> this->block_bits) & ((1 << (this->set_bits)) - 1UL);
    u64 tag = addr >> (this->set_bits + this->block_bits);

    // printf("offset: %lu index %lu tag: %lu\n", offset, index, tag);

    // 1. Iterate through each tag of the cache's set to see whether the cache 
    //    contains this value.
    // 2. If cache contains value: update time & energy and return.
    // 3. If cache does not contain value, read from parent cache.

    // Hit condition
    bool is_dram = !this->parent;
    for (u64 i = 0; i < this->associativity; i++) {
        Line& cur_line = this->lines[set_index*this->associativity + i];
        // printf("Address: %lx: tag:%lx\n", addr, tag);
        if (is_dram || (cur_line.is_valid() && cur_line.get_tag() == tag)) {
            this->read_hits++;
            // memcpy(returned_line, &lines[index + i], sizeof(Line));
            cur_line.set_tag(tag);
            return cur_line;
        }
    }

    // Miss condition
    this->read_misses++;
    const Line& read_line = this->parent->read(addr);
    const Line& replaced_line = this->put(read_line, set_index, tag);
    return replaced_line;
}   


const Line& Cache::write(const address addr, value val)
{
    // u64 offset = (addr & ((1UL << this->block_bits) - 1UL));
    u64 set_index = (addr >> this->block_bits) & ((1 << (this->set_bits)) - 1UL);
    u64 tag = addr >> (this->set_bits + this->block_bits);
    bool is_dram = !this->parent;

    for (size_t i = 0; i < this->associativity; i++) {
        Line& cur_line = lines[set_index*associativity + i];
        if (is_dram || (cur_line.is_valid() && cur_line.get_tag() == tag)) {
            // Write hit
            this->write_hits++;
            cur_line.set(tag, true);

            if (!is_dram) {
                // Writeback to parent, but no need to do anything to value
                // because we have a hit
                parent->write(addr, val); 
            }    
            return cur_line;
            // if (returned_line) {
            //     // If returned_line is null, that means the cache below has
            //     // hit. Therefore, there is no need to return a value, because
            //     // the cache below already has it! We are only calling write
            //     // on its parent because we would like to update the value
            //     *returned_line = cur_line;
            // }
            // return;
        }
    }
    
    this->write_misses++;
    const Line& written_line = this->parent->write(addr, val);
    const Line& replaced_line = this->put(written_line, set_index, tag);
    return replaced_line;
}

// Place a line into the cache at a particular set index. Should the tags
// not match AND there be no free lines in the cache, put will also cause
// the cache to have an eviction and call put on the parent. Returns a
// reference to the line in the cache which contains the new value.
const Line& Cache::put(const Line& line, u64 set_index, u64 tag)
{
    // Attempt to find invalid block to replace
    for (size_t i = 0; i < associativity; i++) {
        Line& cur_line = this->lines[set_index*this->associativity + i];
        if (!cur_line.is_valid()) {
            // Simply replace the lines.
            cur_line.set(tag, true);
            return cur_line;
        } 
    }

    // All lines valid. Evict a random line.
#ifdef NDEBUG 
    u64 victim_index = rand() % associativity;
#else 
    u64 victim_index = evict_index++;
    if (evict_index >= this->associativity) {
        evict_index = 0;
    }
#endif /* NDEBUG */

    Line& victim_line = lines[set_index*this->associativity + victim_index];
    victim_line.set(tag, true);

    return victim_line;
}

Time Cache::calc_delays() {
    Time delays = (this->read_hits + this->read_misses) * this->latency;
    // writes are asynchronous and therefore have no extra delays
    return delays;
}

Time Cache::calc_total_time(Cache& l1d, Cache& l1i, Cache& l2, Cache& dram) {
    // Time l1_read_delays = l1d.calc_delays() + l1i.calc_delays();
    // Time l2_read_delays = l2.calc_delays();
    // Time dram_read_delays = dram.calc_delays();
    // Time write_delays = l1i.latency;
    // // TODO(Nate): WRITE ME!
    return 0;
}

Time Cache::calc_active_time() {
    u64 total_accesses = this->read_hits + this->write_hits + this->write_misses + this->write_misses;
    return total_accesses * this->latency;
}

Time Cache::calc_idle_time() {
    return 0;
}