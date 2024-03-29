.PHONY: run clean

SRCDIR = src
OBJDIR = obj
CPP = g++
CPPFLAGS = -Wall
OBJ = $(addprefix $(OBJDIR)/, consts.o utils.o proc.o)

all: clean hw1

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CPP) -c $< -o $@

hw1: hw1.cpp $(OBJDIR) $(OBJ)
	$(CPP) $(CPPFLAGS) $(OBJ) hw1.cpp -o hw1

run: clean hw1
	./hw1

clean:
	rm -f hw1
	rm -rf $(OBJDIR)