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
	HIT,
	DIRTY,
	CLEAN
};

// A single cache line. The smallest unit of the cache.
struct Line {
	// A packed data store of a cache line.
	// Assumes tag is always <= 62 bits
	//      [dirty | valid | tag]
	// bits: 63    | 62    | 61..0
	u64 metadata; 

	Line();
	u64 tag();
	bool is_dirty();
	bool is_valid();
	bool tag_matches(u64 tag);
	void set_line(u64 tag, bool dirty, bool valid);
};

// A set within the cache. A set is a pointer to the first line of the set.
// Lines in a set exist contiguously in memory. 
struct Set {
	Line* lines;
};

// The Cache itself. DRAM can also be represented as a direct mapped cache.
struct Cache {
	u64 capacity, associativity, block_size, num_sets;
	Time latency;
	Watt idle_power, running_power;
	Joule transfer_penalty;
	Line* lines; // The cache is simply a collection of lines.

	Cache(u64 capacity, u64 associativity, u64 block_size, Time latency,
		Watt idle_power, Watt running_power, Joule transfer_penalty);
	~Cache();
	Eviction read(void* addr);
	Eviction write(void* addr);

	Set get_set(u64 set_index);
};