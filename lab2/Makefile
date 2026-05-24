CC := gcc
CFLAGS := -std=c11 -O2 -Wall -Wextra -Werror -Wpedantic -D_POSIX_C_SOURCE=200809L -Iinclude -Icommon/include
COMMON_SRC := common/source/logger.c
CHECK_DIR := .check_tmp
BIN_DIR := bin
EXE_EXT :=
GEN_BIN := $(BIN_DIR)/gen_random$(EXE_EXT)
QSORT_BIN := $(BIN_DIR)/qsort_solver$(EXE_EXT)
TESTER_BIN := $(BIN_DIR)/tester$(EXE_EXT)

ifeq ($(OS),Windows_NT)
	EXE_EXT := .exe
	GEN_BIN := $(BIN_DIR)/gen_random$(EXE_EXT)
	QSORT_BIN := $(BIN_DIR)/qsort_solver$(EXE_EXT)
	TESTER_BIN := $(BIN_DIR)/tester$(EXE_EXT)
	MKDIR_BIN = if not exist "$(BIN_DIR)" mkdir "$(BIN_DIR)"
	MKDIR_CHECK = if not exist "$(CHECK_DIR)" mkdir "$(CHECK_DIR)"
	RM_BIN = if exist "$(BIN_DIR)" rmdir /S /Q "$(BIN_DIR)"
	RM_CHECK = if exist "$(CHECK_DIR)" rmdir /S /Q "$(CHECK_DIR)"
	RUN_GEN = $(subst /,\,$(GEN_BIN))
	RUN_QSORT = $(subst /,\,$(QSORT_BIN))
	RUN_TESTER = $(subst /,\,$(TESTER_BIN))
	CHECK_IN = $(CHECK_DIR)\\20_0.in
	CHECK_OUT = $(CHECK_DIR)\\20_0.out
	CHECK_CSV = $(CHECK_DIR)\\result.csv
else
	MKDIR_BIN = mkdir -p "$(BIN_DIR)"
	MKDIR_CHECK = mkdir -p "$(CHECK_DIR)"
	RM_BIN = rm -rf "$(BIN_DIR)"
	RM_CHECK = rm -rf "$(CHECK_DIR)"
	RUN_GEN = ./$(GEN_BIN)
	RUN_QSORT = ./$(QSORT_BIN)
	RUN_TESTER = ./$(TESTER_BIN)
	CHECK_IN = $(CHECK_DIR)/20_0.in
	CHECK_OUT = $(CHECK_DIR)/20_0.out
	CHECK_CSV = $(CHECK_DIR)/result.csv
endif

all: $(GEN_BIN) $(QSORT_BIN) $(TESTER_BIN)

$(GEN_BIN): source/gen_random.c $(COMMON_SRC) include/sortings.h
	@$(MKDIR_BIN)
	$(CC) $(CFLAGS) $< $(COMMON_SRC) -o $@

$(QSORT_BIN): source/qsort_solver.c $(COMMON_SRC)
	@$(MKDIR_BIN)
	$(CC) $(CFLAGS) $< $(COMMON_SRC) -o $@

$(TESTER_BIN): source/tester.c source/testing.c source/sortings.c $(COMMON_SRC)
	@$(MKDIR_BIN)
	$(CC) $(CFLAGS) $^ -lm -o $@

check: all
	@$(MKDIR_CHECK)
	"$(RUN_GEN)" 20 100 1 > "$(CHECK_IN)"
	"$(RUN_QSORT)" < "$(CHECK_IN)" > "$(CHECK_OUT)"
	"$(RUN_TESTER)" p1 "$(CHECK_DIR)" "$(CHECK_CSV)" 20 20 1 1

clean:
	$(RM_BIN)
	$(RM_CHECK)

.PHONY: all check clean
