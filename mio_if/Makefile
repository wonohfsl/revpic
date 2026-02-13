.PHONY: all clean
CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -g

SRC     = src/main.c src/piControlIf.c src/mio.c
OBJ     = $(SRC:.c=.o)
TARGET  = myapp

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

# Pattern rule: compile each .c into its own .o
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
