.PHONY: all clean
CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -g

SRC     = src/main.c
OBJ     = $(SRC:.c=.o)
TARGET  = myapp

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)
	rm -f $(OBJ)

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

clean:
	rm -f $(OBJ) $(TARGET)
