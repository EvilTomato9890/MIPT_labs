#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asserts.h"
#include "logger.h"
#include "return_macros.h"
#include "sortings.h"
#include "testing.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define TESTER_ARGC 8
#define RESULT_CSV_HEADER "algorithm,size,seconds\n"
#define DEFINE_HEAP_WRAPPER(name, branching_factor) \
    static void name(int *arr, size_t n) { heap_kary_sort(arr, n, branching_factor); }
#define DEFINE_INTROSORT_THRESHOLD_WRAPPER(name, threshold_value)                           \
    static void name(int *arr, size_t n) {                                                  \
        introsort_config_sort(arr, n, threshold_value, BEST_HEAP_K, BEST_INTROSORT_C,      \
                              PIVOT_MEDIAN3, shell_knuth_sort);                             \
    }
#define DEFINE_INTROSORT_DEPTH_WRAPPER(name, depth_value)                                   \
    static void name(int *arr, size_t n) {                                                  \
        introsort_config_sort(arr, n, BEST_INTROSORT_THRESHOLD, BEST_HEAP_K, depth_value,   \
                              PIVOT_MEDIAN3, shell_knuth_sort);                             \
    }

typedef enum tester_status {
    TESTER_STATUS_OK            = TESTING_STATUS_OK,
    TESTER_STATUS_NULL_ARG      = TESTING_STATUS_NULL_ARG,
    TESTER_STATUS_INVALID_ARG   = TESTING_STATUS_INVALID_ARG,
    TESTER_STATUS_OVERFLOW      = TESTING_STATUS_OVERFLOW,
    TESTER_STATUS_ALLOC_FAIL    = TESTING_STATUS_ALLOC_FAIL,
    TESTER_STATUS_IO_FAIL       = TESTING_STATUS_IO_FAIL,
    TESTER_STATUS_DATA_MISMATCH = TESTING_STATUS_DATA_MISMATCH,
    TESTER_STATUS_UNKNOWN_POINT
} tester_status_t;

typedef testing_status_t (*point_runner_fn)(const dataset_config *cfg);

typedef struct named_point_runner {
    const char *name;
    point_runner_fn fn;
} named_point_runner;

typedef struct quick_range_type {
    ptrdiff_t left;
    ptrdiff_t right;
} quick_range_type;

static const size_t BEST_HEAP_K = 4U;
static const size_t BEST_INTROSORT_THRESHOLD = 32U;
static const double BEST_INTROSORT_C      = 2.0;
static const double INTROSORT_DEPTH_C_125 = 1.25;
static const double INTROSORT_DEPTH_C_150 = 1.50;
static const double INTROSORT_DEPTH_C_200 = 2.00;
static const double INTROSORT_DEPTH_C_250 = 2.50;
static const double INTROSORT_DEPTH_C_300 = 3.00;

static tester_status_t parse_size_arg(const char *text, const char *name, size_t *out) {
    SOFT_ASSERT_FUNCTIONAL(text != NULL && name != NULL && out != NULL,
                           "Parse_size arguments must not be NULL",
                           return TESTER_STATUS_NULL_ARG);

    char *end = NULL;
    errno = 0;
    unsigned long long parsed = strtoull(text, &end, 10);
    RETURN_IF_ERROR(errno != 0 || end == text || *end != '\0',
                    TESTER_STATUS_INVALID_ARG,
                    "tester: invalid %s '%s'", name, text);
    RETURN_IF_ERROR(parsed > SIZE_MAX,
                    TESTER_STATUS_OVERFLOW,
                    "tester: %s '%s' overflows size_t", name, text);
    *out = (size_t)parsed;
    return TESTER_STATUS_OK;
}

static int compare_int(const void *lhs, const void *rhs) {
    int a = *(const int *)lhs;
    int b = *(const int *)rhs;
    return (a > b) - (a < b);
}

static void qsort_stdlib_sort(int *arr, size_t n) {
    SOFT_ASSERT_FUNCTIONAL(arr != NULL || n == 0U, "Array pointer must not be NULL", return);
    qsort(arr, n, sizeof(arr[0]), compare_int);
}

static void quick_pivot_center(int *arr, size_t n) {
    quick_best_sort(arr, n, PIVOT_CENTER);
}

static void quick_pivot_median3(int *arr, size_t n) {
    quick_best_sort(arr, n, PIVOT_MEDIAN3);
}

static void quick_pivot_random(int *arr, size_t n) {
    quick_best_sort(arr, n, PIVOT_RANDOM);
}

static void quick_pivot_m3rand(int *arr, size_t n) {
    quick_best_sort(arr, n, PIVOT_MEDIAN3_RANDOM);
}

static ptrdiff_t median3_index(const int *arr, ptrdiff_t left, ptrdiff_t right) {
    ptrdiff_t center = left + (right - left) / 2;
    int x = arr[left];
    int y = arr[center];
    int z = arr[right];

    if ((x <= y && y <= z) || (z <= y && y <= x)) {
        return center;
    }
    if ((y <= x && x <= z) || (z <= x && x <= y)) {
        return left;
    }
    return right;
}

static void swap_local_int(int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

static quick_range_type partition_three_way_median3(int *arr, ptrdiff_t left, ptrdiff_t right) {
    int pivot = arr[median3_index(arr, left, right)];
    ptrdiff_t lt = left;
    ptrdiff_t i = left;
    ptrdiff_t gt = right;

    while (i <= gt) {
        if (arr[i] < pivot) {
            swap_local_int(&arr[lt++], &arr[i++]);
        } else if (arr[i] > pivot) {
            swap_local_int(&arr[i], &arr[gt--]);
        } else {
            ++i;
        }
    }

    quick_range_type range = {lt, gt};
    return range;
}

static void quick_median3_recursive_impl(int *arr, ptrdiff_t left, ptrdiff_t right) {
    if (left >= right) {
        return;
    }

    quick_range_type range = partition_three_way_median3(arr, left, right);
    quick_median3_recursive_impl(arr, left, range.left - 1);
    quick_median3_recursive_impl(arr, range.right + 1, right);
}

static void quick_median3_recursive(int *arr, size_t n) {
    SOFT_ASSERT_FUNCTIONAL(arr != NULL || n == 0U, "Array pointer must not be NULL", return);
    if (n > 1U) {
        quick_median3_recursive_impl(arr, 0, (ptrdiff_t)n - 1);
    }
}

static void quick_median3_tailrec(int *arr, size_t n) {
    quick_pivot_median3(arr, n);
}

static void quick_median3_iterative(int *arr, size_t n) {
    SOFT_ASSERT_FUNCTIONAL(arr != NULL || n == 0U, "Array pointer must not be NULL", return);
    if (n < 2U) {
        return;
    }

    quick_range_type *stack = calloc(n, sizeof(stack[0]));
    if (stack == NULL) {
        LOGGER_ERROR("quick_median3_iterative: no memory for %zu stack frames", n);
        SOFT_ASSERT_FUNCTIONAL(0, "Quick iterative stack allocation failed", return);
    }

    size_t top = 0U;
    stack[top++] = (quick_range_type){0, (ptrdiff_t)n - 1};
    while (top > 0U) {
        quick_range_type current = stack[--top];
        if (current.left >= current.right) {
            continue;
        }

        quick_range_type range = partition_three_way_median3(arr, current.left, current.right);
        if (range.left - 1 > current.left) {
            stack[top++] = (quick_range_type){current.left, range.left - 1};
        }
        if (range.right + 1 < current.right) {
            stack[top++] = (quick_range_type){range.right + 1, current.right};
        }
    }

    free(stack);
}

DEFINE_HEAP_WRAPPER(heap_k2, 2U)
DEFINE_HEAP_WRAPPER(heap_k3, 3U)
DEFINE_HEAP_WRAPPER(heap_k4, 4U)
DEFINE_HEAP_WRAPPER(heap_k5, 5U)
DEFINE_HEAP_WRAPPER(heap_k6, 6U)
DEFINE_HEAP_WRAPPER(heap_k7, 7U)
DEFINE_HEAP_WRAPPER(heap_k8, 8U)
DEFINE_HEAP_WRAPPER(heap_k9, 9U)
DEFINE_HEAP_WRAPPER(heap_k10, 10U)
DEFINE_INTROSORT_THRESHOLD_WRAPPER(introsort_t16, 16U)
DEFINE_INTROSORT_THRESHOLD_WRAPPER(introsort_t32, 32U)
DEFINE_INTROSORT_THRESHOLD_WRAPPER(introsort_t64, 64U)
DEFINE_INTROSORT_DEPTH_WRAPPER(introsort_c125, INTROSORT_DEPTH_C_125)
DEFINE_INTROSORT_DEPTH_WRAPPER(introsort_c150, INTROSORT_DEPTH_C_150)
DEFINE_INTROSORT_DEPTH_WRAPPER(introsort_c200, INTROSORT_DEPTH_C_200)
DEFINE_INTROSORT_DEPTH_WRAPPER(introsort_c250, INTROSORT_DEPTH_C_250)
DEFINE_INTROSORT_DEPTH_WRAPPER(introsort_c300, INTROSORT_DEPTH_C_300)

static void introsort_best(int *arr, size_t n) {
    introsort_config_sort(arr, n, BEST_INTROSORT_THRESHOLD, BEST_HEAP_K, BEST_INTROSORT_C,
                          PIVOT_MEDIAN3, shell_knuth_sort);
}

static testing_status_t run_group(const dataset_config *cfg, const named_sorting *sorters, size_t count) {
    SOFT_ASSERT_FUNCTIONAL(cfg != NULL && sorters != NULL, "Run_group arguments must not be NULL",
                           return TESTING_STATUS_NULL_ARG);

    for (size_t i = 0U; i < count; ++i) {
        LOGGER_INFO("queue %s", sorters[i].name);
    }
    return run_sorting_group(cfg, sorters, count);
}

static testing_status_t run_point_1(const dataset_config *cfg) {
    named_sorting list[] = {
        {"insertion_simple", insertion_sort},
        {"bubble_simple",    bubble_sort},
        {"selection_simple", selection_sort},
        {"shell_knuth_gap",  shell_knuth_sort}
    };
    return run_group(cfg, list, ARRAY_SIZE(list));
}

static testing_status_t run_point_2(const dataset_config *cfg) {
    named_sorting list[] = {
        {"heap_k2_bottom_up",  heap_k2},
        {"heap_k3_bottom_up",  heap_k3},
        {"heap_k4_bottom_up",  heap_k4},
        {"heap_k5_bottom_up",  heap_k5},
        {"heap_k6_bottom_up",  heap_k6},
        {"heap_k7_bottom_up",  heap_k7},
        {"heap_k8_bottom_up",  heap_k8},
        {"heap_k9_bottom_up",  heap_k9},
        {"heap_k10_bottom_up", heap_k10}
    };
    return run_group(cfg, list, ARRAY_SIZE(list));
}

static testing_status_t run_point_3(const dataset_config *cfg) {
    named_sorting list[] = {
        {"merge_recursive_top_down",  merge_recursive_sort},
        {"merge_iterative_bottom_up", merge_iterative_sort}
    };
    return run_group(cfg, list, ARRAY_SIZE(list));
}

static testing_status_t run_point_4(const dataset_config *cfg) {
    named_sorting list[] = {
        {"quick_lomuto_partition",    quick_lomuto_sort},
        {"quick_hoare_partition",     quick_hoare_sort},
        {"quick_three_way_partition", quick_three_way_sort}
    };
    return run_group(cfg, list, ARRAY_SIZE(list));
}

static testing_status_t run_point_4_opt(const dataset_config *cfg) {
    named_sorting list[] = {
        {"quick_median3_recursive", quick_median3_recursive},
        {"quick_median3_tailrec",   quick_median3_tailrec},
        {"quick_median3_iterative", quick_median3_iterative}
    };
    return run_group(cfg, list, ARRAY_SIZE(list));
}

static testing_status_t run_point_5(const dataset_config *cfg) {
    named_sorting list[] = {
        {"quick_3way_pivot_center",         quick_pivot_center},
        {"quick_3way_pivot_median3",        quick_pivot_median3},
        {"quick_3way_pivot_random",         quick_pivot_random},
        {"quick_3way_pivot_median3_random", quick_pivot_m3rand}
    };
    return run_group(cfg, list, ARRAY_SIZE(list));
}

static testing_status_t run_point_6(const dataset_config *cfg) {
    named_sorting list[] = {
        {"quick_3way_pivot_median3_baseline", quick_pivot_median3},
        {"introsort_threshold16", introsort_t16},
        {"introsort_threshold32", introsort_t32},
        {"introsort_threshold64", introsort_t64}
    };
    return run_group(cfg, list, ARRAY_SIZE(list));
}

static testing_status_t run_point_7(const dataset_config *cfg) {
    named_sorting list[] = {
        {"quick_3way_pivot_median3_baseline", quick_pivot_median3},
        {"introsort_best_config", introsort_best}
    };
    return run_group(cfg, list, ARRAY_SIZE(list));
}

static testing_status_t run_point_7_scan(const dataset_config *cfg) {
    named_sorting list[] = {
        {"introsort_c125", introsort_c125},
        {"introsort_c150", introsort_c150},
        {"introsort_c200", introsort_c200},
        {"introsort_c250", introsort_c250},
        {"introsort_c300", introsort_c300}
    };
    return run_group(cfg, list, ARRAY_SIZE(list));
}

static testing_status_t run_point_8(const dataset_config *cfg) {
    named_sorting list[] = {
        {"quick_3way_pivot_median3_baseline", quick_pivot_median3},
        {"introsort_best_config",             introsort_best},
        {"timsort_hybrid_runs",               timsort_sort},
        {"pdqsort_pattern_defeating",         pdqsort_sort}
    };
    return run_group(cfg, list, ARRAY_SIZE(list));
}

static testing_status_t run_point_9(const dataset_config *cfg) {
    named_sorting list[] = {
        {"radix_lsd_bytewise", lsd_radix_sort},
        {"radix_msd_bytewise", msd_radix_sort}
    };
    return run_group(cfg, list, ARRAY_SIZE(list));
}

static testing_status_t run_point_10(const dataset_config *cfg) {
    named_sorting list[] = {
        {"shell_knuth_best",              shell_knuth_sort},
        {"heap_k4_bottom_up_best",        heap_k4},
        {"merge_iterative_best",          merge_iterative_sort},
        {"quick_3way_pivot_median3_best", quick_pivot_median3},
        {"introsort_best_config",         introsort_best},
        {"pdqsort_pattern_defeating",     pdqsort_sort},
        {"radix_lsd_bytewise",            lsd_radix_sort},
        {"timsort_hybrid_runs",           timsort_sort},
        {"qsort_stdlib_reference",        qsort_stdlib_sort}
    };
    return run_group(cfg, list, ARRAY_SIZE(list));
}

static point_runner_fn resolve_point_runner(const char *name) {
    SOFT_ASSERT_FUNCTIONAL(name != NULL, "Point name must not be NULL", return NULL);

    static const named_point_runner runners[] = {
        {"p1",     run_point_1},
        {"p2",     run_point_2},
        {"p3",     run_point_3},
        {"p4",     run_point_4},
        {"p4opt",  run_point_4_opt},
        {"p5",     run_point_5},
        {"p6",     run_point_6},
        {"p7scan", run_point_7_scan},
        {"p7",     run_point_7},
        {"p8",     run_point_8},
        {"p9",     run_point_9},
        {"p10",    run_point_10}
    };

    for (size_t i = 0U; i < ARRAY_SIZE(runners); ++i) {
        if (strcmp(name, runners[i].name) == 0) {
            return runners[i].fn;
        }
    }
    return NULL;
}

static tester_status_t initialize_result_csv(const char *path) {
    SOFT_ASSERT_FUNCTIONAL(path != NULL, "Result csv path must not be NULL",
                           return TESTER_STATUS_NULL_ARG);

    FILE *out = fopen(path, "w");
    RETURN_IF_ERROR(out == NULL, TESTER_STATUS_IO_FAIL,
                    "tester: cannot open result csv '%s'", path);

    RETURN_IF_ERROR_CLEANUP(fputs(RESULT_CSV_HEADER, out) == EOF,
                            TESTER_STATUS_IO_FAIL,
                            fclose(out),
                            "tester: failed to write csv header to '%s'", path);
    RETURN_IF_ERROR_CLEANUP(fclose(out) != 0,
                            TESTER_STATUS_IO_FAIL,
                            (void)0,
                            "tester: failed to close result csv '%s'", path);
    return TESTER_STATUS_OK;
}

int main(int argc, char **argv) {
    RETURN_IF_ERROR(argc != TESTER_ARGC,
                    TESTER_STATUS_INVALID_ARG,
                    "usage: tester <point> <tests_dir> <csv> <from> <to> <step> <copies>");

    point_runner_fn runner = resolve_point_runner(argv[1]);
    RETURN_IF_ERROR(runner == NULL, TESTER_STATUS_UNKNOWN_POINT,
                    "tester: unknown point '%s'", argv[1]);

    dataset_config cfg = {
        .tests_dir  = argv[2],
        .result_csv = argv[3],
        .from       = 0U,
        .to         = 0U,
        .step       = 0U,
        .copies     = 0U
    };

    tester_status_t status = parse_size_arg(argv[4], "from", &cfg.from);
    if (status != TESTER_STATUS_OK) return (int)status;
    status = parse_size_arg(argv[5], "to",     &cfg.to);
    if (status != TESTER_STATUS_OK) return (int)status;
    status = parse_size_arg(argv[6], "step",   &cfg.step);
    if (status != TESTER_STATUS_OK) return (int)status;
    status = parse_size_arg(argv[7], "copies", &cfg.copies);
    if (status != TESTER_STATUS_OK) return (int)status;

    status = initialize_result_csv(cfg.result_csv);
    if (status != TESTER_STATUS_OK) {
        return (int)status;
    }

    return (int)runner(&cfg);
}
