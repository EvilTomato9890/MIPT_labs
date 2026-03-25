CC := gcc
CFLAGS := -std=c11 -O2 -Wall -Wextra -Werror -Wpedantic -D_POSIX_C_SOURCE=200809L -Iinclude -I../lab1/common/include
COMMON_SRC := ../lab1/common/source/logger.c
CHECK_DIR := .check_tmp

ifeq ($(OS),Windows_NT)
	MKDIR_BIN = if not exist bin mkdir bin
	MKDIR_CHECK = if not exist $(CHECK_DIR) mkdir $(CHECK_DIR)
	RM_DIR = if exist bin rmdir /S /Q bin
	RUN_GEN = bin\\gen_random
	RUN_QSORT = bin\\qsort_solver
	RUN_TESTER = bin\\tester
	CHECK_IN = $(CHECK_DIR)\\20_0.in
	CHECK_OUT = $(CHECK_DIR)\\20_0.out
	CHECK_CSV = $(CHECK_DIR)\\result.csv
else
	MKDIR_BIN = mkdir -p bin
	MKDIR_CHECK = mkdir -p $(CHECK_DIR)
	RM_DIR = rm -rf bin
	RUN_GEN = ./bin/gen_random
	RUN_QSORT = ./bin/qsort_solver
	RUN_TESTER = ./bin/tester
	CHECK_IN = $(CHECK_DIR)/20_0.in
	CHECK_OUT = $(CHECK_DIR)/20_0.out
	CHECK_CSV = $(CHECK_DIR)/result.csv
endif

all: bin/gen_random bin/qsort_solver bin/tester

bin/gen_random: source/gen_random.c $(COMMON_SRC) include/sortings.h
	@$(MKDIR_BIN)
	$(CC) $(CFLAGS) $< $(COMMON_SRC) -o $@

bin/qsort_solver: source/qsort_solver.c $(COMMON_SRC)
	@$(MKDIR_BIN)
	$(CC) $(CFLAGS) $< $(COMMON_SRC) -o $@

bin/tester: source/tester.c source/testing.c source/sortings.c $(COMMON_SRC)
	@$(MKDIR_BIN)
	$(CC) $(CFLAGS) $^ -lm -o $@

check: all
	@$(MKDIR_CHECK)
	$(RUN_GEN) 20 100 > $(CHECK_IN)
	$(RUN_QSORT) < $(CHECK_IN) > $(CHECK_OUT)
	$(RUN_TESTER) p1 $(CHECK_DIR) $(CHECK_CSV) 20 20 1 1

clean:
	$(RM_DIR)

.PHONY: all check clean
