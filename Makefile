### C++ Arguments
CC=g++
CF=-std=c++14 -Wall -O2

### Program Arguments
NAME=zynthetic

### Compiler Arguments
SRC_P=src

all: merge clean

merge: main.o 
	$(CC) $(CF) -o $(NAME) main.o

main.o: $(SRC_P)/main.cpp
	$(CC) $(CF) -c $(SRC_P)/main.cpp

clean:
	rm -rf *.o
