CC = gcc
CFLAGS = -Wall -Wextra -D_POSIX_C_SOURCE=200809L

BUILD_DIR = build
TARGETS = $(BUILD_DIR)/server $(BUILD_DIR)/client

all: $(TARGETS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/server: $(BUILD_DIR)/server.o
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/client: $(BUILD_DIR)/client.o
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
