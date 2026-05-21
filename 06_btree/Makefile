CXX ?= c++
CXXFLAGS ?= -std=gnu++17 -O2 -Wall -Wextra -Wpedantic -Wno-unused-function -I../common
BIN_DIR := bin
ifeq ($(OS),Windows_NT)
PYTHON ?= $(shell powershell -NoProfile -Command "(Get-Command python).Source -replace '\\','/'")
EXEEXT := .exe
else
PYTHON ?= python3
EXEEXT :=
endif
TEST_BIN := $(BIN_DIR)/test$(EXEEXT)
BENCH_BIN := $(BIN_DIR)/bench$(EXEEXT)
MKDIR_P := $(PYTHON) -c "import os,sys; [os.makedirs(p, exist_ok=True) for p in sys.argv[1:]]"

.PHONY: build test bench plots clean

build: $(TEST_BIN) $(BENCH_BIN)

$(BIN_DIR):
	$(MKDIR_P) $(BIN_DIR)

$(TEST_BIN): tree.cpp test.cpp tree.h ../common/test_common.h ../common/lab_common.h | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) tree.cpp test.cpp -o $@

$(BENCH_BIN): tree.cpp bench.cpp tree.h ../common/bench_common.h ../common/lab_common.h | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) tree.cpp bench.cpp -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

bench: $(BENCH_BIN)
	./$(BENCH_BIN)

plots:
	$(PYTHON) plot.py

clean:
	$(PYTHON) -c "import shutil; shutil.rmtree('bin', ignore_errors=True); shutil.rmtree('results', ignore_errors=True); shutil.rmtree('plots', ignore_errors=True)"
