#include "cache.hpp"
#include "memcontrol.hpp"

#include <new>
#include <cmath>
#include <cstring>
#include <cstdio>


// Line::Line() : metadata(0) {}

// inline void Line::set_line(u64 tag, bool dirty, bool valid) {
// 	this->metadata = ((u64)dirty << 63) | ((u64)valid << 62) | tag;
// }

Line::Line() : dirty(false), valid(false), tag(0) {
  u64* data[64];
  for (int i = 0; i < 64; i++) {
    data[i] = nullptr;
  }
}


// bool Line::is_valid() {
// 	// return (this->metadata << 1) >> 63

// }

// bool Line::is_dirty() {
// 	return metadata >> 63
// }

// void Line::set_dirty() {
// 	return metadata >> 63
// }

// inline bool Line::tag_matches(u64 tag) {
// 	return tag() == tag;
// }

Cache::Cache(u64 capacity, u64 associativity, u64 block_size, Time latency,
             Watt idle_power, Watt running_power, Joule transfer_penalty)
    : capacity(capacity), associativity(associativity), block_size(block_size),
      num_sets(capacity / (associativity * block_size)), latency(latency),
      idle_power(idle_power), running_power(running_power),
      transfer_penalty(transfer_penalty),n(log2(block_size)), k(log2(associativity)), 
      m(block_size), tag_size(m - k - n),
      lines(new Line[associativity * block_size]) {
        // printf("Cache n %d k %d m %d", n, k, m);

      }

Cache::~Cache() {
	delete[] this->lines;
}

Cache_Info Cache::get_info(u64* addr) {
  Cache_Info info;
  info.offset = (*addr & ((1 << n) - 1));
  info.index = (*addr >> n) & ((1 << (m - k - n)) - 1);
  info.tag = *addr >> (k + n); 
  info.cache_index = info.index * associativity;
}

Eviction Cache::read(u64* addr) {
  // Cache_Info info = get_info(addr);
  // int offset = (*addr & (1 << n) - 1);
  // int index = (*addr >> n) & ((1 << (m - k - n)) - 1);
  // int tag = *addr >> (k + n); 
  int offset = log2(block_size);
  int index = log2(associativity * block_size);
  int tag = log2(log2(*addr) - index - offset);
  printf("offset: %d index %d tag: %d", offset, index, tag);


  // Index is the first line within the set. 
  // We must check all elements of the desired set to see if there is a hit.
  // u64 cache_index = index * associativity;
  bool miss_valid = false;
  bool invalid_clean = false;

  for (int i = 0; i < associativity; i++) {
    if(lines[index + i].tag == tag) {
      return R_HIT;
    } 
  }
  return R_MISS;
}

// Deal with only the W1 case, where we get a hit on the given level of cache.
// This is to preserve the abstraction.
Eviction Cache::write_hit(u64* addr, u64* value) {
  Cache_Info info = get_info(addr);

  for (int i = 0; i < associativity; i++) {
      if (lines[info.cache_index + i].tag == info.tag) {
        if (lines[info.cache_index + i].dirty) {

        } else {
          lines[info.cache_index + i].dirty = true;
          memcpy(lines[info.cache_index + i].data, value, 64);
          return W_HIT;
        }
      } 
  }
  // It is not in the given cache level, so the controller will have to try a write on the next level.
  return W_MISS;
}

// This function is invoked by the memory controller only when we know that
// an eviction has to occur (i.e. at the current cache set, there are no lines that match the tag)
void Cache::write_miss(u64* addr, u64* value) {
    Cache_Info info = get_info(addr);
    
    for (int i = 0; i < associativity; i++) {
      if(lines[info.cache_index + i].valid == true) {
        // If we encounter a valid line in the set, use that.
        if (lines[info.cache_index + i].dirty) {

        } else {
          lines[info.cache_index + i].dirty = true;
          memcpy(lines[info.cache_index + i].data, value, 64);
          return;
        }
      } 
    }
    // We must apply a Random eviction scheme.
    // TODO: FOR NOW, I'M JUST GOING TO EVICT THE FIRST LINE IN THE SET. CHANGE!!
    // u64 victim_index = cache_index + (rand() % associativity);
    u64 victim_index = info.cache_index;

    if (lines[victim_index].dirty) {

    } else {
      lines[victim_index].dirty = true;
      memcpy(lines[victim_index].data, value, 64);
    }

    // This is the first time the data has ever been written.
    lines[victim_index].tag = info.tag;
    lines[victim_index].valid = true;
    lines[victim_index].dirty = true; // Write-back cache
    return;
}

void Cache::write_dram(u64* addr, u64* value) {
    Cache_Info info = get_info(addr);
  
    // This is the first time the data has ever been written.
    memcpy(lines[info.cache_index].data, value, 64);
    lines[info.cache_index].tag = info.tag;
    lines[info.cache_index].valid = true;
    lines[info.cache_index].dirty = true; // Write-back cache
}