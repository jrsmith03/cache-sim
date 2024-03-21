#include "cache.hpp"
#include <new>

Line::Line() : metadata(0) {}

inline void Line::set_line(u64 tag, bool dirty, bool valid) {
	this->metadata = ((u64)dirty << 63) | ((u64)valid << 62) | tag;
}

inline bool Line::tag_matches(u64 tag) {
	return this->tag() == tag;
}

Cache::Cache(u64 capacity, u64 associativity, u64 block_size, Time latency,
             Watt idle_power, Watt running_power, Joule transfer_penalty)
    : capacity(capacity), associativity(associativity), block_size(block_size),
      num_sets(capacity / (associativity * block_size)), latency(latency),
      idle_power(idle_power), running_power(running_power),
      transfer_penalty(transfer_penalty),
      lines(new Line[associativity * block_size]) {}

Cache::~Cache() {
	delete[] this->lines;
}