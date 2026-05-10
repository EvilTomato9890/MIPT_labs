SUBDIRS := 01_naive 02_avl 03_treap 04_splay 05_rbtree 06_btree 07_skiplist
BUILD_TARGETS := $(SUBDIRS:%=%-build)
TEST_TARGETS := $(SUBDIRS:%=%-test)
BENCH_TARGETS := $(SUBDIRS:%=%-bench)
PLOT_TARGETS := $(SUBDIRS:%=%-plots)
CLEAN_TARGETS := $(SUBDIRS:%=%-clean)

.PHONY: all build test bench plots report clean
.PHONY: $(BUILD_TARGETS) $(TEST_TARGETS) $(BENCH_TARGETS) $(PLOT_TARGETS) $(CLEAN_TARGETS)

all: test bench plots report

build: $(BUILD_TARGETS)

test: $(TEST_TARGETS)

bench: $(BENCH_TARGETS)

plots: $(PLOT_TARGETS)

report:
	$(MAKE) -C 08_conclusion report

clean: $(CLEAN_TARGETS)
	$(MAKE) -C 08_conclusion clean

$(BUILD_TARGETS): %-build:
	$(MAKE) -C $* build

$(TEST_TARGETS): %-test:
	$(MAKE) -C $* test

$(BENCH_TARGETS): %-bench:
	$(MAKE) -C $* bench

$(PLOT_TARGETS): %-plots:
	$(MAKE) -C $* plots

$(CLEAN_TARGETS): %-clean:
	$(MAKE) -C $* clean
