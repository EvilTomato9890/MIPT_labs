.DEFAULT_GOAL := all

CC ?= gcc

LAB3_DIR := lab3
COMMON_DIR := $(LAB3_DIR)/common
BINARY_HEAP_DIR := $(LAB3_DIR)/binary_heap
COMMON_INC_DIR := $(COMMON_DIR)/include
BINARY_HEAP_INC_DIR := $(BINARY_HEAP_DIR)/include

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin

ROOT_SRCS := main.c
COMMON_SRCS := $(wildcard $(COMMON_DIR)/source/*.c)
BINARY_HEAP_SRCS := $(wildcard $(BINARY_HEAP_DIR)/source/*.c)
SRCS := $(ROOT_SRCS) $(COMMON_SRCS) $(BINARY_HEAP_SRCS)

OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

TARGET := $(BIN_DIR)/main

CPPFLAGS += -D_POSIX_C_SOURCE=200809L -I$(COMMON_INC_DIR) -I$(BINARY_HEAP_INC_DIR)
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -g
LDFLAGS ?=
LDLIBS ?=

.PHONY: all run clean rebuild help

all: $(TARGET)

help:
	@echo "Available targets:"
	@echo "  make all      Build $(TARGET)"
	@echo "  make run      Build and run main.c"
	@echo "  make rebuild  Remove build artifacts and rebuild everything"
	@echo "  make clean    Remove the $(BUILD_DIR) directory"
	@echo "  make help     Show this help"

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

run: $(TARGET)
	$(TARGET)

rebuild: clean all

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)
