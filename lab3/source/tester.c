#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asserts.h"
#include "logger.h"
#include "return_macros.h"
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

static void qsort_stdlib_sort(int *arr, size_t n) { qsort(arr, n, sizeof(arr[0]), compare_int); }
static void quick_pivot_center(int *arr, size_t n) { quick_best_sort(arr, n, PIVOT_CENTER); }
static void quick_pivot_median3(int *arr, size_t n) { quick_best_sort(arr, n, PIVOT_MEDIAN3); }
static void quick_pivot_random(int *arr, size_t n) { quick_best_sort(arr, n, PIVOT_RANDOM); }
static void quick_pivot_m3rand(int *arr, size_t n) { quick_best_sort(arr, n, PIVOT_MEDIAN3_RANDOM); }
static void heap_k2(int *arr, size_t n) { heap_kary_sort(arr, n, 2U); }
static void heap_k3(int *arr, size_t n) { heap_kary_sort(arr, n, 3U); }
static void heap_k4(int *arr, size_t n) { heap_kary_sort(arr, n, 4U); }
static void heap_k5(int *arr, size_t n) { heap_kary_sort(arr, n, 5U); }
static void heap_k6(int *arr, size_t n) { heap_kary_sort(arr, n, 6U); }
static void heap_k7(int *arr, size_t n) { heap_kary_sort(arr, n, 7U); }
static void heap_k8(int *arr, size_t n) { heap_kary_sort(arr, n, 8U); }
static void heap_k9(int *arr, size_t n) { heap_kary_sort(arr, n, 9U); }
static void heap_k10(int *arr, size_t n) { heap_kary_sort(arr, n, 10U); }
static void introsort_t16(int *arr, size_t n) { introsort(arr, n, 16U, 4U, 2.0); }
static void introsort_t32(int *arr, size_t n) { introsort(arr, n, 32U, 4U, 2.0); }
static void introsort_t64(int *arr, size_t n) { introsort(arr, n, 64U, 4U, 2.0); }
static void introsort_best(int *arr, size_t n) { introsort(arr, n, 32U, 4U, 2.0); }

static void run_group(const dataset_config *cfg, const named_sorting *arr, size_t count) {
    for (size_t i = 0U; i < count; ++i) {
        LOGGER_INFO("run %s", arr[i].name);
        timing_array times = run_sorting_dataset(cfg, arr[i].fn, arr[i].name);
        timing_array_free(&times);
    }
}

static void run_point_1(const dataset_config *cfg) {
    named_sorting list[] = {
        {"insertion_simple", insertion_sort},
        {"bubble_simple", bubble_sort},
        {"selection_simple", selection_sort},
        {"shell_knuth_gap", shell_knuth_sort}
    };
    run_group(cfg, list, sizeof(list) / sizeof(list[0]));
}

static void run_point_2(const dataset_config *cfg) {
    named_sorting list[] = {
        {"heap_k2_bottom_up", heap_k2},
        {"heap_k3_bottom_up", heap_k3},
        {"heap_k4_bottom_up", heap_k4},
        {"heap_k5_bottom_up", heap_k5},
        {"heap_k6_bottom_up", heap_k6},
        {"heap_k7_bottom_up", heap_k7},
        {"heap_k8_bottom_up", heap_k8},
        {"heap_k9_bottom_up", heap_k9},
        {"heap_k10_bottom_up", heap_k10}
    };
    run_group(cfg, list, sizeof(list) / sizeof(list[0]));
}

static void run_point_3(const dataset_config *cfg) {
    named_sorting list[] = {
        {"merge_recursive_top_down", merge_recursive_sort},
        {"merge_iterative_bottom_up", merge_iterative_sort}
    };
    run_group(cfg, list, sizeof(list) / sizeof(list[0]));
}

static void run_point_4(const dataset_config *cfg) {
    named_sorting list[] = {
        {"quick_lomuto_partition", quick_lomuto_sort},
        {"quick_hoare_partition", quick_hoare_sort},
        {"quick_three_way_partition", quick_three_way_sort}
    };
    run_group(cfg, list, sizeof(list) / sizeof(list[0]));
}

static void run_point_5(const dataset_config *cfg) {
    named_sorting list[] = {
        {"quick_3way_pivot_center", quick_pivot_center},
        {"quick_3way_pivot_median3", quick_pivot_median3},
        {"quick_3way_pivot_random", quick_pivot_random},
        {"quick_3way_pivot_median3_random", quick_pivot_m3rand}
    };
    run_group(cfg, list, sizeof(list) / sizeof(list[0]));
}

static void run_point_6(const dataset_config *cfg) {
    named_sorting list[] = {
        {"quick_3way_pivot_median3_baseline", quick_pivot_median3},
        {"introsort_threshold16", introsort_t16},
        {"introsort_threshold32", introsort_t32},
        {"introsort_threshold64", introsort_t64}
    };
    run_group(cfg, list, sizeof(list) / sizeof(list[0]));
}

static void run_point_7(const dataset_config *cfg) {
    named_sorting list[] = {
        {"quick_3way_pivot_median3_baseline", quick_pivot_median3},
        {"introsort_best_config", introsort_best}
    };
    run_group(cfg, list, sizeof(list) / sizeof(list[0]));
}

static void run_point_8(const dataset_config *cfg) {
    named_sorting list[] = {
        {"quick_3way_pivot_median3_baseline", quick_pivot_median3},
        {"introsort_best_config", introsort_best},
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
        {"quick_3way_pivot_median3_best", quick_pivot_median3},
        {"introsort_best_config", introsort_best},
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
    RETURN_VAL_IF_FAIL(out != NULL, 1, "tester: cannot open result csv");
    fprintf(out, "algorithm,size,seconds\n");
    fclose(out);

    if (strcmp(argv[1], "p1") == 0) { run_point_1(&cfg); return 0; }
    if (strcmp(argv[1], "p2") == 0) { run_point_2(&cfg); return 0; }
    if (strcmp(argv[1], "p3") == 0) { run_point_3(&cfg); return 0; }
    if (strcmp(argv[1], "p4") == 0) { run_point_4(&cfg); return 0; }
    if (strcmp(argv[1], "p5") == 0) { run_point_5(&cfg); return 0; }
    if (strcmp(argv[1], "p6") == 0) { run_point_6(&cfg); return 0; }
    if (strcmp(argv[1], "p7") == 0) { run_point_7(&cfg); return 0; }
    if (strcmp(argv[1], "p8") == 0) { run_point_8(&cfg); return 0; }
    if (strcmp(argv[1], "p9") == 0) { run_point_9(&cfg); return 0; }
    if (strcmp(argv[1], "p10") == 0) { run_point_10(&cfg); return 0; }

    LOGGER_ERROR("unknown point '%s'", argv[1]);
    return 2;
}
