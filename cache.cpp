#include "cache.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>
#include <queue>
#include <utility>

#ifndef NDEBUG
static u32 evict_index = 0;
#endif

Line::Line() : metadata(0) {}

void Line::set_metadata_bit(u8 pos, bool value) {
    u64 mask = ~(1UL << pos);
    this->metadata &= mask; 
    this->metadata |= (u64) value << pos;
    return;
}

bool Line::get_metadata_bit(u8 pos) const {
    u64 temp = this->metadata >> pos;
    return temp & ~(1UL);
}

bool Line::is_valid() const {
    return this->get_metadata_bit(VALID_BIT);
}

bool Line::is_dirty() const {
    return this->get_metadata_bit(DIRTY_BIT);
}

bool Line::is_in_flight() const {
    return this->get_metadata_bit(IN_FLIGHT_BIT);
}

void Line::set_valid(bool is_valid) {
    return this->set_metadata_bit(VALID_BIT, is_valid);
}

void Line::set_dirty(bool is_dirty) {
    return this->set_metadata_bit(VALID_BIT, is_dirty);
}

void Line::set_in_flight(bool is_in_flight) {
    return this->set_metadata_bit(VALID_BIT, is_in_flight);
}

u64 Line::get_tag() const {
    return this->metadata & ((1UL << (MAX_TAG_SIZE-1)) - 1);
}

void Line::set_tag(u64 tag) {
    this->metadata &= ~((1UL << (MAX_TAG_SIZE-1)) - 1);
    this->metadata |= tag;
    return;
}

void Line::set_metadata(u64 tag, bool is_valid, bool is_dirty, bool is_in_flight) {
    this->metadata = 
        ((u64)is_valid << VALID_BIT) | 
        ((u64)is_dirty << DIRTY_BIT) | 
        ((u64)is_in_flight << IN_FLIGHT_BIT) | 
        (tag & ~(1UL << (MAX_TAG_SIZE - 1)));
    return;
}

Cache::Cache(u64 capacity, u64 associativity, u64 block_size, Time latency,
    Watt idle_power, Watt running_power, Joule transfer_penalty,
    CacheFlags flags, Machine& machine, Cache* parent)
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
    , machine(machine)
    , in_flight_count(0)
    , dirty_evict_count(0)
    , read_hits(0)
    , read_misses(0)
    , write_hits(0)
    , write_misses(0)
    , transfer_penalty(transfer_penalty)
    , latency(latency)
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

bool Cache::is_write_back() const {
    return !(this->flags & WRITE_THROUGH);
}
bool Cache::is_write_through() const {
    return this->flags & WRITE_THROUGH;
}
bool Cache::is_sync_write() const {
    return !(this->flags & ASYNC_WRITE);
}
bool Cache::is_async_write() const {
    return this->flags & ASYNC_WRITE;
}

const Line& Cache::read(const address addr)
{
    const u64 set_index = (addr >> this->block_bits) & ((1UL << (this->set_bits)) - 1);
    const u64 tag = addr >> (this->set_bits + this->block_bits);
    const bool is_dram = !this->parent;

    // Hit condition
    for (u64 i = 0; i < this->associativity; i++) {
        Line& cur_line = this->lines[set_index*this->associativity + i];
        const bool is_hit = is_dram || (cur_line.is_valid() && cur_line.get_tag() == tag);
        if (is_hit) {
            this->read_hits++;
            if (cur_line.is_in_flight()) {
                this->machine.wait_for_line(this, cur_line.get_tag(), set_index);
            } 
            if (this->in_flight_count == 0) {
                this->active_time += latency;
            } else {
                this->machine.advanceTime(latency);
            }
            return cur_line;
        }
    }

    // Miss condition
    this->read_misses++;
    const Line& read_line = this->parent->read(addr);
    const Line& replaced_line = this->put(read_line, addr);
    return replaced_line;
}


const Line& Cache::write(const address addr, value val)
{
    const u64 set_index = (addr >> this->block_bits) & ((1UL << (this->set_bits)) - 1);
    const u64 tag = addr >> (this->set_bits + this->block_bits);

    // Base case
    const bool is_dram = !this->parent;
    if (is_dram) {
        write_hits++;
        // Ensure that a DRAM line is always clean, valid, and not in flight.
        lines[set_index].set_metadata(0, true, false, false);
        return lines[set_index];
    }

    // Tag matching to see if thre is a hit
    for (size_t i = 0; i < this->associativity; i++) {
        Line& cur_line = lines[set_index*associativity + i];
        if (cur_line.is_valid() && cur_line.get_tag() == tag) {
            // Write hit
            this->write_hits++;
            if (this->is_write_back()) {
                cur_line.set_dirty(true);
            } else if (this->is_write_through()) {
                if (this->is_async_write()) { // Is this even possible?
                    this->machine.in_flight_queue.push_line(this, set_index, cur_line, this->latency);
                }
                parent->write(addr, val); 
            }
            return cur_line;
        }
    }
    
    this->write_misses++;
    this->read_misses--; // Remove a read miss to balance out the miss we are about to have
    this->read(addr); // Retrieve the correct line. This handles eviction and such.
    for (size_t i = 0; i < this->associativity; i++) {
        Line& cur_line = lines[set_index*associativity + i];
        if (cur_line.is_valid() && cur_line.get_tag() == tag) {
            // Write hit
            if (this->is_write_back()) {
                cur_line.set_dirty(true);
            } else if (this->is_write_through()) {
                if (this->is_async_write()) { // Is this even possible?
                    this->machine.in_flight_queue.push_line(this, set_index, cur_line, this->latency);
                }
                parent->write(addr, val); 
            }
            return cur_line;
        }
    }

    /* UNREACHABLE */
    return lines[0];
}

// Place a line into the cache at a particular set index. Should the tags
// not match AND there be no free lines in the cache, put will also cause
// the cache to have an eviction and call put on the parent. Returns a
// reference to the line in the cache which contains the new value.
const Line& Cache::put(const Line& line, address addr, value val)
{
    const u64 set_index = (addr >> this->block_bits) & ((1UL << (this->set_bits)) - 1);
    const u64 tag = addr >> (this->set_bits + this->block_bits);

    // Attempt to find invalid block to replace
    Line* victim_line = nullptr;
    for (size_t i = 0; i < associativity; i++) {
        Line& cur_line = this->lines[set_index*this->associativity + i];
        if (!cur_line.is_valid()) {
            // If line is not valid, it can be selected for replacement
            cur_line.set_metadata(tag, true, false, false);
            victim_line = &cur_line;
            break;
        } 
    }

    bool all_lines_valid = victim_line == nullptr;
    if (all_lines_valid) {
        #ifdef NDEBUG 
        u64 victim_index = rand() % associativity;
        #else 
        u64 victim_index = evict_index++;
        if (evict_index >= this->associativity) {
            evict_index = 0;
        }
        #endif /* NDEBUG */
        victim_line = &lines[set_index*this->associativity + victim_index];
    }

    if (victim_line->is_in_flight()) {
        this->machine.wait_for_line(this, tag, set_index);
    }
    if (this->is_write_back() && victim_line->is_dirty()) {
        this->parent->write(addr, val); // values in writes don't matter
    }

    victim_line->set_metadata(tag, true, false, false);

    return *victim_line;
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

InFlightQueue::InFlightQueue(Time& machine_time) 
    : std::priority_queue<InFlightData, std::vector<InFlightData>, std::greater<InFlightData>>()
    , machine_time(machine_time)
{}

void InFlightQueue::push_line(Cache* parent_cache, u64 set_index, Line& dst_line, Time latency) {
    InFlightData data = InFlightData{
        parent_cache,
        dst_line,
        set_index,
        this->machine_time + latency
    };
    this->push(data);
}

void InFlightQueue::flush() {
    while (this->size() > 0) {
        const InFlightData& top = this->top();
        if (this->machine_time < top.finish_time) {
            this->machine_time = top.finish_time;
        }
        top.dst_line.set_in_flight(false);

        this->pop();
    }
}

bool InFlightData::operator>(const InFlightData& rhs) const {
    return this->finish_time > rhs.finish_time;
}

InFlightData& InFlightData::operator=(const InFlightData& other) noexcept {
    if (this != &other) {
        // Copy member variables from 'other' to 'this'
        // Be sure to handle any dynamic memory allocation appropriately
    }
    return *this;
}

// Move constructor
InFlightData::InFlightData(InFlightData&& rhs)
    : parent_cache(rhs.parent_cache)
    , dst_line(rhs.dst_line)
    , dst_set_index(rhs.dst_set_index)
    , finish_time(rhs.finish_time) {}

// Copy constructor
InFlightData::InFlightData(const InFlightData& rhs)
    : parent_cache(rhs.parent_cache)
    , dst_line(rhs.dst_line)
    , dst_set_index(rhs.dst_set_index)
    , finish_time(rhs.finish_time) {}

// Time InFlightData::operator-(const InFlightData& rhs) const {
//     return this->finish_time - rhs.finish_time;
// }

Machine::Machine() : time(0), in_flight_queue(time), caches(), waited_this_access(false) {}

// Advance the time of the machine, while updating the active times of any
// caches which are currently waiting on a writeback
void Machine::advanceTime(const Time duration) {
    const Time advanced_time = this->time + duration;
    while (!this->in_flight_queue.empty() && this->in_flight_queue.top().finish_time <= advanced_time) {
        const InFlightData& next = this->in_flight_queue.top();
        // Advance active time for all active caches
        for (Cache* cache : this->caches) {
            if (cache->in_flight_count > 0) {
                cache->active_time += next.finish_time - this->time;
            }
        }
        this->time += next.finish_time - this->time;
        // Update metadata for cache line
        next.dst_line.set_in_flight(false);
        next.parent_cache->in_flight_count -= 1;

        this->in_flight_queue.pop();
    }

    return;
}

// Advances time until a line is available. A line can be uniquely identified by
// its tag, set_index, and parent cache
void Machine::wait_for_line(Cache* cache, u64 tag, u64 set_index) {
    this->waited_this_access = true;
    u64 last_tag;
    u64 last_set_index;
    Cache* last_cache;

    do {
        const InFlightData& top = this->in_flight_queue.top();
        last_tag = top.dst_line.get_tag();
        last_set_index = top.dst_set_index;
        last_cache = top.parent_cache;

        this->advanceTime(top.finish_time - this->time);
    } while (tag != last_tag || set_index != last_set_index || cache != last_cache);
}
