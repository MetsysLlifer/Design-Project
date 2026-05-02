CC = gcc

# Try pkg-config first; if not available, fall back to a local vendor/raylib folder
RAYLIB_CFLAGS := $(shell pkg-config --cflags raylib 2>/dev/null || echo)
RAYLIB_LIBS   := $(shell pkg-config --libs raylib 2>/dev/null || echo)

ifeq ($(RAYLIB_CFLAGS),)
	ifneq (,$(wildcard vendor/raylib/include/raylib.h))
		RAYLIB_CFLAGS = -Idvendor/raylib/include
		RAYLIB_CFLAGS = -Ivendor/raylib/include
		RAYLIB_LIBS = -Lvendor/raylib/lib -lraylib
	else
		$(warning pkg-config for raylib not found and no vendor/raylib present.)
		$(warning To build, either install raylib or add a prebuilt library under vendor/raylib.)
		RAYLIB_CFLAGS =
		RAYLIB_LIBS =
	endif
endif

CFLAGS = -Wall -std=c99 -Wno-missing-braces -Iinclude $(RAYLIB_CFLAGS)
LDFLAGS = $(RAYLIB_LIBS) -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

TARGET = $(BIN_DIR)/dsa_visualizer

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJ) | $(BIN_DIR)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean
