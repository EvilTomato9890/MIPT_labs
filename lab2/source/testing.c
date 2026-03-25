#include "testing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "asserts.h"
#include "logger.h"
#include "return_macros.h"

#define MIN_ALLOCATED_LENGTH 1U
#define DATASET_PATH_BUFFER_SIZE 512U
#define CSV_LINE_FORMAT "%s,%zu,%.9f\n"
#define PROGRESS_BAR_WIDTH 28U
#define PERCENT_SCALE 100U

static int *read_array_file(const char *path, size_t *n) {
    HARD_ASSERT(path != NULL && n != NULL, "read_array_file: invalid args");
    *n = 0U;
    FILE *in = fopen(path, "r");
    if (in == NULL) {
        fprintf(stderr, "read_array_file: cannot open '%s'\n", path);
        HARD_ASSERT(0, "read_array_file: fopen failed");
    }

    if (fscanf(in, "%zu", n) != 1) {
        fprintf(stderr, "read_array_file: failed to read array size from '%s'\n", path);
        fclose(in);
        HARD_ASSERT(0, "read_array_file: bad size");
    }
    size_t alloc_n = (*n == 0U) ? MIN_ALLOCATED_LENGTH : *n;
    int *arr = (int *)calloc(alloc_n, sizeof(int));
    if (arr == NULL) {
        fprintf(stderr, "read_array_file: cannot allocate %zu integers for '%s'\n", *n, path);
        fclose(in);
        HARD_ASSERT(0, "read_array_file: no memory");
    }

    for (size_t i = 0U; i < *n; ++i) {
        if (fscanf(in, "%d", &arr[i]) != 1) {
            fprintf(stderr, "read_array_file: failed to read item %zu from '%s'\n", i, path);
            free(arr);
            fclose(in);
            HARD_ASSERT(0, "read_array_file: bad item");
        }
    }
    fclose(in);
    return arr;
}

static void append_csv_line(FILE *out, const char *name, size_t size, double avg) {
    HARD_ASSERT(out != NULL && name != NULL, "append_csv_line: invalid args");
    fprintf(out, CSV_LINE_FORMAT, name, size, avg);
}

static int *duplicate_array(const int *src, size_t n) {
    HARD_ASSERT(src != NULL || n == 0U, "duplicate_array: invalid args");
    size_t alloc_n = (n == 0U) ? MIN_ALLOCATED_LENGTH : n;
    int *copy = (int *)malloc(alloc_n * sizeof(copy[0]));
    HARD_ASSERT(copy != NULL, "duplicate_array: no memory");
    if (n > 0U) {
        memcpy(copy, src, n * sizeof(copy[0]));
    }
    return copy;
}

static void render_progress_bar(const char *name, size_t current, size_t total,
                                size_t size, size_t copy, size_t copies) {
    HARD_ASSERT(name != NULL, "render_progress_bar: invalid args");
    if (total == 0U) {
        return;
    }

    size_t filled = (current * PROGRESS_BAR_WIDTH) / total;
    size_t percent = (current * PERCENT_SCALE) / total;

    fprintf(stderr, "\r[");
    for (size_t i = 0U; i < PROGRESS_BAR_WIDTH; ++i) {
        fputc((i < filled) ? '#' : '-', stderr);
    }
    fprintf(stderr, "] %3zu%%  %-32s size=%zu copy=%zu/%zu",
            percent, name, size, copy, copies);
    fflush(stderr);
}

void run_sorting_group(const dataset_config *cfg, const named_sorting *sorters, size_t count) {
    HARD_ASSERT(cfg != NULL && sorters != NULL, "run_sorting_group: invalid args");
    HARD_ASSERT(cfg->step > 0U, "run_sorting_group: step must be > 0");

    size_t size_count = (cfg->to >= cfg->from) ? ((cfg->to - cfg->from) / cfg->step + 1U) : 0U;
    size_t total_runs = size_count * cfg->copies * count;
    size_t completed = 0U;
    double *sums = (double *)calloc(size_count * count, sizeof(sums[0]));
    RETURN_IF_FAIL(sums != NULL || size_count == 0U || count == 0U, "run_sorting_group: no memory");

    FILE *csv = fopen(cfg->result_csv, "a");
    if (csv == NULL) {
        free(sums);
        RETURN_IF_FAIL(0, "run_sorting_group: cannot open csv");
    }

    for (size_t size_idx = 0U; size_idx < size_count; ++size_idx) {
        size_t size = cfg->from + size_idx * cfg->step;
        for (size_t copy_idx = 0U; copy_idx < cfg->copies; ++copy_idx) {
            char in_path[DATASET_PATH_BUFFER_SIZE] = "";
            char out_path[DATASET_PATH_BUFFER_SIZE] = "";
            snprintf(in_path, sizeof(in_path), "%s/%zu_%zu.in", cfg->tests_dir, size, copy_idx);
            snprintf(out_path, sizeof(out_path), "%s/%zu_%zu.out", cfg->tests_dir, size, copy_idx);

            size_t n = 0U;
            int *source = read_array_file(in_path, &n);
            HARD_ASSERT(source != NULL, "run_sorting_group: cannot read input");

            size_t expected_n = 0U;
            int *gold = read_array_file(out_path, &expected_n);
            HARD_ASSERT(gold != NULL, "run_sorting_group: cannot read expected output");
            HARD_ASSERT(expected_n == n, "run_sorting_group: input/output size mismatch");

            for (size_t sorter_idx = 0U; sorter_idx < count; ++sorter_idx) {
                int *work = duplicate_array(source, n);
                clock_t start = clock();
                sorters[sorter_idx].fn(work, n);
                clock_t end = clock();
                sums[sorter_idx * size_count + size_idx] +=
                    (double)(end - start) / (double)CLOCKS_PER_SEC;

                HARD_ASSERT(memcmp(work, gold, n * sizeof(work[0])) == 0,
                            "run_sorting_group: sorting result mismatch");

                free(work);
                ++completed;
                render_progress_bar(sorters[sorter_idx].name, completed, total_runs, size,
                                    copy_idx + 1U, cfg->copies);
            }

            free(gold);
            free(source);
        }
    }

    for (size_t sorter_idx = 0U; sorter_idx < count; ++sorter_idx) {
        for (size_t size_idx = 0U; size_idx < size_count; ++size_idx) {
            size_t size = cfg->from + size_idx * cfg->step;
            double avg = (cfg->copies == 0U)
                           ? 0.0
                           : (sums[sorter_idx * size_count + size_idx] / (double)cfg->copies);
            append_csv_line(csv, sorters[sorter_idx].name, size, avg);
        }
    }

    if (total_runs > 0U) {
        fputc('\n', stderr);
    }
    fclose(csv);
    free(sums);
}
