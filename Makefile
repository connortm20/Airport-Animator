DEBUG = -g
OPTS = $(DEBUG) -Wall -ansi -pedantic -std=c++20

CC=g++ $(OPTS) -c
LN=g++

OBJS = main.o AirportAnimator.o

proj2: $(OBJS)
	$(LN) -o proj2 $(OBJS) -lncurses -lpthread

main.o: main.cpp AirportAnimator.hpp
	$(CC) main.cpp

AirportAnimator.o: AirportAnimator.cpp AirportAnimator.hpp
	$(CC) AirportAnimator.cpp

clean:
	/bin/rm -rf *~ $(OBJS) proj1
