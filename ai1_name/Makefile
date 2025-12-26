CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -g

SRC     = src/main.c
OBJ     = $(SRC:.c=.o)
TARGET  = myapp

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)
