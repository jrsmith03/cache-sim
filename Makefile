SRC = parser.cpp cache.cpp simulator.cpp mem_control.cpp
EXEC = csim
CC = g++
# CFLAGS = -Wall -Werror -O3 
CFLAGS = -g -O0


csim: 
	${CC} ${CFLAGS} -o ${EXEC} ${SRC}

clean:
	rm ${EXEC}

test:
	rm ${EXEC}
	make
	gdb ./csim