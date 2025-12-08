# Compiler
CC = clang
CFLAGS = -Wall -Wextra -Werror -std=c11 -pedantic
CFLAGS += -I./src -I./include

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# Source files
BASE_SRCS = $(wildcard $(SRC_DIR)/base/*.c)
DIAG_SRCS = $(wildcard $(SRC_DIR)/diagnostics/*.c)
PARSE_SRCS = $(wildcard $(SRC_DIR)/parsing/*.c)
RUNTIME_SRCS = $(wildcard $(SRC_DIR)/runtime/*.c)
BUILTIN_SRCS = $(wildcard $(SRC_DIR)/builtins/*.c)
ISO_SRCS = $(wildcard $(SRC_DIR)/isolation/*.c)
GC_SRCS = $(wildcard $(SRC_DIR)/gc/*.c)
CLI_SRCS = $(wildcard $(SRC_DIR)/cli/*.c)

ALL_SRCS = $(BASE_SRCS) $(DIAG_SRCS) $(PARSE_SRCS) $(RUNTIME_SRCS) \
           $(BUILTIN_SRCS) $(ISO_SRCS) $(GC_SRCS) $(CLI_SRCS)

OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(ALL_SRCS))

# Target
TARGET = $(BIN_DIR)/jbox

# Build modes
.PHONY: all debug release test clean

all: CFLAGS += -O2
all: $(TARGET)

debug: CFLAGS += -g -O0 -DJSBOX_DEBUG
debug: $(TARGET)

release: CFLAGS += -O3 -DNDEBUG -flto
release: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ -lm

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(TARGET)
	@echo "Running tests..."
	@./tests/run.sh

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Dependencies
-include $(OBJS:.o=.d)
