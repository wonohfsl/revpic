# ------------------------------------------------------------
# Project: allcore-control
# Makefile for main app + test binaries
# ------------------------------------------------------------

CC       := gcc
CFLAGS   := -Wall -Wextra -O2 -g
INCLUDE  := -Isrc -Isrc/include -Isrc/hal -Isrc/config
LDFLAGS  := -lm

SRC_DIR  := src
TEST_DIR := test
BUILD_DIR := build

# ------------------------------------------------------------
# Source discovery
# ------------------------------------------------------------

SRC_FILES := $(shell find $(SRC_DIR) -name "*.c")
OBJ_FILES := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC_FILES))

# Remove main.o from test builds
TEST_OBJ_FILES := $(filter-out $(BUILD_DIR)/src/app/main.o,$(OBJ_FILES))

# Test files
TEST_FILES := $(wildcard $(TEST_DIR)/test_*.c)
TEST_BINS  := $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/%,$(TEST_FILES))
TEST_NAMES := $(patsubst $(TEST_DIR)/test_%.c,test_%,$(TEST_FILES))

MAIN_BIN := $(BUILD_DIR)/main

# ------------------------------------------------------------
# Default target
# ------------------------------------------------------------
all: $(MAIN_BIN)

# ------------------------------------------------------------
# Build a specific test
# ------------------------------------------------------------
test_%: $(BUILD_DIR)/test_%


$(BUILD_DIR)/test_%: $(TEST_DIR)/test_%.c $(TEST_OBJ_FILES)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDE) $< $(TEST_OBJ_FILES) -o $@ $(LDFLAGS)

# ------------------------------------------------------------
# Build all tests
# ------------------------------------------------------------
test: $(TEST_BINS)

$(BUILD_DIR)/%: $(TEST_DIR)/%.c $(TEST_OBJ_FILES)
	$(CC) $(CFLAGS) $(INCLUDE) $< $(TEST_OBJ_FILES) -o $@ $(LDFLAGS)

# ------------------------------------------------------------
# Build main application
# ------------------------------------------------------------
$(MAIN_BIN): $(OBJ_FILES)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@ $(LDFLAGS)

# ------------------------------------------------------------
# Build object files (IMPORTANT: prefix src/)
# ------------------------------------------------------------
$(BUILD_DIR)/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

# ------------------------------------------------------------
# Create build directory
# ------------------------------------------------------------
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# ------------------------------------------------------------
# Clean
# ------------------------------------------------------------
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean test $(TEST_NAMES)
