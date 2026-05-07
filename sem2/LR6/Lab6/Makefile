CC = gcc
CFLAGS = -Wall -Wextra -pthread
TARGET = sort_test

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) main.c -o $(TARGET)

clean:
	rm -f $(TARGET)

test: all
	@echo "Running test with 1 thread (baseline)..."
	./$(TARGET) 1000000 1
	@echo "\nRunning test with 2 threads..."
	./$(TARGET) 1000000 2
	@echo "\nRunning test with 4 threads..."
	./$(TARGET) 1000000 4
	@echo "\nRunning test with 8 threads..."
	./$(TARGET) 1000000 8
