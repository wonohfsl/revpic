# ------------------------------------------------------------
# Project: allcore-control
# General Makefile for main app + test binaries
# ------------------------------------------------------------

CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -g
INCLUDE := -Isrc -Isrc/include -Isrc/hal -Isrc/config -Isrc/utils

# Directories
SRC_DIR   := src
HAL_DIR   := src/hal
TEST_DIR  := test
BUILD_DIR := build

# HAL sources (mio.c, piControlIf.c, ro.c when added)
HAL_SRCS := $(wildcard $(HAL_DIR)/*.c)

# Main application source
MAIN_SRC := $(SRC_DIR)/main.c
MAIN_BIN := $(BUILD_DIR)/main

# Test sources (test_mio.c, test_ro.c when added)
TEST_SRCS := $(wildcard $(TEST_DIR)/*.c)
TEST_BINS := $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/%,$(TEST_SRCS))

# ------------------------------------------------------------
# Default target: build main
# ------------------------------------------------------------
all: main

# ------------------------------------------------------------
# Build main application
# ------------------------------------------------------------
main: $(BUILD_DIR) $(MAIN_BIN)

$(MAIN_BIN): $(MAIN_SRC) $(HAL_SRCS)
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@

# ------------------------------------------------------------
# Build all test binaries
# ------------------------------------------------------------
test: $(BUILD_DIR) $(TEST_BINS)

$(BUILD_DIR)/%: $(TEST_DIR)/%.c $(HAL_SRCS)
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@

# ------------------------------------------------------------
# Create build directory if missing
# ------------------------------------------------------------
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ------------------------------------------------------------
# Clean
# ------------------------------------------------------------
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all main test clean

