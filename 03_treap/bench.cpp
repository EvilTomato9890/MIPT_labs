#include "tree.h"

#include "../common/bench_common.h"

int main(void) {
    const struct BenchCase cases[] = {
        {"random", 100000, 0, 5},
        {"random", 200000, 0, 5},
        {"random", 300000, 0, 5},
        {"random", 400000, 0, 5},
        {"random", 500000, 0, 5},
        {"random", 600000, 0, 5},
        {"random", 700000, 0, 5},
        {"random", 800000, 0, 5},
        {"random", 900000, 0, 5},
        {"random", 1000000, 0, 5},
    };

    return lab_run_benchmarks(tree_name(),
                              cases,
                              (int) (sizeof(cases) / sizeof(cases[0])),
                              "results/raw.csv",
                              "results/summary.csv");
}
