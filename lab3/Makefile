.DEFAULT_GOAL := all

CC ?= gcc

BUILD_DIR := build
BIN_DIR := $(BUILD_DIR)/bin
CHECK_DIR := $(BUILD_DIR)/check
CHECK_HEAP_SIZE := 10000
CHECK_HEAP_LIMIT := 1000
CHECK_DIJKSTRA_FROM := 200
CHECK_DIJKSTRA_TO := 1000
CHECK_DIJKSTRA_STEP := 200
CHECK_COPIES := 1

ROOT_INC_DIR := include
COMMON_INC_DIR := common/include
BINARY_HEAP_INC_DIR := binary_heap/include
BINOMIAL_HEAP_INC_DIR := binomial_heap/include
FIBONACCI_HEAP_INC_DIR := fibonacci_heap/include

CPPFLAGS += -D_POSIX_C_SOURCE=200809L \
            -I$(ROOT_INC_DIR) \
            -I$(COMMON_INC_DIR) \
            -I$(BINARY_HEAP_INC_DIR) \
            -I$(BINOMIAL_HEAP_INC_DIR) \
            -I$(FIBONACCI_HEAP_INC_DIR)
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -g
LDFLAGS ?=
LDLIBS ?=

COMMON_SRC := common/source/logger.c common/source/vector.c
HEAP_SRC := binary_heap/source/binary_heap.c binomial_heap/source/binomial_heap.c
TESTING_SRC := source/testing.c
DIJKSTRA_SRC := source/dijkstra.c fibonacci_heap/source/fibonacci_heap.c

GEN_RANDOM_BIN := $(BIN_DIR)/gen_random
QSORT_BIN := $(BIN_DIR)/qsort_solver
TESTER_BIN := $(BIN_DIR)/tester

.PHONY: all check benchmark plots clean help

all: $(GEN_RANDOM_BIN) $(QSORT_BIN) $(TESTER_BIN)

help:
	@echo "Available targets:"
	@echo "  make all       Build gen_random, qsort_solver and tester"
	@echo "  make check     Run a small smoke test for all points"
	@echo "  make benchmark Generate datasets and run all benchmark points"
	@echo "  make plots     Rebuild plots from existing CSV files"
	@echo "  make clean     Remove build artifacts"

$(BIN_DIR):
	mkdir -p "$@"

$(CHECK_DIR):
	mkdir -p "$@"

$(GEN_RANDOM_BIN): source/gen_random.c $(COMMON_SRC) | $(BIN_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(QSORT_BIN): source/qsort_solver.c $(COMMON_SRC) | $(BIN_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(TESTER_BIN): source/tester.c $(TESTING_SRC) $(DIJKSTRA_SRC) $(HEAP_SRC) $(COMMON_SRC) | $(BIN_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

check: all | $(CHECK_DIR)
	"$(GEN_RANDOM_BIN)" $(CHECK_HEAP_SIZE) $(CHECK_HEAP_LIMIT) 42 > "$(CHECK_DIR)/$(CHECK_HEAP_SIZE)_0.in"
	"$(QSORT_BIN)" < "$(CHECK_DIR)/$(CHECK_HEAP_SIZE)_0.in" > "$(CHECK_DIR)/$(CHECK_HEAP_SIZE)_0.out"
	"$(TESTER_BIN)" p1 "$(CHECK_DIR)" "$(CHECK_DIR)/point1.csv" $(CHECK_HEAP_SIZE) $(CHECK_HEAP_SIZE) 1 $(CHECK_COPIES)
	"$(TESTER_BIN)" p2 "$(CHECK_DIR)" "$(CHECK_DIR)/point2.csv" $(CHECK_HEAP_SIZE) $(CHECK_HEAP_SIZE) 1 $(CHECK_COPIES)
	"$(TESTER_BIN)" p3_sparse "$(CHECK_DIR)" "$(CHECK_DIR)/point3_sparse.csv" $(CHECK_DIJKSTRA_FROM) $(CHECK_DIJKSTRA_TO) $(CHECK_DIJKSTRA_STEP) $(CHECK_COPIES)
	"$(TESTER_BIN)" p3_dense "$(CHECK_DIR)" "$(CHECK_DIR)/point3_dense.csv" $(CHECK_DIJKSTRA_FROM) $(CHECK_DIJKSTRA_TO) $(CHECK_DIJKSTRA_STEP) $(CHECK_COPIES)

benchmark: all
	bash ./scripts/generate_tests.sh tests 2147483647 42 all
	bash ./scripts/run_all_points.sh tests results

plots:
	bash ./scripts/plot_results.sh all results/csv results/plots

clean:
	rm -rf "$(BUILD_DIR)"
