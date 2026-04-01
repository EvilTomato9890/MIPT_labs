#include "testing.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "asserts.h"
#include "logger.h"
#include "return_macros.h"

#define CSV_HEADER               "algorithm,size,seconds\n"
#define CSV_LINE_FORMAT          "%s,%zu,%.9f\n"
#define DATASET_PATH_BUFFER_SIZE 512U
#define MIN_ALLOCATED_LENGTH     1U
#define PROGRESS_BAR_WIDTH       28U
#define PROGRESS_LABEL_WIDTH     24U
#define PROGRESS_LINE_BUFFER     160U
#define PERCENT_SCALE            100U

static testing_status_t checked_mul_size(size_t lhs, size_t rhs, size_t *out) {
    SOFT_ASSERT_FUNCTIONAL(out != NULL, "out is NULL", return TESTING_STATUS_NULL_ARG);

    RETURN_IF_ERROR(lhs != 0U && rhs > SIZE_MAX / lhs,
                    TESTING_STATUS_OVERFLOW,
                    "testing: multiplication overflow for %zu * %zu", lhs, rhs);
    *out = lhs * rhs;
    return TESTING_STATUS_OK;
}

static testing_status_t build_dataset_path(const char *tests_dir, size_t size, size_t copy_idx,
                                           const char *extension, char *buffer, size_t buffer_size) {
    SOFT_ASSERT_FUNCTIONAL(tests_dir != NULL && extension != NULL && buffer != NULL,
                           "build_dataset_path arguments must not be NULL",
                           return TESTING_STATUS_NULL_ARG);

    int written = snprintf(buffer, buffer_size, "%s/%zu_%zu.%s", tests_dir, size, copy_idx, extension);
    RETURN_IF_ERROR(written <= 0 || (size_t)written >= buffer_size,
                    TESTING_STATUS_OVERFLOW,
                    "testing: dataset path is too long for size=%zu copy=%zu", size, copy_idx);
    return TESTING_STATUS_OK;
}

static testing_status_t read_array_file(const char *path, int **out_arr, size_t *out_n) {
    SOFT_ASSERT_FUNCTIONAL(path != NULL && out_arr != NULL && out_n != NULL,
                           "read_array_file arguments must not be NULL",
                           return TESTING_STATUS_NULL_ARG);

    *out_arr = NULL;
    *out_n = 0U;

    FILE *in = fopen(path, "r");
    RETURN_IF_ERROR(in == NULL, TESTING_STATUS_IO_FAIL,
                    "testing: cannot open '%s'", path);

    int read_count = fscanf(in, "%zu", out_n);
    RETURN_IF_ERROR_CLEANUP(read_count != 1,
                            TESTING_STATUS_IO_FAIL,
                            fclose(in),
                            "testing: failed to read array size from '%s'", path);

    size_t alloc_n = (*out_n == 0U) ? MIN_ALLOCATED_LENGTH : *out_n;
    int *arr = calloc(alloc_n, sizeof(arr[0]));
    RETURN_IF_ERROR_CLEANUP(arr == NULL,
                            TESTING_STATUS_ALLOC_FAIL,
                            fclose(in),
                            "testing: cannot allocate %zu integers for '%s'", *out_n, path);

    for (size_t index = 0U; index < *out_n; ++index) {
        read_count = fscanf(in, "%d", &arr[index]);
        RETURN_IF_ERROR_CLEANUP(read_count != 1,
                                TESTING_STATUS_IO_FAIL,
                                free(arr); fclose(in),
                                "testing: failed to read item %zu from '%s'", index, path);
    }

    RETURN_IF_ERROR_CLEANUP(fclose(in) != 0,
                            TESTING_STATUS_IO_FAIL,
                            free(arr),
                            "testing: failed to close '%s'", path);

    *out_arr = arr;
    return TESTING_STATUS_OK;
}

static testing_status_t duplicate_array(const int *src, size_t n, int **out_copy) {
    SOFT_ASSERT_FUNCTIONAL(out_copy != NULL, "out_copy is NULL", return TESTING_STATUS_NULL_ARG);
    SOFT_ASSERT_FUNCTIONAL(src != NULL || n == 0U, "src is NULL", return TESTING_STATUS_NULL_ARG);

    size_t alloc_n = (n == 0U) ? MIN_ALLOCATED_LENGTH : n;
    int *copy = calloc(alloc_n, sizeof(copy[0]));
    RETURN_IF_ERROR(copy == NULL, TESTING_STATUS_ALLOC_FAIL,
                    "testing: cannot allocate %zu integers", n);

    if (n > 0U) {
        memcpy(copy, src, n * sizeof(copy[0]));
    }
    *out_copy = copy;
    return TESTING_STATUS_OK;
}

static testing_status_t allocate_output_buffer(size_t n, int **out_buffer) {
    SOFT_ASSERT_FUNCTIONAL(out_buffer != NULL, "out_buffer is NULL", return TESTING_STATUS_NULL_ARG);

    size_t alloc_n = (n == 0U) ? MIN_ALLOCATED_LENGTH : n;
    int *buffer = calloc(alloc_n, sizeof(buffer[0]));
    RETURN_IF_ERROR(buffer == NULL, TESTING_STATUS_ALLOC_FAIL,
                    "testing: cannot allocate %zu output integers", n);

    *out_buffer = buffer;
    return TESTING_STATUS_OK;
}

static testing_status_t initialize_result_csv(const char *path) {
    SOFT_ASSERT_FUNCTIONAL(path != NULL, "path is NULL", return TESTING_STATUS_NULL_ARG);

    FILE *out = fopen(path, "w");
    RETURN_IF_ERROR(out == NULL, TESTING_STATUS_IO_FAIL,
                    "testing: cannot open '%s' for writing", path);

    RETURN_IF_ERROR_CLEANUP(fputs(CSV_HEADER, out) == EOF,
                            TESTING_STATUS_IO_FAIL,
                            fclose(out),
                            "testing: failed to write header into '%s'", path);
    RETURN_IF_ERROR_CLEANUP(fclose(out) != 0,
                            TESTING_STATUS_IO_FAIL,
                            (void)0,
                            "testing: failed to close '%s'", path);
    return TESTING_STATUS_OK;
}

static testing_status_t append_csv_line(FILE *out, const char *name, size_t size, double avg) {
    SOFT_ASSERT_FUNCTIONAL(out != NULL && name != NULL,
                           "append_csv_line arguments must not be NULL",
                           return TESTING_STATUS_NULL_ARG);

    RETURN_IF_ERROR(fprintf(out, CSV_LINE_FORMAT, name, size, avg) < 0,
                    TESTING_STATUS_IO_FAIL,
                    "testing: failed to write csv line for '%s'", name);
    return TESTING_STATUS_OK;
}

static testing_status_t convert_heap_status(heap_status_t status) {
    switch (status) {
        case HEAP_STATUS_OK:
            return TESTING_STATUS_OK;
        case HEAP_STATUS_NULL_ARG:
            return TESTING_STATUS_NULL_ARG;
        case HEAP_STATUS_INVALID_ARG:
        case HEAP_STATUS_EMPTY:
            return TESTING_STATUS_HEAP_FAIL;
        case HEAP_STATUS_ALLOC_FAIL:
            return TESTING_STATUS_ALLOC_FAIL;
        case HEAP_STATUS_INTERNAL:
        default:
            return TESTING_STATUS_HEAP_FAIL;
    }
}

static testing_status_t convert_dijkstra_status(dijkstra_status_t status) {
    switch (status) {
        case DIJKSTRA_STATUS_OK:
            return TESTING_STATUS_OK;
        case DIJKSTRA_STATUS_NULL_ARG:
            return TESTING_STATUS_NULL_ARG;
        case DIJKSTRA_STATUS_INVALID_ARG:
            return TESTING_STATUS_INVALID_ARG;
        case DIJKSTRA_STATUS_ALLOC_FAIL:
            return TESTING_STATUS_ALLOC_FAIL;
        case DIJKSTRA_STATUS_INTERNAL:
        default:
            return TESTING_STATUS_HEAP_FAIL;
    }
}

static double monotonic_seconds(void) {
    struct timespec time_spec = {0};
    clock_gettime(CLOCK_MONOTONIC, &time_spec);
    return (double)time_spec.tv_sec + (double)time_spec.tv_nsec / 1000000000.0;
}

static unsigned mix_instance_seed(unsigned base_seed, size_t size, size_t copy_idx) {
    uint64_t value = ((uint64_t)base_seed << 32U) ^
                     ((uint64_t)size * 0x9E3779B97F4A7C15ULL) ^
                     ((uint64_t)(copy_idx + 1U) * 0xBF58476D1CE4E5B9ULL);
    value ^= value >> 30U;
    value *= 0xBF58476D1CE4E5B9ULL;
    value ^= value >> 27U;
    value *= 0x94D049BB133111EBULL;
    value ^= value >> 31U;
    return (unsigned)(value ^ (value >> 32U));
}

static testing_status_t allocate_distance_buffer(size_t n, uint64_t **out_buffer) {
    SOFT_ASSERT_FUNCTIONAL(out_buffer != NULL, "out_buffer is NULL", return TESTING_STATUS_NULL_ARG);

    size_t alloc_n = (n == 0U) ? MIN_ALLOCATED_LENGTH : n;
    uint64_t *buffer = calloc(alloc_n, sizeof(buffer[0]));
    RETURN_IF_ERROR(buffer == NULL,
                    TESTING_STATUS_ALLOC_FAIL,
                    "testing: cannot allocate %zu distances", n);

    *out_buffer = buffer;
    return TESTING_STATUS_OK;
}

static void render_progress_bar(const char *name, size_t current, size_t total, size_t size) {
    static size_t previous_length = 0U;

    SOFT_ASSERT_FUNCTIONAL(name != NULL, "name is NULL", return);
    if (total == 0U) {
        return;
    }

    size_t filled = (current * PROGRESS_BAR_WIDTH) / total;
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

static testing_status_t benchmark_heap_group(const dataset_config_t *cfg,
                                             const named_heap_builder_t *builders,
                                             size_t count) {
    SOFT_ASSERT_FUNCTIONAL(cfg != NULL && builders != NULL,
                           "benchmark_heap_group arguments must not be NULL",
                           return TESTING_STATUS_NULL_ARG);

    RETURN_IF_ERROR(cfg->tests_dir == NULL || cfg->result_csv == NULL,
                    TESTING_STATUS_NULL_ARG,
                    "testing: tests_dir and result_csv must not be NULL");
    RETURN_IF_ERROR(cfg->step == 0U,
                    TESTING_STATUS_INVALID_ARG,
                    "testing: step must be greater than zero");
    RETURN_IF_ERROR(cfg->to < cfg->from,
                    TESTING_STATUS_INVALID_ARG,
                    "testing: to (%zu) must be >= from (%zu)", cfg->to, cfg->from);
    RETURN_IF_ERROR(cfg->copies == 0U,
                    TESTING_STATUS_INVALID_ARG,
                    "testing: copies must be greater than zero");
    RETURN_IF_ERROR(count == 0U,
                    TESTING_STATUS_INVALID_ARG,
                    "testing: at least one builder is required");

    for (size_t builder_idx = 0U; builder_idx < count; ++builder_idx) {
        RETURN_IF_ERROR(builders[builder_idx].name == NULL || builders[builder_idx].fn == NULL,
                        TESTING_STATUS_NULL_ARG,
                        "testing: builder %zu is not fully initialized", builder_idx);
    }

    testing_status_t status = initialize_result_csv(cfg->result_csv);
    RETURN_IF_ERROR(status != TESTING_STATUS_OK, status,
                    "testing: failed to initialize csv '%s'", cfg->result_csv);

    size_t size_count = (cfg->to - cfg->from) / cfg->step + 1U;
    size_t averages_count = 0U;
    status = checked_mul_size(size_count, count, &averages_count);
    RETURN_IF_ERROR(status != TESTING_STATUS_OK, status,
                    "testing: too many result cells");

    size_t runs_per_size = 0U;
    size_t total_runs = 0U;
    status = checked_mul_size(cfg->copies, count, &runs_per_size);
    RETURN_IF_ERROR(status != TESTING_STATUS_OK, status,
                    "testing: too many runs per size");
    status = checked_mul_size(size_count, runs_per_size, &total_runs);
    RETURN_IF_ERROR(status != TESTING_STATUS_OK, status,
                    "testing: too many benchmark runs");

    double *averages = calloc((averages_count == 0U) ? 1U : averages_count, sizeof(averages[0]));
    RETURN_IF_ERROR(averages == NULL,
                    TESTING_STATUS_ALLOC_FAIL,
                    "testing: cannot allocate %zu timing slots", averages_count);

    FILE *csv = fopen(cfg->result_csv, "a");
    RETURN_IF_ERROR_CLEANUP(csv == NULL,
                            TESTING_STATUS_IO_FAIL,
                            free(averages),
                            "testing: cannot reopen csv '%s'", cfg->result_csv);

    size_t completed = 0U;
    testing_status_t result = TESTING_STATUS_OK;

    for (size_t size_idx = 0U; size_idx < size_count && result == TESTING_STATUS_OK; ++size_idx) {
        size_t size = cfg->from + size_idx * cfg->step;

        for (size_t copy_idx = 0U; copy_idx < cfg->copies && result == TESTING_STATUS_OK; ++copy_idx) {
            char in_path[DATASET_PATH_BUFFER_SIZE] = "";
            char out_path[DATASET_PATH_BUFFER_SIZE] = "";

            result = build_dataset_path(cfg->tests_dir, size, copy_idx, "in", in_path, sizeof(in_path));
            if (result != TESTING_STATUS_OK) {
                break;
            }

            result = build_dataset_path(cfg->tests_dir, size, copy_idx, "out", out_path, sizeof(out_path));
            if (result != TESTING_STATUS_OK) {
                break;
            }

            int *source = NULL;
            size_t n = 0U;
            result = read_array_file(in_path, &source, &n);
            if (result != TESTING_STATUS_OK) {
                break;
            }

            int *gold = NULL;
            size_t gold_n = 0U;
            result = read_array_file(out_path, &gold, &gold_n);
            if (result != TESTING_STATUS_OK) {
                free(source);
                break;
            }

            if (gold_n != n) {
                LOGGER_ERROR("testing: input/output size mismatch for size=%zu copy=%zu", size, copy_idx);
                free(gold);
                free(source);
                result = TESTING_STATUS_DATA_MISMATCH;
                break;
            }

            for (size_t builder_idx = 0U; builder_idx < count && result == TESTING_STATUS_OK; ++builder_idx) {
                int *work = NULL;
                result = duplicate_array(source, n, &work);
                if (result != TESTING_STATUS_OK) {
                    break;
                }

                int *output = NULL;
                result = allocate_output_buffer(n, &output);
                if (result != TESTING_STATUS_OK) {
                    free(work);
                    break;
                }

                double seconds = 0.0;
                heap_status_t heap_result = builders[builder_idx].fn(work, n, &seconds, output);
                if (heap_result != HEAP_STATUS_OK) {
                    LOGGER_ERROR("testing: builder '%s' failed for size=%zu copy=%zu with status=%d",
                                 builders[builder_idx].name, size, copy_idx, (int)heap_result);
                    free(output);
                    free(work);
                    result = convert_heap_status(heap_result);
                    break;
                }

                if (memcmp(output, gold, n * sizeof(output[0])) != 0) {
                    LOGGER_ERROR("testing: builder '%s' produced invalid order for size=%zu copy=%zu",
                                 builders[builder_idx].name, size, copy_idx);
                    free(output);
                    free(work);
                    result = TESTING_STATUS_DATA_MISMATCH;
                    break;
                }

                averages[builder_idx * size_count + size_idx] += seconds;

                free(output);
                free(work);

                ++completed;
                render_progress_bar(builders[builder_idx].name, completed, total_runs, size);
            }

            free(gold);
            free(source);
        }
    }

    if (completed > 0U) {
        fputc('\n', stderr);
    }

    if (result == TESTING_STATUS_OK) {
        for (size_t builder_idx = 0U; builder_idx < count && result == TESTING_STATUS_OK; ++builder_idx) {
            for (size_t size_idx = 0U; size_idx < size_count; ++size_idx) {
                size_t size = cfg->from + size_idx * cfg->step;
                double avg = averages[builder_idx * size_count + size_idx] / (double)cfg->copies;
                result = append_csv_line(csv, builders[builder_idx].name, size, avg);
                if (result != TESTING_STATUS_OK) {
                    break;
                }
            }
        }
    }

    if (fclose(csv) != 0 && result == TESTING_STATUS_OK) {
        LOGGER_ERROR("testing: failed to close csv '%s'", cfg->result_csv);
        result = TESTING_STATUS_IO_FAIL;
    }

    free(averages);
    return result;
}

testing_status_t run_heap_group(const dataset_config_t *cfg,
                                const named_heap_builder_t *builders,
                                size_t count) {
    return benchmark_heap_group(cfg, builders, count);
}

static testing_status_t benchmark_dijkstra_group(const graph_benchmark_config_t *cfg,
                                                 const named_dijkstra_runner_t *runners,
                                                 size_t count) {
    SOFT_ASSERT_FUNCTIONAL(cfg != NULL && runners != NULL,
                           "benchmark_dijkstra_group arguments must not be NULL",
                           return TESTING_STATUS_NULL_ARG);

    RETURN_IF_ERROR(cfg->result_csv == NULL,
                    TESTING_STATUS_NULL_ARG,
                    "testing: result_csv must not be NULL");
    RETURN_IF_ERROR(cfg->step == 0U,
                    TESTING_STATUS_INVALID_ARG,
                    "testing: step must be greater than zero");
    RETURN_IF_ERROR(cfg->to < cfg->from,
                    TESTING_STATUS_INVALID_ARG,
                    "testing: to (%zu) must be >= from (%zu)", cfg->to, cfg->from);
    RETURN_IF_ERROR(cfg->copies == 0U,
                    TESTING_STATUS_INVALID_ARG,
                    "testing: copies must be greater than zero");
    RETURN_IF_ERROR(count == 0U,
                    TESTING_STATUS_INVALID_ARG,
                    "testing: at least one dijkstra runner is required");

    for (size_t runner_idx = 0U; runner_idx < count; ++runner_idx) {
        RETURN_IF_ERROR(runners[runner_idx].name == NULL || runners[runner_idx].fn == NULL,
                        TESTING_STATUS_NULL_ARG,
                        "testing: dijkstra runner %zu is not fully initialized", runner_idx);
    }

    testing_status_t status = initialize_result_csv(cfg->result_csv);
    RETURN_IF_ERROR(status != TESTING_STATUS_OK, status,
                    "testing: failed to initialize csv '%s'", cfg->result_csv);

    size_t size_count = (cfg->to - cfg->from) / cfg->step + 1U;
    size_t averages_count = 0U;
    status = checked_mul_size(size_count, count, &averages_count);
    RETURN_IF_ERROR(status != TESTING_STATUS_OK, status,
                    "testing: too many result cells");

    size_t runs_per_size = 0U;
    size_t total_runs = 0U;
    status = checked_mul_size(cfg->copies, count, &runs_per_size);
    RETURN_IF_ERROR(status != TESTING_STATUS_OK, status,
                    "testing: too many runs per size");
    status = checked_mul_size(size_count, runs_per_size, &total_runs);
    RETURN_IF_ERROR(status != TESTING_STATUS_OK, status,
                    "testing: too many benchmark runs");

    double *averages = calloc((averages_count == 0U) ? 1U : averages_count, sizeof(averages[0]));
    RETURN_IF_ERROR(averages == NULL,
                    TESTING_STATUS_ALLOC_FAIL,
                    "testing: cannot allocate %zu timing slots", averages_count);

    FILE *csv = fopen(cfg->result_csv, "a");
    RETURN_IF_ERROR_CLEANUP(csv == NULL,
                            TESTING_STATUS_IO_FAIL,
                            free(averages),
                            "testing: cannot reopen csv '%s'", cfg->result_csv);

    size_t completed = 0U;
    testing_status_t result = TESTING_STATUS_OK;

    for (size_t size_idx = 0U; size_idx < size_count && result == TESTING_STATUS_OK; ++size_idx) {
        size_t size = cfg->from + size_idx * cfg->step;

        for (size_t copy_idx = 0U; copy_idx < cfg->copies && result == TESTING_STATUS_OK; ++copy_idx) {
            graph_t *graph = NULL;
            dijkstra_status_t dijkstra_status =
                graph_create_random(size, cfg->kind, mix_instance_seed(cfg->seed, size, copy_idx), &graph);
            result = convert_dijkstra_status(dijkstra_status);
            if (result != TESTING_STATUS_OK) {
                LOGGER_ERROR("testing: failed to create graph for size=%zu copy=%zu with status=%d",
                             size, copy_idx, (int)dijkstra_status);
                break;
            }

            uint64_t *reference = NULL;
            result = allocate_distance_buffer(size, &reference);
            if (result != TESTING_STATUS_OK) {
                graph_destroy(graph);
                break;
            }

            uint64_t *buffer = NULL;
            result = allocate_distance_buffer(size, &buffer);
            if (result != TESTING_STATUS_OK) {
                free(reference);
                graph_destroy(graph);
                break;
            }

            for (size_t runner_idx = 0U; runner_idx < count && result == TESTING_STATUS_OK; ++runner_idx) {
                uint64_t *target = (runner_idx == 0U) ? reference : buffer;

                double start = monotonic_seconds();
                dijkstra_status = runners[runner_idx].fn(graph, 0U, target);
                double end = monotonic_seconds();

                if (dijkstra_status != DIJKSTRA_STATUS_OK) {
                    LOGGER_ERROR("testing: runner '%s' failed for size=%zu copy=%zu with status=%d",
                                 runners[runner_idx].name, size, copy_idx, (int)dijkstra_status);
                    result = convert_dijkstra_status(dijkstra_status);
                    break;
                }

                if (runner_idx > 0U && memcmp(target, reference, size * sizeof(reference[0])) != 0) {
                    LOGGER_ERROR("testing: runner '%s' produced invalid distances for size=%zu copy=%zu",
                                 runners[runner_idx].name, size, copy_idx);
                    result = TESTING_STATUS_DATA_MISMATCH;
                    break;
                }

                averages[runner_idx * size_count + size_idx] += end - start;
                ++completed;
                render_progress_bar(runners[runner_idx].name, completed, total_runs, size);
            }

            free(buffer);
            free(reference);
            graph_destroy(graph);
        }
    }

    if (completed > 0U) {
        fputc('\n', stderr);
    }

    if (result == TESTING_STATUS_OK) {
        for (size_t runner_idx = 0U; runner_idx < count && result == TESTING_STATUS_OK; ++runner_idx) {
            for (size_t size_idx = 0U; size_idx < size_count; ++size_idx) {
                size_t size = cfg->from + size_idx * cfg->step;
                double avg = averages[runner_idx * size_count + size_idx] / (double)cfg->copies;
                result = append_csv_line(csv, runners[runner_idx].name, size, avg);
                if (result != TESTING_STATUS_OK) {
                    break;
                }
            }
        }
    }

    if (fclose(csv) != 0 && result == TESTING_STATUS_OK) {
        LOGGER_ERROR("testing: failed to close csv '%s'", cfg->result_csv);
        result = TESTING_STATUS_IO_FAIL;
    }

    free(averages);
    return result;
}

testing_status_t run_dijkstra_group(const graph_benchmark_config_t *cfg,
                                    const named_dijkstra_runner_t *runners,
                                    size_t count) {
    return benchmark_dijkstra_group(cfg, runners, count);
}
