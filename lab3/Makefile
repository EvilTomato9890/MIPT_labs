CC := gcc
CFLAGS := -std=c11 -O2 -Wall -Wextra -Werror -Wpedantic -D_POSIX_C_SOURCE=200809L -Iinclude -I../lab1/common/include
COMMON_SRC := ../lab1/common/source/logger.c

all: bin/gen_random bin/qsort_solver bin/tester

bin/gen_random: source/gen_random.c $(COMMON_SRC) include/sortings.h
	@mkdir -p bin
	$(CC) $(CFLAGS) $< $(COMMON_SRC) -o $@

bin/qsort_solver: source/qsort_solver.c $(COMMON_SRC)
	@mkdir -p bin
	$(CC) $(CFLAGS) $< $(COMMON_SRC) -o $@

bin/tester: source/tester.c source/testing.c source/sortings.c $(COMMON_SRC)
	@mkdir -p bin
	$(CC) $(CFLAGS) $^ -lm -o $@

check: all
	./bin/gen_random 20 100 > /tmp/lab3_demo.in
	./bin/qsort_solver < /tmp/lab3_demo.in > /tmp/lab3_demo.out
	mkdir -p /tmp/lab3_tests
	cp /tmp/lab3_demo.in /tmp/lab3_tests/20_0.in
	cp /tmp/lab3_demo.out /tmp/lab3_tests/20_0.out
	./bin/tester p1 /tmp/lab3_tests /tmp/lab3.csv 20 20 1 1

clean:
	rm -rf bin

.PHONY: all check clean
