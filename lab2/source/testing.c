#include "testing.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "asserts.h"
#include "logger.h"
#include "return_macros.h"

#define MIN_ALLOCATED_LENGTH     1U
#define DATASET_PATH_BUFFER_SIZE 512U
#define CSV_LINE_FORMAT          "%s,%zu,%.9f\n"
#define CSV_HEADER               "algorithm,size,seconds\n"
#define PROGRESS_BAR_WIDTH       28U
#define PROGRESS_LABEL_WIDTH     20U
#define PROGRESS_LINE_BUFFER     128U
#define PERCENT_SCALE            100U

static testing_status_t checked_mul_size(size_t lhs, size_t rhs, size_t *out) {
    SOFT_ASSERT_FUNCTIONAL(out != NULL, "Result pointer must not be NULL",
                           return TESTING_STATUS_NULL_ARG);

    RETURN_IF_ERROR(lhs != 0U && rhs > SIZE_MAX / lhs,
                    TESTING_STATUS_OVERFLOW,
                    "testing: multiplication overflow for %zu * %zu", lhs, rhs);
    *out = lhs * rhs;
    return TESTING_STATUS_OK;
}

static testing_status_t build_dataset_path(const char *tests_dir, size_t size, size_t copy_idx,
                                           const char *extension, char *buffer, size_t buffer_size) {
    SOFT_ASSERT_FUNCTIONAL(tests_dir != NULL && extension != NULL && buffer != NULL,
                           "Build_dataset_path arguments must not be NULL",
                           return TESTING_STATUS_NULL_ARG);

    int written = snprintf(buffer, buffer_size, "%s/%zu_%zu.%s", tests_dir, size, copy_idx, extension);
    RETURN_IF_ERROR(written <= 0 || (size_t)written >= buffer_size,
                    TESTING_STATUS_OVERFLOW,
                    "build_dataset_path: dataset path is too long for size=%zu copy=%zu", size, copy_idx);
    return TESTING_STATUS_OK;
}

static int path_exists(const char *path) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return 0;
    }
    fclose(file);
    return 1;
}

static testing_status_t detect_copies_for_size(const char *tests_dir, size_t size, size_t *out_copies) {
    SOFT_ASSERT_FUNCTIONAL(tests_dir != NULL && out_copies != NULL,
                           "Detect_copies_for_size arguments must not be NULL",
                           return TESTING_STATUS_NULL_ARG);

    for (size_t copy_idx = 0U;; ++copy_idx) {
        char in_path[DATASET_PATH_BUFFER_SIZE] = "";
        char out_path[DATASET_PATH_BUFFER_SIZE] = "";

        testing_status_t status = build_dataset_path(tests_dir, size, copy_idx, "in",
                                                     in_path, sizeof(in_path));
        RETURN_IF_ERROR(status != TESTING_STATUS_OK, status,
                        "detect_copies_for_size: failed to build input path");
        status = build_dataset_path(tests_dir, size, copy_idx, "out",
                                    out_path, sizeof(out_path));
        RETURN_IF_ERROR(status != TESTING_STATUS_OK, status,
                        "detect_copies_for_size: failed to build output path");

        int has_in = path_exists(in_path);
        int has_out = path_exists(out_path);
        RETURN_IF_ERROR(has_in != has_out, TESTING_STATUS_IO_FAIL,
                        "detect_copies_for_size: dataset pair is incomplete for size=%zu copy=%zu", size, copy_idx);

        if (!has_in) {
            RETURN_IF_ERROR(copy_idx == 0U, TESTING_STATUS_IO_FAIL,
                            "detect_copies_for_size: no tests found for size=%zu", size);
            *out_copies = copy_idx;
            return TESTING_STATUS_OK;
        }
    }
}

static testing_status_t detect_uniform_copies(const char *tests_dir, size_t from, size_t to,
                                              size_t step, size_t *out_copies) {
    SOFT_ASSERT_FUNCTIONAL(tests_dir != NULL && out_copies != NULL,
                           "Detect_uniform_copies arguments must not be NULL",
                           return TESTING_STATUS_NULL_ARG);

    RETURN_IF_ERROR(step == 0U, TESTING_STATUS_INVALID_ARG,
                    "detect_uniform_copies: step must be greater than zero");
    RETURN_IF_ERROR(to < from, TESTING_STATUS_INVALID_ARG,
                    "detect_uniform_copies: 'to' (%zu) must be >= 'from' (%zu)", to, from);

    size_t expected_copies = 0U;
    for (size_t size = from;; size += step) {
        size_t copies = 0U;
        testing_status_t status = detect_copies_for_size(tests_dir, size, &copies);
        RETURN_IF_ERROR(status != TESTING_STATUS_OK, status,
                        "detect_uniform_copies: failed to inspect tests for size=%zu", size);

        if (expected_copies == 0U) {
            expected_copies = copies;
        } else {
            RETURN_IF_ERROR(copies != expected_copies, TESTING_STATUS_INVALID_ARG,
                            "detect_uniform_copies: inconsistent copies count for size=%zu", size);
        }

        if (size == to) {
            break;
        }
        RETURN_IF_ERROR(size > SIZE_MAX - step, TESTING_STATUS_OVERFLOW,
                        "detect_uniform_copies: size progression overflow");
    }

    *out_copies = expected_copies;
    return TESTING_STATUS_OK;
}

static testing_status_t read_array_file(const char *path, int **out_arr, size_t *out_n) {
    SOFT_ASSERT_FUNCTIONAL(path != NULL && out_arr != NULL && out_n != NULL,
                           "Read_array_file arguments must not be NULL",
                           return TESTING_STATUS_NULL_ARG);

    *out_arr = NULL;
    *out_n = 0U;

    FILE *in = fopen(path, "r");
    RETURN_IF_ERROR(in == NULL, TESTING_STATUS_IO_FAIL,
                    "read_array_file: cannot open '%s'", path);

    int arg_cnt = fscanf(in, "%zu", out_n);
    RETURN_IF_ERROR_CLEANUP(arg_cnt != 1,
                            TESTING_STATUS_IO_FAIL,
                            fclose(in),
                            "read_array_file: failed to read array size from '%s'", path);

    size_t alloc_n = (*out_n == 0U) ? MIN_ALLOCATED_LENGTH : *out_n;
    int *arr = calloc(alloc_n, sizeof(arr[0]));
    RETURN_IF_ERROR_CLEANUP(arr == NULL, TESTING_STATUS_ALLOC_FAIL,
                            fclose(in),
                            "read_array_file: cannot allocate %zu integers for '%s'", *out_n, path);

    for (size_t i = 0U; i < *out_n; ++i) {
        arg_cnt = fscanf(in, "%d", &arr[i]);
        RETURN_IF_ERROR_CLEANUP(arg_cnt != 1,
                                TESTING_STATUS_IO_FAIL,
                                free(arr); fclose(in),
                                "read_array_file: failed to read item %zu from '%s'", i, path);
    }

    RETURN_IF_ERROR_CLEANUP(fclose(in) != 0,
                            TESTING_STATUS_IO_FAIL,
                            free(arr),
                            "read_array_file: failed to close '%s'", path);

    *out_arr = arr;
    return TESTING_STATUS_OK;
}

static testing_status_t append_csv_line(FILE *out, const char *name, size_t size, double avg) {
    SOFT_ASSERT_FUNCTIONAL(out != NULL && name != NULL,
                           "Append_csv_line arguments must not be NULL",
                           return TESTING_STATUS_NULL_ARG);

    RETURN_IF_ERROR(fprintf(out, CSV_LINE_FORMAT, name, size, avg) < 0,
                    TESTING_STATUS_IO_FAIL,
                    "append_csv_line: failed to write csv line for '%s'", name);
    return TESTING_STATUS_OK;
}

static testing_status_t initialize_result_csv(const char *path) {
    SOFT_ASSERT_FUNCTIONAL(path != NULL, "Result csv path must not be NULL",
                           return TESTING_STATUS_NULL_ARG);

    FILE *out = fopen(path, "w");
    RETURN_IF_ERROR(out == NULL, TESTING_STATUS_IO_FAIL,
                    "initialize_result_csv: cannot open '%s'", path);

    RETURN_IF_ERROR_CLEANUP(fputs(CSV_HEADER, out) == EOF,
                            TESTING_STATUS_IO_FAIL,
                            fclose(out),
                            "initialize_result_csv: failed to write header to '%s'", path);
    RETURN_IF_ERROR_CLEANUP(fclose(out) != 0,
                            TESTING_STATUS_IO_FAIL,
                            (void)0,
                            "initialize_result_csv: failed to close '%s'", path);
    return TESTING_STATUS_OK;
}

static testing_status_t duplicate_array(const int *src, size_t n, int **out_copy) {
    SOFT_ASSERT_FUNCTIONAL(out_copy != NULL, "Output array pointer must not be NULL",
                           return TESTING_STATUS_NULL_ARG);
    SOFT_ASSERT_FUNCTIONAL(src != NULL || n == 0U, "Source array pointer must not be NULL",
                           return TESTING_STATUS_NULL_ARG);

    size_t alloc_n = (n == 0U) ? MIN_ALLOCATED_LENGTH : n;
    int *copy = calloc(alloc_n, sizeof(copy[0]));
    RETURN_IF_ERROR(copy == NULL, TESTING_STATUS_ALLOC_FAIL,
                    "duplicate_array: no memory for %zu integers", n);

    if (n > 0U) {
        memcpy(copy, src, n * sizeof(copy[0]));
    }
    *out_copy = copy;
    return TESTING_STATUS_OK;
}

static void render_progress_bar(const char *name, size_t current, size_t total, size_t size) {
    static size_t previous_length = 0U;

    SOFT_ASSERT_FUNCTIONAL(name != NULL, "Progress label pointer must not be NULL", return);
    if (total == 0U) {
        return;
    }

    size_t filled  = (current * PROGRESS_BAR_WIDTH) / total;
    size_t percent = (current * PERCENT_SCALE) / total;
    char line[PROGRESS_LINE_BUFFER] = "";
    size_t offset = 0U;

    offset += (size_t)snprintf(line + offset, sizeof(line) - offset, "[");
    for (size_t i = 0U; i < PROGRESS_BAR_WIDTH; ++i) {
        if (offset + 1U >= sizeof(line)) {
            break;
        }
        line[offset++] = (i < filled) ? '#' : '-';
    }
    line[offset] = '\0';
    offset += (size_t)snprintf(line + offset, sizeof(line) - offset,
                               "] %3zu%%  %-*.*s n=%zu %zu/%zu",
                               percent,
                               (int)PROGRESS_LABEL_WIDTH,
                               (int)PROGRESS_LABEL_WIDTH,
                               name,
                               size,
                               current,
                               total);

    if (offset >= sizeof(line)) {
        offset = sizeof(line) - 1U;
        line[offset] = '\0';
    }

    fprintf(stderr, "\r%s", line);
    if (previous_length > offset) {
        for (size_t i = offset; i < previous_length; ++i) {
            fputc(' ', stderr);
        }
    }
    previous_length = offset;
    fflush(stderr);
}

static testing_status_t benchmark_sorting_group(const dataset_config *cfg, const named_sorting *sorters,
                                                size_t count, double **out_averages, size_t *out_size_count) {
    SOFT_ASSERT_FUNCTIONAL(cfg != NULL && sorters != NULL,
                           "Benchmark_sorting_group arguments must not be NULL",
                           return TESTING_STATUS_NULL_ARG);

    if (out_averages   != NULL) *out_averages   = NULL;
    if (out_size_count != NULL) *out_size_count = 0U;


    RETURN_IF_ERROR(cfg->step == 0U, TESTING_STATUS_INVALID_ARG,
                    "benchmark_sorting_group: step must be greater than zero");
    RETURN_IF_ERROR(cfg->to < cfg->from, TESTING_STATUS_INVALID_ARG,
                    "benchmark_sorting_group: 'to' (%zu) must be >= 'from' (%zu)", cfg->to, cfg->from);
    RETURN_IF_ERROR(count == 0U, TESTING_STATUS_INVALID_ARG,
                    "benchmark_sorting_group: sorters count must be greater than zero");

    for (size_t sorter_idx = 0U; sorter_idx < count; ++sorter_idx) {
        RETURN_IF_ERROR(sorters[sorter_idx].name == NULL || sorters[sorter_idx].fn == NULL,
                        TESTING_STATUS_NULL_ARG,
                        "benchmark_sorting_group: sorter %zu is not fully initialized", sorter_idx);
    }

    size_t size_count = (cfg->to - cfg->from) / cfg->step + 1U;
    size_t averages_count = 0U;
    testing_status_t status = checked_mul_size(size_count, count, &averages_count);
    RETURN_IF_ERROR(status != TESTING_STATUS_OK, status,
                    "benchmark_sorting_group: too many result cells");

    size_t runs_per_size = 0U;
    size_t total_runs = 0U;
    status = checked_mul_size(cfg->copies, count, &runs_per_size);
    RETURN_IF_ERROR(status != TESTING_STATUS_OK, status,
                    "benchmark_sorting_group: too many benchmark runs");
    status = checked_mul_size(size_count, runs_per_size, &total_runs);
    RETURN_IF_ERROR(status != TESTING_STATUS_OK, status,
                    "benchmark_sorting_group: too many benchmark runs");

    double *averages = (averages_count == 0U) ? NULL : calloc(averages_count, sizeof(averages[0]));
    RETURN_IF_ERROR(averages_count != 0U && averages == NULL, TESTING_STATUS_ALLOC_FAIL,
                    "benchmark_sorting_group: no memory for %zu timings", averages_count);

    FILE *csv = fopen(cfg->result_csv, "a");
    RETURN_IF_ERROR_CLEANUP(csv == NULL, TESTING_STATUS_IO_FAIL,
                            free(averages),
                            "benchmark_sorting_group: cannot open csv '%s'", cfg->result_csv);

    size_t completed = 0U;
    testing_status_t result = TESTING_STATUS_OK;
    for (size_t size_idx = 0U; size_idx < size_count && result == TESTING_STATUS_OK; ++size_idx) {
        size_t size = cfg->from + size_idx * cfg->step;
        for (size_t copy_idx = 0U; copy_idx < cfg->copies && result == TESTING_STATUS_OK; ++copy_idx) {
            char in_path [DATASET_PATH_BUFFER_SIZE] = "";
            char out_path[DATASET_PATH_BUFFER_SIZE] = "";

            result = build_dataset_path(cfg->tests_dir, size, copy_idx, "in", in_path, sizeof(in_path));
            if (result != TESTING_STATUS_OK) break;

            result = build_dataset_path(cfg->tests_dir, size, copy_idx, "out", out_path, sizeof(out_path));
            if (result != TESTING_STATUS_OK) break;

            int *source = NULL;
            size_t n = 0U;
            result = read_array_file(in_path, &source, &n);
            if (result != TESTING_STATUS_OK) {
                break;
            }

            int *gold = NULL;
            size_t expected_n = 0U;
            result = read_array_file(out_path, &gold, &expected_n);
            if (result != TESTING_STATUS_OK) {
                free(source);
                break;
            }

            RETURN_IF_ERROR_CLEANUP(expected_n != n,
                                    TESTING_STATUS_DATA_MISMATCH,
                                    free(gold); free(source); fclose(csv); free(averages),
                                    "benchmark_sorting_group: input/output size mismatch for size=%zu copy=%zu",
                                    size, copy_idx);

            for (size_t sorter_idx = 0U; sorter_idx < count && result == TESTING_STATUS_OK; ++sorter_idx) {
                int *work = NULL;
                result = duplicate_array(source, n, &work);
                if (result != TESTING_STATUS_OK) {
                    break;
                }

                clock_t start = clock();
                sorters[sorter_idx].fn(work, n);
                clock_t end = clock();

                if (memcmp(work, gold, n * sizeof(work[0])) != 0) {
                    LOGGER_ERROR("benchmark_sorting_group: sorting result mismatch for '%s' size=%zu copy=%zu",
                                 sorters[sorter_idx].name, size, copy_idx);
                    HARD_ASSERT(0, "Sorting function produced incorrect result");
                }

                averages[sorter_idx * size_count + size_idx] +=
                    (double)(end - start) / (double)CLOCKS_PER_SEC;

                free(work);
                ++completed;
                render_progress_bar(sorters[sorter_idx].name, completed, total_runs, size);
            }

            free(gold);
            free(source);
        }
    }

    if (result == TESTING_STATUS_OK) {
        for (size_t sorter_idx = 0U; sorter_idx < count && result == TESTING_STATUS_OK; ++sorter_idx) {
            for (size_t size_idx = 0U; size_idx < size_count; ++size_idx) {
                size_t size = cfg->from + size_idx * cfg->step;
                double avg = (cfg->copies == 0U)
                               ? 0.0
                               : averages[sorter_idx * size_count + size_idx] / (double)cfg->copies;
                averages[sorter_idx * size_count + size_idx] = avg;
                result = append_csv_line(csv, sorters[sorter_idx].name, size, avg);
                if (result != TESTING_STATUS_OK) {
                    break;
                }
            }
        }
    }

    if (completed > 0U) {
        fputc('\n', stderr);
    }
    if (fclose(csv) != 0 && result == TESTING_STATUS_OK) {
        LOGGER_ERROR("benchmark_sorting_group: failed to close csv '%s'", cfg->result_csv);
        result = TESTING_STATUS_IO_FAIL;
    }

    if (result != TESTING_STATUS_OK) {
        free(averages);
        return result;
    }

    if (out_size_count != NULL) {
        *out_size_count = size_count;
    }
    if (out_averages != NULL) {
        *out_averages = averages;
    } else {
        free(averages);
    }
    return TESTING_STATUS_OK;
}

double *test_sorting(const char *tests_dir, sorting_fn sorter, const char *result_csv,
                     size_t from, size_t to, size_t step) {
    SOFT_ASSERT_FUNCTIONAL(tests_dir != NULL && sorter != NULL && result_csv != NULL,
                           "Test_sorting arguments must not be NULL",
                           return NULL);

    size_t copies = 0U;
    testing_status_t status = detect_uniform_copies(tests_dir, from, to, step, &copies);
    if (status != TESTING_STATUS_OK) {
        LOGGER_ERROR("test_sorting: failed to detect copies count with status=%d", (int)status);
        return NULL;
    }

    status = initialize_result_csv(result_csv);
    if (status != TESTING_STATUS_OK) {
        LOGGER_ERROR("test_sorting: failed to initialize csv '%s' with status=%d",
                     result_csv, (int)status);
        return NULL;
    }

    dataset_config cfg = {
        .tests_dir = tests_dir,
        .result_csv = result_csv,
        .from = from,
        .to = to,
        .step = step,
        .copies = copies
    };
    named_sorting one_sorter = {
        .name = "sorting",
        .fn = sorter
    };

    double *averages = NULL;
    status = benchmark_sorting_group(&cfg, &one_sorter, 1U, &averages, NULL);
    if (status != TESTING_STATUS_OK) {
        LOGGER_ERROR("test_sorting: benchmark failed with status=%d", (int)status);
        free(averages);
        return NULL;
    }
    return averages;
}

testing_status_t run_sorting_group(const dataset_config *cfg, const named_sorting *sorters, size_t count) {
    return benchmark_sorting_group(cfg, sorters, count, NULL, NULL);
}
