# Makefile for Retro Game Menu
# Requires Raylib to be installed

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
LDFLAGS = -lraylib -lm -lpthread -ldl -lrt -lX11

# Source files
SOURCES = main.c menu.c scores.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = the_switch_game

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)


# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all debug clean install-deps install-deps-arch check-deps run help