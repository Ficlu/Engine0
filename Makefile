# Define variables
CC := gcc
CFLAGS := -Isrc/include/SDL2 -Isrc/include/GL -Isrc/include -Wall -Wextra -g
LDFLAGS := -Lsrc/lib
LIBS := -lmingw32 -lSDL2main -lSDL2 -lglew32 -lopengl32
SRC := gameloop.c 
OBJ := $(SRC:.c=.o)
TARGET := bin/gameloop.exe

# Default target
all: $(TARGET)

# Link the object files to create the executable
$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean up build files
clean:
	rm -f $(OBJ) $(TARGET)

# Phony targets
.PHONY: all clean
