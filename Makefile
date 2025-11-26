CC = gcc
CFLAGS = -Wall -Wextra -g -I include
LDFLAGS = -L lib -lraylib -lopengl32 -lgdi32 -lwinmm
CC = gcc
CFLAGS = -Wall -Wextra -g -I include
LDFLAGS = -L lib -lraylib -lopengl32 -lgdi32 -lwinmm

# Target 1: The Menu System
TARGET_MAIN = main.exe
SOURCES_MAIN = main.c menu.c scores.c
OBJECTS_MAIN = $(SOURCES_MAIN:.c=.o)

# Target 2: Hugo's Game
TARGET_GAME = survivor.exe
SOURCES_GAME = survivor.c
OBJECTS_GAME = $(SOURCES_GAME:.c=.o)

all: $(TARGET_MAIN) $(TARGET_GAME)

$(TARGET_MAIN): $(OBJECTS_MAIN)
	$(CC) $(OBJECTS_MAIN) -o $(TARGET_MAIN) $(LDFLAGS)

$(TARGET_GAME): $(OBJECTS_GAME)
	$(CC) $(OBJECTS_GAME) -o $(TARGET_GAME) $(LDFLAGS)

run: $(TARGET_GAME)
	./$(TARGET_GAME)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	del /Q *.o *.exe