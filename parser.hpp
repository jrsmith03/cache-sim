#include "shortints.h"

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
	Instruction next_instr();
	bool has_next_instr();
};