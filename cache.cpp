#include "cache.hpp"
#include <new>
#include <cmath>

Line::Line() : metadata(0) {}

inline void Line::set_line(u64 tag, bool dirty, bool valid) {
	this->metadata = ((u64)dirty << 63) | ((u64)valid << 62) | tag;
}

bool Line::is_valid() {
	return (this->metadata << 1) >> 63
}

bool Line::is_dirty() {
	return this.metadata >> 63
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

Eviction Cache::read(void* addr) {

  uint64_t n = log2(this.block_size);
  uint64_t k = log2(this.associativity);
  uint64_t m = 64;
  uint64_t tag_size = m - k - n;

  uint64_t offset = addr & (1 << n) - 1;
  uint64_t index = (addr >> n) & ((1 << (m - k - n)) - 1);
  uint64_t tag = addr >> (k + n); 

  // Index is the first line within the set. 
  // We must check all elements of the desired set to see if there is a hit.
  uint64_t cache_index = index * this.associativity;
  bool miss_valid = false;
  bool invalid_clean = false;

  for (int i = 0; i < this.associativity; i++) {
    if(this.lines[cache_index + i].tag_matches(tag)) {
      return HIT;
    } else if (this.lines[cache_index + i].is_valid()) {
        miss_valid = true;
    } else if (!this.lines[cache_index + i].is_dirty()) {
        invalid_clean = true;
    }
  }

  if (miss_valid) {
    // there is a valid line in the set, tell the controller to fetch block size from memory and place it there
    return MISS_VALID;
  } else if (invalid_clean) {
    // there is no valid line, tell controller to run the eviction algorithm
    // however, there is a clean line
    return INVALID_CLEAN;
  } else {
    return INVALID_DIRTY;
  }  
}