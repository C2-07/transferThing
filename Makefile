# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude

# Project name
TARGET = tt

# Directories
SRC_DIR = src
OBJ_DIR = obj
INCLUDE_DIR = include

# Find all .c files in the source directory, excluding test files
SRCS = $(filter-out $(SRC_DIR)/test.c, $(wildcard $(SRC_DIR)/*.c))

# Generate object file names from source files, placing them in the OBJ_DIR
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Default rule
all: $(TARGET)

# Rule to link the program
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $^

# Rule to compile a .c file from src/ into a .o file in obj/
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to clean up the project
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# Phony targets
.PHONY: all clean