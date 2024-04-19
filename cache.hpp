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

enum Eviction : u8 {
    R_HIT,
    R_MISS,
    W_HIT,
    W_MISS,
};

// A single cache line. The smallest unit of the cache.
struct Line {
    // A packed data store of a cache line.
    // Assumes tag is always <= 62 bits
    //      [dirty | valid | tag]
    // bits: 63    | 62    | 61..0
    bool valid;
    u64 tag;
    u64 data[64];

    Line();
    void set_line(u64 tag, bool valid);
};

// A set within the cache. A set is a pointer to the first line of the set.
// Lines in a set exist contiguously in memory. 
struct Set {
    Line* lines;
};

// The Cache itself. DRAM can also be represented as a direct mapped cache.
struct Cache {
private:
    // Cache construction. A cache is simply a collection of lines.
    u64 capacity, associativity, block_size, num_sets;
    u64 block_bits, set_bits, assoc_bits, tag_bits; // NOTE(Nate): What are n, k, and m? Do they refer
    Line* lines;
    Cache* parent;
    // Cache performance
    u64 hit, miss;
    // Used for calculations at the end of the sim
    Joule transfer_penalty;
    Watt idle_power, running_power; 

public:

    Cache(u64 capacity, u64 associativity, u64 block_size, Time latency,
        Watt idle_power, Watt running_power, Joule transfer_penalty,
        Cache* parent = nullptr);
    ~Cache();
    
    // Note(Nate): Though these are addresses we are simulating, we gain no
    // benefit from passing them around as pointers. It may be more practical to
    // pass them around as `u64`s, or to define a datatype for an address. 
    typedef u64 address;
    typedef u64 value;

    void read(address addr, Line* returned_line);
    void write(address addr, value val, Line* line);

    // Replace a line in the cache
    void put(Line* line, u64 set_index);



    static u64 calcTotalTime(Cache& l1d, Cache& l1i, Cache& l2, Cache& dram);
};