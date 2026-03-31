.DEFAULT_GOAL := all

CC ?= gcc
AR ?= ar

LAB3_DIR := lab3
COMMON_DIR := $(LAB3_DIR)/common
BINARY_HEAP_DIR := $(LAB3_DIR)/binary_heap

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
LIB_DIR := $(BUILD_DIR)/lib

COMMON_SRCS := $(wildcard $(COMMON_DIR)/source/*.c)
BINARY_HEAP_SRCS := $(wildcard $(BINARY_HEAP_DIR)/source/*.c)

COMMON_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(COMMON_SRCS))
BINARY_HEAP_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(BINARY_HEAP_SRCS))
DEPS := $(COMMON_OBJS:.o=.d) $(BINARY_HEAP_OBJS:.o=.d)

COMMON_LIB := $(LIB_DIR)/libcommon.a
BINARY_HEAP_LIB := $(LIB_DIR)/libbinary_heap.a

CPPFLAGS += -D_POSIX_C_SOURCE=200809L -I$(COMMON_DIR)/include -I$(BINARY_HEAP_DIR)/include
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -g

.PHONY: all clean rebuild help

all: $(COMMON_LIB) $(BINARY_HEAP_LIB)

help:
	@echo "Available targets:"
	@echo "  make all      Build lab3 static libraries into $(LIB_DIR)"
	@echo "  make rebuild  Remove build artifacts and rebuild everything"
	@echo "  make clean    Remove the $(BUILD_DIR) directory"
	@echo "  make help     Show this help"

$(COMMON_LIB): $(COMMON_OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

$(BINARY_HEAP_LIB): $(BINARY_HEAP_OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

rebuild: clean all

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)
