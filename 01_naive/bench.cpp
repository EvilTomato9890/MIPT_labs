#include "tree.h"

#include "../common/bench_common.h"

int main(void) {
    const struct BenchCase cases[] = {
        {"random", 100000, 0, 5},
        {"sorted", 10000, 1, 5},
    };

    return lab_run_benchmarks(tree_name(),
                              cases,
                              (int) (sizeof(cases) / sizeof(cases[0])),
                              "results/raw.csv",
                              "results/summary.csv");
}
