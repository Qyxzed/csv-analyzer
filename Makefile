CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic
DEBUG_FLAGS = -g -O0
SANITIZE_FLAGS = -fsanitize=address,undefined -fno-sanitize-recover=all
RELEASE_FLAGS = -O3 -DNDEBUG

SRC_DIR = src
BUILD_DIR = build

SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/csv_parser.c $(SRC_DIR)/binary_io.c $(SRC_DIR)/analyzer.c $(SRC_DIR)/types.c
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

TARGET = csvstat

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

sanitize: CFLAGS += $(SANITIZE_FLAGS)
sanitize: clean all

release: CFLAGS += $(RELEASE_FLAGS)
release: clean all

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all sanitize release clean