# Makefile for Retro Game Menu
# Requires Raylib to be installed
# Works on Linux, macOS, and Windows (MinGW)

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2

# Detect operating system
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# Platform-specific libraries
ifeq ($(OS),Windows_NT)
    # Windows (MinGW)
    LDFLAGS = -lraylib -lwinmm -lgdi32 -lopengl32
    EXE_EXT = .exe
    RM_CMD = del /Q
else
    # Unix-like systems (Linux, macOS)
    EXE_EXT =
    RM_CMD = rm -f

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
SOURCES = Jeu_valentin/jeu_valentin.c Jeu_valentin/etoile.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = the_switch_game$(EXE_EXT)

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)


# Clean build artifacts
clean:
	$(RM_CMD) $(OBJECTS) $(TARGET)

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