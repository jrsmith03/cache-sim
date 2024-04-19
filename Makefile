SRC = parser.cpp cache.cpp simulator.cpp mem_control.cpp
EXEC = csim
CC = g++
CFLAGS = -Wall -Werror
OPTFLAGS = -O3

.PHONY: debug release clean test

debug: OPTFLAGS = -g3 -Og
debug: ${EXEC}

release: ${EXEC}

${EXEC}: ${SRC}
	${CC} ${CFLAGS} ${OPTFLAGS} -o ${EXEC} ${SRC} 

clean:
	rm -f ${EXEC}

test: debug
	gdb ./csim