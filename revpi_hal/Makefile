CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -g
SRC_APP = src/main.c src/hal_core.c src/hal_revpi.c src/hal_sim.c
SRC_TEST = src/tests.c src/mock_io.c src/hal_core.c src/hal_revpi.c src/hal_sim.c
TARGET_APP  = myapp
TARGET_TEST = hal_tests
all: $(TARGET_APP)
$(TARGET_APP): $(SRC_APP)
	$(CC) $(CFLAGS) $(SRC_APP) -o $(TARGET_APP)
$(TARGET_TEST): $(SRC_TEST)
	$(CC) $(CFLAGS) -DHAL_TEST_MAIN $(SRC_TEST) -o $(TARGET_TEST)
clean:
	rm -f $(TARGET_APP) $(TARGET_TEST)
