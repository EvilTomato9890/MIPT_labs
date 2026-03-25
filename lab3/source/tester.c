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

static void heap_k2(int *arr, size_t n) { heap_kary_sort(arr, n, 2U); }
static void heap_k3(int *arr, size_t n) { heap_kary_sort(arr, n, 3U); }
static void heap_k4(int *arr, size_t n) { heap_kary_sort(arr, n, 4U); }
static void heap_k5(int *arr, size_t n) { heap_kary_sort(arr, n, 5U); }
static void heap_k6(int *arr, size_t n) { heap_kary_sort(arr, n, 6U); }
static void heap_k7(int *arr, size_t n) { heap_kary_sort(arr, n, 7U); }
static void heap_k8(int *arr, size_t n) { heap_kary_sort(arr, n, 8U); }
static void heap_k9(int *arr, size_t n) { heap_kary_sort(arr, n, 9U); }
static void heap_k10(int *arr, size_t n) { heap_kary_sort(arr, n, 10U); }

static void run_group(const dataset_config *cfg, const named_sorting *arr, size_t count) {
    for (size_t i = 0U; i < count; ++i) {
        LOGGER_INFO("run %s", arr[i].name);
        timing_array times = run_sorting_dataset(cfg, arr[i].fn, arr[i].name);
        timing_array_free(&times);
    }
}

static void run_point_1(const dataset_config *cfg) {
    named_sorting list[] = {
        {"insertion", insertion_sort},
        {"bubble", bubble_sort},
        {"selection", selection_sort},
        {"shell_knuth", shell_knuth_sort}
    };
    run_group(cfg, list, sizeof(list) / sizeof(list[0]));
}

static void run_point_2(const dataset_config *cfg) {
    named_sorting list[] = {
        {"heap_k2", heap_k2},
        {"heap_k3", heap_k3},
        {"heap_k4", heap_k4},
        {"heap_k5", heap_k5},
        {"heap_k6", heap_k6},
        {"heap_k7", heap_k7},
        {"heap_k8", heap_k8},
        {"heap_k9", heap_k9},
        {"heap_k10", heap_k10}
    };
    run_group(cfg, list, sizeof(list) / sizeof(list[0]));
}

static void run_point_3(const dataset_config *cfg) {
    named_sorting list[] = {
        {"merge_recursive", merge_recursive_sort},
        {"merge_iterative", merge_iterative_sort}
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

    if (strcmp(argv[1], "p1") == 0) {
        run_point_1(&cfg);
        return 0;
    }
    if (strcmp(argv[1], "p2") == 0) {
        run_point_2(&cfg);
        return 0;
    }
    if (strcmp(argv[1], "p3") == 0) {
        run_point_3(&cfg);
        return 0;
    }

    LOGGER_ERROR("unknown point '%s'", argv[1]);
    return 2;
}
