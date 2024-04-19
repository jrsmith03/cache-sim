SRC = parser.cpp cache.cpp simulator.cpp
EXEC = csim
CC = g++
CFLAGS = -Wall -Werror
OPTFLAGS = -O3 -DNDEBUG

.PHONY: debug release clean test

debug: OPTFLAGS = -g3 -O0
debug: ${EXEC}

release: ${EXEC}

${EXEC}: ${SRC}
	${CC} ${CFLAGS} ${OPTFLAGS} -o ${EXEC} ${SRC} 

clean:
	rm -f ${EXEC}

test: debug
	gdb ./csim