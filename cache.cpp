#include "cache.hpp"
#include <new>
#include <cmath>
#include "memcontrol.hpp"

// Line::Line() : metadata(0) {}

// inline void Line::set_line(u64 tag, bool dirty, bool valid) {
// 	this->metadata = ((u64)dirty << 63) | ((u64)valid << 62) | tag;
// }

Line::Line() : dirty(false), valid(false), tag(0) {
  void* data[64];
  for (int i = 0; i < 64; i++) {
    data[i] = nullptr;
  }
}


// bool Line::is_valid() {
// 	// return (this->metadata << 1) >> 63

// }

// bool Line::is_dirty() {
// 	return this.metadata >> 63
// }

// void Line::set_dirty() {
// 	return this.metadata >> 63
// }

inline bool Line::tag_matches(u64 tag) {
	return this->tag() == tag;
}

Cache::Cache(u64 capacity, u64 associativity, u64 block_size, Time latency,
             Watt idle_power, Watt running_power, Joule transfer_penalty)
    : capacity(capacity), associativity(associativity), block_size(block_size),
      num_sets(capacity / (associativity * block_size)), latency(latency),
      idle_power(idle_power), running_power(running_power),
      transfer_penalty(transfer_penalty),
      lines(new Line[associativity * block_size]) {
          n = log2(this.block_size);
          k = log2(this.associativity);
          m = 64;
          tag_size = m - k - n;

      }

Cache::~Cache() {
	delete[] this->lines;
}

Eviction Cache::read(void* addr) {
  uint64_t offset = addr & (1 << this.n) - 1;
  uint64_t index = (addr >> this.n) & ((1 << (this.m - this.k - this.n)) - 1);
  uint64_t tag = addr >> (this.k + this.n); 

  // Index is the first line within the set. 
  // We must check all elements of the desired set to see if there is a hit.
  uint64_t cache_index = index * this.associativity;
  bool miss_valid = false;
  bool invalid_clean = false;

  for (int i = 0; i < this.associativity; i++) {
    if(this.lines[cache_index + i].tag == tag) {
      return HIT;
    } 
  }
  return MISS;
}

// Deal with only the W1 case, where we get a hit on the given level of cache.
// This is to preserve the abstraction.
Status Cache::write_hit(void* addr, void* value) {
  uint64_t offset = addr & (1 << this.n) - 1;
  uint64_t index = (addr >> this.n) & ((1 << (this.m - this.k - this.n)) - 1);
  uint64_t tag = addr >> (this.k + this.n); 
  uint64_t cache_index = index * this.associativity;


  for (int i = 0; i < this.associativity; i++) {
      if(this.lines[cache_index + i].tag == tag) {
        if (this.lines[cache_index + i].dirty) {

        } else {
          this.lines[cache_index + i].dirty = true;
          memcpy(this.lines[cache_index + i].data, value, 64);
          return WRITE_HIT;
        }
      } 
  }
  // It is not in the given cache level, so the controller will have to try a write on the next level.
  return WRITE_MISS;
}



// This function is invoked by the memory controller only when we know that
// an eviction has to occur (i.e. at the current cache set, there are no lines that match the tag)
void Cache::write_miss(void* addr, void* value) {
    uint64_t offset = addr & (1 << this.n) - 1;
    uint64_t index = (addr >> this.n) & ((1 << (this.m - this.k - this.n)) - 1);
    uint64_t tag = addr >> (this.k + this.n);

    uint64_t cache_index = index * this.associativity;

    for (int i = 0; i < this.associativity; i++) {
      if(this.lines[cache_index + i].valid == valid) {
        // If we encounter a valid line in the set, use that.
        if (this.lines[cache_index + i].dirty) {

        } else {
          this.lines[cache_index + i].dirty = true;
          memcpy(this.lines[cache_index + i].data, value, 64);
          return;
        }
      } 
    }
    // We must apply a Random eviction scheme.
    // TODO: FOR NOW, I'M JUST GOING TO EVICT THE FIRST LINE IN THE SET. CHANGE!!
    // uint64_t victim_index = cache_index + (rand() % this.associativity);
    uint64_t victim_index = cache_index;

    if (this.lines[victim_index].dirty) {

    } else {
      this.lines[victim_index].dirty = true;
      memcpy(this.lines[victim_index].data, value, 64);
    }

    // This is the first time the data has ever been written.
    this.lines[victim_index].tag = tag;
    this.lines[victim_index].valid = true;
    this.lines[victim_index].dirty = true; // Write-back cache
    return;
}

void Cache::write_dram(void* addr, void* value) {
    uint64_t offset = addr & (1 << this.n) - 1;
    uint64_t index = (addr >> this.n) & ((1 << (this.m - this.k - this.n)) - 1);
    uint64_t tag = addr >> (this.k + this.n);
    uint64_t cache_index = index * this.associativity;
  
    // This is the first time the data has ever been written.
    memcpy(this.lines[cache_index].data, value, 64);
    this.lines[cache_index].tag = tag;
    this.lines[cache_index].valid = true;
    this.lines[cache_index].dirty = true; // Write-back cache
}