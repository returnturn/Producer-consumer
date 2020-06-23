CC = g++
CFLAGS = -Wall -std=c++11 -pthread
SOURCES = $(wildcard *.cpp)
OBJECTS = prodcon.o tands.o

all: clean compile prodcon

clean:
	rm -f *.o prodcon

compile:
%.o: %.cpp
	${CC} ${CFLAGS} -c $^ -o $@

prodcon: $(OBJECTS)
	$(CC) -o prodcon $(OBJECTS) -pthread

