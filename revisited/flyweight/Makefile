CC=g++
CFLAGS=-I. -ggdb -std=c++11 -Wall -Wextra -pedantic
LDFLAGS=
OBJ=main.o
OUT=flyweight.out

.PHONY: clean

%.o: %.cpp
	$(CC) -c $(CFLAGS) -o $@ $<
$(OUT): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf *.o *.out
