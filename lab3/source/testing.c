#include "testing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "asserts.h"
#include "logger.h"
#include "return_macros.h"

static int compare_int(const void *lhs, const void *rhs) {
    int a = *(const int *)lhs;
    int b = *(const int *)rhs;
    return (a > b) - (a < b);
}

static int *read_array_file(const char *path, size_t *n) {
    HARD_ASSERT(path != NULL && n != NULL, "read_array_file: invalid args");
    FILE *in = fopen(path, "r");
    RETURN_VAL_IF_FAIL(in != NULL, NULL, "read_array_file: fopen failed");

    HARD_ASSERT(fscanf(in, "%zu", n) == 1, "read_array_file: bad size");
    size_t alloc_n = (*n == 0U) ? 1U : *n;
    int *arr = (int *)calloc(alloc_n, sizeof(int));
    if (arr == NULL) {
        fclose(in);
        RETURN_VAL_IF_FAIL(0, NULL, "read_array_file: no memory");
    }

    for (size_t i = 0U; i < *n; ++i) {
        HARD_ASSERT(fscanf(in, "%d", &arr[i]) == 1, "read_array_file: bad item");
    }
    fclose(in);
    return arr;
}

static void append_csv_line(FILE *out, const char *name, size_t size, double avg) {
    HARD_ASSERT(out != NULL && name != NULL, "append_csv_line: invalid args");
    fprintf(out, "%s,%zu,%.9f\n", name, size, avg);
}

timing_array run_sorting_dataset(const dataset_config *cfg, sorting_fn sorter, const char *name) {
    HARD_ASSERT(cfg != NULL && sorter != NULL && name != NULL,
                "run_sorting_dataset: invalid args");
    HARD_ASSERT(cfg->step > 0U, "run_sorting_dataset: step must be > 0");

    size_t count = (cfg->to >= cfg->from) ? ((cfg->to - cfg->from) / cfg->step + 1U) : 0U;
    timing_array empty = {NULL, 0U};
    double *values = (double *)calloc(count, sizeof(double));
    RETURN_VAL_IF_FAIL(values != NULL || count == 0U, empty, "run_sorting_dataset: no memory");

    FILE *csv = fopen(cfg->result_csv, "a");
    if (csv == NULL) {
        free(values);
        RETURN_VAL_IF_FAIL(0, empty, "run_sorting_dataset: cannot open csv");
    }

    for (size_t i = 0U; i < count; ++i) {
        size_t size = cfg->from + i * cfg->step;
        double sum = 0.0;
        for (size_t copy = 0U; copy < cfg->copies; ++copy) {
            char in_path[512] = "";
            snprintf(in_path, sizeof(in_path), "%s/%zu_%zu.in", cfg->tests_dir, size, copy);

            size_t n = 0U;
            int *arr = read_array_file(in_path, &n);
            if (!(arr != NULL || n == 0U)) {
                LOGGER_ERROR("run_sorting_dataset: cannot read input");
                continue;
            }

            size_t alloc_n = (n == 0U) ? 1U : n;
            int *gold = (int *)calloc(alloc_n, sizeof(int));
            if (gold == NULL) {
                LOGGER_ERROR("run_sorting_dataset: no memory for gold");
                free(arr);
                continue;
            }
            memcpy(gold, arr, n * sizeof(arr[0]));
            if (n > 1U) {
                qsort(gold, n, sizeof(gold[0]), compare_int);
            }

            clock_t start = clock();
            sorter(arr, n);
            clock_t end = clock();
            sum += (double)(end - start) / (double)CLOCKS_PER_SEC;

            HARD_ASSERT(memcmp(arr, gold, n * sizeof(arr[0])) == 0,
                        "run_sorting_dataset: sorting result mismatch");

            free(gold);
            free(arr);
        }
        values[i] = (cfg->copies == 0U) ? 0.0 : (sum / (double)cfg->copies);
        append_csv_line(csv, name, size, values[i]);
    }

    fclose(csv);
    timing_array result = {values, count};
    return result;
}

void timing_array_free(timing_array *times) {
    HARD_ASSERT(times != NULL, "timing_array_free: invalid args");
    free(times->values);
    times->values = NULL;
    times->count = 0U;
}
