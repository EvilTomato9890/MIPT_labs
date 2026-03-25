#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asserts.h"
#include "logger.h"
#include "sortings.h"
#include "testing.h"

typedef struct named_sorting {
    const char *name;
    sorting_fn fn;
} named_sorting;

static int compare_int(const void *lhs, const void *rhs) {
    int a = *(const int *)lhs;
    int b = *(const int *)rhs;
    return (a > b) - (a < b);
}

static void qsort_stdlib_sort(int *arr, size_t n) {
    qsort(arr, n, sizeof(arr[0]), compare_int);
}

static void quick_median3_pivot(int *arr, size_t n) { quick_best_sort(arr, n, PIVOT_MEDIAN3); }
static void introsort_t32_d2_k4(int *arr, size_t n) { introsort(arr, n, 32U, 4U, 2.0); }

static void run_group(const dataset_config *cfg, const named_sorting *arr, size_t count) {
    for (size_t i = 0U; i < count; ++i) {
        LOGGER_INFO("run %s", arr[i].name);
        timing_array times = run_sorting_dataset(cfg, arr[i].fn, arr[i].name);
        timing_array_free(&times);
    }
}

static void run_point_8(const dataset_config *cfg) {
    named_sorting list[] = {
        {"quick_3way_pivot_median3_baseline", quick_median3_pivot},
        {"introsort_threshold32_depth2_heap4", introsort_t32_d2_k4},
        {"timsort_hybrid_runs", timsort_sort},
        {"pdqsort_pattern_defeating", pdqsort_sort}
    };
    run_group(cfg, list, sizeof(list) / sizeof(list[0]));
}

static void run_point_9(const dataset_config *cfg) {
    named_sorting list[] = {
        {"radix_lsd_bytewise", lsd_radix_sort},
        {"radix_msd_bytewise", msd_radix_sort}
    };
    run_group(cfg, list, sizeof(list) / sizeof(list[0]));
}

static void run_point_10(const dataset_config *cfg) {
    named_sorting list[] = {
        {"quick_3way_pivot_median3_best", quick_median3_pivot},
        {"introsort_threshold32_depth2_heap4", introsort_t32_d2_k4},
        {"pdqsort_pattern_defeating", pdqsort_sort},
        {"radix_lsd_bytewise", lsd_radix_sort},
        {"timsort_hybrid_runs", timsort_sort},
        {"qsort_stdlib_reference", qsort_stdlib_sort}
    };
    run_group(cfg, list, sizeof(list) / sizeof(list[0]));
}

int main(int argc, char **argv) {
    HARD_ASSERT(argc == 8,
                "usage: tester <point> <tests_dir> <csv> <from> <to> <step> <copies>");

    dataset_config cfg = {
        .tests_dir = argv[2],
        .result_csv = argv[3],
        .from = (size_t)strtoull(argv[4], NULL, 10),
        .to = (size_t)strtoull(argv[5], NULL, 10),
        .step = (size_t)strtoull(argv[6], NULL, 10),
        .copies = (size_t)strtoull(argv[7], NULL, 10)
    };

    FILE *out = fopen(cfg.result_csv, "w");
    SOFT_ASSERT_FUNCTIONAL(out != NULL,
                           "tester: cannot open result csv",
                           return 1);
    fprintf(out, "algorithm,size,seconds\n");
    fclose(out);

    if (strcmp(argv[1], "p8") == 0) {
        run_point_8(&cfg);
        return 0;
    }
    if (strcmp(argv[1], "p9") == 0) {
        run_point_9(&cfg);
        return 0;
    }
    if (strcmp(argv[1], "p10") == 0) {
        run_point_10(&cfg);
        return 0;
    }

    LOGGER_ERROR("unknown point '%s'", argv[1]);
    return 2;
}
