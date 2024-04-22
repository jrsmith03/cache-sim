#pragma once
#include "shortints.h"
#include <functional>
#include <list>
#include <ratio>
#include <queue>
#include <string>

// Time has a resolution of 1 picosecond, and should be set via a macro.
using Time = u64; 
#define ps(num) (num)
#define ns(num) (ps(num)*1000UL)
#define us(num) (ns(num)*1000UL)
#define ms(num) (us(num)*1000UL)
#define s(num) (ms(num)*1000UL)

// Watts have a resolution of 1 picosecond, and should be set via a macro.
using Watt = u64;
#define pW(num) (num)
#define nW(num) (pW(num)*1000UL)
#define uW(num) (nW(num)*1000UL)
#define mW(num) (uW(num)*1000UL)
#define W(num) (mW(num)*1000UL)

// Joules have a resolution of 1 picosecond, and should be set via a macro.
using Joule = u64;
#define pJ(num) (num)
#define nJ(num) (pJ(num)*1000UL)
#define uJ(num) (nJ(num)*1000UL)
#define mJ(num) (uJ(num)*1000UL)
#define J(num) (mJ(num)*1000UL)

struct Machine;

// A single cache line. The smallest unit of the cache.
struct Line {
    // A packed data store of a cache line.
    // Assumes tag is always <= 61 bits
    //      [valid : dirty : in_flight : tag  ]
    // bits: 63    , 62    , 61        , 60..0
    using LineMetadata = u64;
    LineMetadata metadata;

    Line();
    bool is_valid() const;
    bool is_dirty() const;
    bool is_in_flight() const;
    u64 get_tag() const;

    void set_valid(bool is_valid);
    void set_dirty(bool is_dirty);
    void set_in_flight(bool is_in_flight);
    void set_tag(u64 tag);
    void set_metadata(u64 tag, bool is_valid, bool is_dirty, bool is_in_flight);
private:
    const u8 VALID_BIT = 63;
    const u8 DIRTY_BIT = 62;
    const u8 IN_FLIGHT_BIT = 61;
    const u8 MAX_TAG_SIZE = 61;
    void set_metadata_bit(u8 pos, bool value);
    bool get_metadata_bit(u8 pos) const;
};

using CacheFlags = u8;
enum CacheFlagBits : CacheFlags {
    // Cache consistency in bit 0
    WRITE_BACK = 0x0,
    WRITE_THROUGH = 0x1,
    // Cache synchronization in bit 1
    ASYNC_WRITE = 0x2,
    SYNC_WRITE = 0x0,
};

// A set within the cache. A set is a pointer to the first line of the set.
// Lines in a set exist contiguously in memory. 
struct Set {
    Line* lines;
};

// The Cache itself. DRAM can also be represented as a direct mapped cache.
struct Cache {
private:
    // Cache construction. A cache is simply a collection of lines. Determined
    // at creation.
    const u64 capacity, associativity, block_size, num_sets;
    const u64 block_bits, set_bits, assoc_bits, tag_bits; 
    Line* const lines;
    Cache* const parent;
    CacheFlags flags;
public:
    // Modified during runtime and used to evaluate cache performance.
    Machine& machine;
    Time active_time;
    u64 in_flight_count, dirty_evict_count;
    u64 read_hits, read_misses, write_hits, write_misses;
private:
    // Used for calculations at the end of the sim
    const Joule transfer_penalty;
    const Time latency;
    const Watt idle_power, running_power; 

public:

    Cache(u64 capacity, u64 associativity, u64 block_size, Time latency,
        Watt idle_power, Watt running_power, Joule transfer_penalty,
        CacheFlags flags, Machine& machine, Cache* parent = nullptr);
    ~Cache();
    
    // Note(Nate): Though these are addresses we are simulating, we gain no
    // benefit from passing them around as pointers. It may be more practical to
    // pass them around as `u64`s, or to define a datatype for an address. 
    using address = u64;
    using value = u64;

    const Line& read(address addr);
    const Line& write(address addr, value val);

    // Replace a line in the cache
private:
    bool is_write_through() const;
    bool is_write_back() const;
    bool is_async_write() const;
    bool is_sync_write() const;
    const Line& put(const Line& line, address addr, value val = 0);

public:
    Time calc_energy();
};

struct InFlightData {
    InFlightData(Cache* parent_cache, Line &dst_line, u64 dst_set_index, Time finish_time)
        : parent_cache(parent_cache)
        , dst_line(dst_line)
        , dst_set_index(dst_set_index)
        , finish_time(finish_time) {}
    Cache* parent_cache;
    Line &dst_line;
    u64 dst_set_index;
    Time finish_time;

    InFlightData(InFlightData&& rhs);
    InFlightData(const InFlightData& rhs);
    bool operator>(const InFlightData& rhs) const;
    Time operator-(const InFlightData& rhs) const;
    InFlightData& operator=(const InFlightData& other) noexcept;
};

// An InFlightQueue is an extension of a regular queue.
struct InFlightQueue : public std::priority_queue<InFlightData, std::vector<InFlightData>, std::greater<InFlightData>> {
    InFlightQueue(Time &machine_time);

    Time& machine_time;

    void push_line(Cache* parent_cache, u64 set_index, Line& dst_line, Time latency);
    void flush();

};

struct Machine {
    Time time;
    InFlightQueue in_flight_queue;
    std::vector<Cache*> caches;
    bool waited_this_access;

    Machine();
    void advanceTime(Time duration);
    void wait_for_line(Cache* cache, u64 tag, u64 set_index);
};

std::string unit_to_string(u64 num, char unit);