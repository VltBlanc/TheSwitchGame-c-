# Makefile for Retro Game Menu
# Requires Raylib to be installed
# Works on Linux, macOS, and Windows (MinGW)

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -Iinclude

# Detect operating system
UNAME_S := $(shell uname -s)

# Build directory
BUILD_DIR = build

# Platform-specific libraries and commands
ifeq ($(OS),Windows_NT)
    # Windows (MinGW)
    LDFLAGS = -lraylib -lwinmm -lgdi32 -lopengl32
    EXE_EXT = .exe
    RM_CMD = rmdir /s /q
    MKDIR_CMD = mkdir
else
    # Unix-like systems (Linux, macOS)
    EXE_EXT =
    RM_CMD = rm -rf
    MKDIR_CMD = mkdir -p

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

# Source files
SOURCES = main.c menu.c scores.c survivor.c hyperspace.c snake.c jeuxLuca.c
VALENTIN_SOURCES = Jeu_Valentin/jeu_valentin.c Jeu_Valentin/etoile.c
OBJS = $(addprefix $(BUILD_DIR)/, $(SOURCES:.c=.o)) $(BUILD_DIR)/jeu_valentin.o $(BUILD_DIR)/etoile.o
TARGET = the_switch_game$(EXE_EXT)

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Compile object files from main directory
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile Valentin game files from subdirectory
$(BUILD_DIR)/jeu_valentin.o: Jeu_Valentin/jeu_valentin.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/etoile.o: Jeu_Valentin/etoile.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create build directory
$(BUILD_DIR):
	$(MKDIR_CMD) $@

# Run the game
run: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	-rm -rf $(BUILD_DIR) $(TARGET)

# Show build info
info:
	@echo "Detected OS: $(UNAME_S)"
	@echo "Target executable: $(TARGET)"
	@echo "Linker flags: $(LDFLAGS)"

# Platform-specific installation help
help-platform:
	@echo "Platform-specific Raylib installation instructions:"
	@echo ""
	@if [ "$(UNAME_S)" = "Darwin" ]; then \
		echo "macOS: brew install raylib"; \
	elif [ "$(UNAME_S)" = "Linux" ]; then \
		echo "Linux (Ubuntu/Debian): sudo apt install libraylib-dev"; \
		echo "Linux (Arch): sudo pacman -S raylib"; \
		echo "Linux (Fedora): sudo dnf install raylib-devel"; \
	else \
		echo "Windows: Download from https://www.raylib.com/ or use vcpkg"; \
	fi

.PHONY: all debug clean install-deps install-deps-arch check-deps run help info help-platform