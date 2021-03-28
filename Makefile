.PHONY: run clean

CPP = g++
CPPFLAGS = -Wall
objects = consts.o utils.o proc.o

all: clean hw1

%.o: %.c
	$(CPP) ─c $<

hw1: hw1.cpp $(objects)
	$(CPP) $(CPPFLAGS) $(objects) hw1.cpp -o hw1

run: clean hw1
	./hw1

clean:
	rm -f hw1 *.o