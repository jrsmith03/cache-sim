SRC = parser.cpp cache.cpp simulator.cpp
EXEC = csim
CC = g++
CFLAGS = -Wall -Werror -O3

csim: 
	${CC} -o ${EXEC} ${SRC}

clean:
	rm ${EXEC}

test:
	rm ${EXEC}
	make
	./csim 008.espresso.din