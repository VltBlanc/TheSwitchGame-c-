CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -I include
LDFLAGS = -L lib -lraylib -lwinmm -lgdi32 -lopengl32

# Single Target: The Game (starts with menu)
TARGET = the_switch_game.exe
SOURCES = main.c menu.c scores.c survivor.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	del /Q *.o *.exe

run: $(TARGET)
	./$(TARGET)