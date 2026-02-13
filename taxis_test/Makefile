.PHONY: all clean
CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -Isrc

SRC_DIR = src

# Automatically find all .c files in src/
SRC = $(wildcard $(SRC_DIR)/*.c)

# Convert .c → .o
OBJ = $(SRC:.c=.o)

TARGET = myapp

all: $(TARGET)

# Link all object files
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

# Compile rule for each .c file
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SRC_DIR)/*.o
	rm -f $(TARGET)
