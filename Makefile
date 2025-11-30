# Makefile for Retro Game Menu
# Requires Raylib to be installed
# Works on Linux, macOS, and Windows (MinGW)

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2

# Detect operating system
UNAME_S := $(shell uname -s)

# Platform-specific libraries
ifeq ($(OS),Windows_NT)
    # Windows (MinGW)
    LDFLAGS = -lraylib -lwinmm -lgdi32 -lopengl32
    EXE_EXT = .exe
else
    # Unix-like systems (Linux, macOS)
    EXE_EXT =

    ifeq ($(UNAME_S),Darwin)
        # macOS
        RAYLIB_PATH := $(shell brew --prefix raylib 2>/dev/null)
        ifeq ($(RAYLIB_PATH),)
            # Fallback if brew not found
            LDFLAGS = -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreAudio -framework CoreVideo
        else
            # Check if raylib framework exists
            FRAMEWORK_EXISTS := $(shell test -d $(RAYLIB_PATH)/lib/raylib.framework && echo yes || echo no)
            ifeq ($(FRAMEWORK_EXISTS),yes)
                # Use framework from Homebrew
                LDFLAGS = -framework raylib -F $(RAYLIB_PATH)/lib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreAudio -framework CoreVideo
                CFLAGS += -I $(RAYLIB_PATH)/include
            else
                # Use traditional library from Homebrew
                LDFLAGS = -L $(RAYLIB_PATH)/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreAudio -framework CoreVideo
                CFLAGS += -I $(RAYLIB_PATH)/include
            endif
        endif
    else ifeq ($(UNAME_S),Linux)
        # Linux
        LDFLAGS = -lraylib -lm -lpthread -ldl -lrt -lX11 -lGL
    endif
endif

# Common commands (assuming unix-like shell available via MinGW/Git Bash)
RM_CMD = rm -rf
MKDIR_CMD = mkdir -p

# Directories
BUILD_DIR = build
SRC_DIR = .

# Sources (wildcard for all .c files in current directory, excluding test_stdio.c)
SRCS = $(filter-out $(SRC_DIR)/test_stdio.c, $(wildcard $(SRC_DIR)/*.c))
# Objects in build directory
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

TARGET = the_switch_game$(EXE_EXT)

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Compile object files
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create build directory
$(BUILD_DIR):
	$(MKDIR_CMD) $@

# Run the game
run: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	$(RM_CMD) $(BUILD_DIR) $(TARGET)

# Show build info
info:
	@echo "Detected OS: $(UNAME_S)"
	@echo "Target executable: $(TARGET)"
	@echo "Linker flags: $(LDFLAGS)"
	@echo "Sources: $(SRCS)"
	@echo "Objects: $(OBJS)"

.PHONY: all clean run info