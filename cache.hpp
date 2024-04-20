#pragma once
#include "shortints.h"
#include <ratio>

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

// A single cache line. The smallest unit of the cache.
struct Line {
    // A packed data store of a cache line.
    // Assumes tag is always <= 62 bits
    //      [dirty | valid | tag]
    // bits: 63    | 62    | 61..0
    u64 tag;
    // u64 data[64];

    Line();
    bool is_valid() const;
    void set_valid();
    void set_invalid();
    u64 get_tag() const;
    void set_tag(u64 tag);
    void set(u64 tag, bool valid);
};

using CacheFlags = u8;
enum CacheFlagBits : CacheFlags {
    // Cache consistency in bit 0
    CONSISTENCY_WRITE_BACK = 0x0,
    CONSISTENCY_WRITE_THROUGH = 0x1,
    // Cache synchronization in bit 1
    SYNC_WRITE = 0x2,
    ASYNC_WRITE = 0x0,
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
    // Cache performance. Determined at runtime.
    Time& time;
    u64 read_hits, read_misses, write_hits, write_misses;
private:
    // Used for calculations at the end of the sim
    const Joule transfer_penalty;
    const Time latency;
    const Watt idle_power, running_power; 

public:

    Cache(u64 capacity, u64 associativity, u64 block_size, Time latency,
        Watt idle_power, Watt running_power, Joule transfer_penalty,
        CacheFlags flags, Time& time, Cache* parent = nullptr);
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
    const Line& put(const Line& line, u64 set_index, u64 tag);

public:
    static Time calc_total_time(Cache& l1d, Cache& l1i, Cache& l2, Cache& dram);
    Time calc_delays();
    Time calc_active_time();
    Time calc_idle_time();
};