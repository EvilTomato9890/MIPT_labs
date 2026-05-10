CXX := g++
CXXFLAGS := -std=gnu++17 -O2 -Wall -Wextra -Wpedantic -Wno-unused-function -I../common
BIN_DIR := bin
MKDIR_P := python -c "import os,sys; [os.makedirs(p, exist_ok=True) for p in sys.argv[1:]]"

.PHONY: build test bench plots clean

build: $(BIN_DIR)/test.exe $(BIN_DIR)/bench.exe

$(BIN_DIR):
	$(MKDIR_P) $(BIN_DIR)

$(BIN_DIR)/test.exe: tree.cpp test.cpp tree.h ../common/test_common.h ../common/lab_common.h | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) tree.cpp test.cpp -o $@

$(BIN_DIR)/bench.exe: tree.cpp bench.cpp tree.h ../common/bench_common.h ../common/lab_common.h | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) tree.cpp bench.cpp -o $@

test: $(BIN_DIR)/test.exe
	./$(BIN_DIR)/test.exe

bench: $(BIN_DIR)/bench.exe
	./$(BIN_DIR)/bench.exe

plots:
	powershell -NoProfile -Command "python plot.py"

clean:
	python -c "import shutil; shutil.rmtree('bin', ignore_errors=True); shutil.rmtree('results', ignore_errors=True); shutil.rmtree('plots', ignore_errors=True)"
