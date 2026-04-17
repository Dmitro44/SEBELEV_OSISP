CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c18 -D_GNU_SOURCE -g -Iinclude
LDFLAGS = 

SRC_DIR = src
INC_DIR = include
BUILD_DIR = build

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TARGET = autocensor

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@if [ -z "$(OBJS)" ]; then \
		echo "No source files found in $(SRC_DIR). Nothing to build."; \
	else \
		$(CC) $(OBJS) -o $@ $(LDFLAGS); \
	fi

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
