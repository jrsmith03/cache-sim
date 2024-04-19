#include "parser.hpp"
#include <cstdlib>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <stdlib.h>




// Trace constructor
Trace::Trace(char* filename) {
    this->has_next_instr = true; 
    // Open a file descriptor for the provided file
    
    this->trace_fd = open(filename, O_RDWR);   
    this->last_ins = 0;
}



void Trace::next_instr() {
    this->last_ins++;
    if (this->trace_fd != 0) {
        // todo: memory management
        char *read_buf = (char*)malloc(18);
        char *read_char = (char*)calloc(1, 1);
        char* end;
        int pos = 0;
        // Walk each line of input until it ends.
        while(read_char[0] != '\n') {
            size_t bytes_read = read(this->trace_fd, read_char, 1);
            // We have reached EOF
            if (read_char[0] == '\0' || bytes_read != 1) {
                this->has_next_instr = false;
                return;
            }
            read_buf[pos++] = read_char[0];
        }
        read_buf[pos] = '\0';
        // Now that we've read a line from the file, create the Instruction
        
        // this->instructions[this->last_ins].op = (Op) atoi(strtok(read_buf, " "));
        // this->instructions[this->last_ins].address = (u64) strtoull(strtok(NULL, " "), &end, 16); 
        // this->instructions[this->last_ins].value = (u64) strtoull(strtok(NULL, " "), &end, 16);

        this->instruction.op = (Op) atoi(strtok(read_buf, " "));
        this->instruction.address = (u64) strtoull(strtok(NULL, " "), &end, 16); 
        this->instruction.value = (u64) strtoull(strtok(NULL, " "), &end, 16);
        return;
    } else {
        printf("error: invalid filename");
        this->has_next_instr = false;
    }
}