#include "shortints.h"
#include <vector>
// TODO: Define useful function definitions

enum Op : u8 {
	READ = 0,
	WRITE = 1,
	FETCH = 2,
	IGNORE = 3,
	FLUSH = 4,
};

struct Instruction {
	Op op;
	u64 address;
	u64 value;

};

struct Trace {
	Trace(char* filename); 
	int trace_fd;
	// A trace will have many instructions. I don't know if it would be beneficial to 
	// store all of them or just store the last one that was read. 
	// For now, I'm going to store all of them but this can easily be changed.
	
	u64 last_ins; // Index to the last read instruction 
	// std::vector<Instruction> instructions[1024];
	// Instruction *instructions = new Instruction[2];
	Instruction instruction;
	void next_instr(); // method to add to the instruction array
	bool has_next_instr;
};