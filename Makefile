# Compiler and flags
CC = gcc
FLAGS = -g -Wall `pkg-config --cflags sdl3 sdl3-ttf`
LFLAGS = 

# Source files and object files
OBJS = editv/config.o editv/main.o editv/storage.o
SOURCE = editv/config.c editv/main.c editv/storage.c
HEADER = editv/config.h editv/storage.h
OUT = editv/editv

# Libraries
LDLIBS = `pkg-config --libs sdl3 sdl3-ttf`

# Default target
all: $(OUT)

# Linking rules
$(OUT): $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS) $(LDLIBS)

# Compilation rules
%.o: %.cpp $(HEADER)
	$(CC) $(FLAGS) -o $@ $<

# Clean rule
clean:
	rm -f $(OBJS) $(OUT)
