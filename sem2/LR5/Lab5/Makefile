CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_DEFAULT_SOURCE -Iinclude

SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
TEST_DIR = tests

TARGET = analyzer
TEST_TARGET = test_signature

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Get all object files except main.o for the test
TEST_OBJS = $(filter-out $(BUILD_DIR)/main.o, $(OBJS))
TEST_SIGNATURE_SRC = $(TEST_DIR)/test_signature.c

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_TARGET): $(TEST_OBJS) $(TEST_SIGNATURE_SRC)
	$(CC) $(CFLAGS) $(TEST_SIGNATURE_SRC) $(TEST_OBJS) -o $(TEST_TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TEST_TARGET) tests/results.txt tests/mock_fs

test: $(TARGET) $(TEST_TARGET)
	bash $(TEST_DIR)/setup_mock_fs.sh
	./$(TEST_TARGET)
	bash $(TEST_DIR)/test_e2e.sh
