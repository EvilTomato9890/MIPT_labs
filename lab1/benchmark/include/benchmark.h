/**
 * @file benchmark.h
 * @brief Benchmark runner API for comparing stack implementations.
 */

#ifndef LAB1_BENCHMARK_INCLUDE_BENCHMARK_H_NCLUDED
#define LAB1_BENCHMARK_INCLUDE_BENCHMARK_H_NCLUDED

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Stack implementation selected for a benchmark run.
 */
enum benchmark_impl_type {
    /** Stack backed by a dynamic array. */
    BENCHMARK_IMPL_ARRAY = 1,
    /** Stack backed by a singly linked list. */
    BENCHMARK_IMPL_LIST = 2
};

/**
 * @brief Status codes returned by benchmark operations.
 */
typedef enum benchmark_status {
    /** Operation completed successfully. */
    BENCHMARK_STATUS_OK = 0,
    /** A required pointer argument was NULL. */
    BENCHMARK_STATUS_NULL_ARG,
    /** An argument value is invalid. */
    BENCHMARK_STATUS_INVALID_ARG,
    /** Memory allocation failed. */
    BENCHMARK_STATUS_ALLOC_FAIL,
    /** Benchmark object or dependency is in a bad state. */
    BENCHMARK_STATUS_BAD_STATE,
    /** Unexpected error from an internal dependency. */
    BENCHMARK_STATUS_INTERNAL_ERROR
} benchmark_status_t;

/**
 * @brief Configuration for one benchmark run.
 */
struct benchmark_config {
    /** Stack implementation to benchmark. */
    enum benchmark_impl_type implementation;
    /** Test identifier from 1 to 4. */
    int test_id;
    /** Number of pushes for test 4. */
    size_t test4_push_count;
    /** Operation sequence for test 3: 1 means push, 2 means pop. */
    const uint8_t *test3_operations;
    /** Number of operations in the test 3 sequence. */
    size_t test3_operation_count;
};

/**
 * @brief Parses a textual implementation name.
 *
 * @param text Implementation name: "array" or "list".
 * @param implementation Destination for the parsed implementation.
 * @return Operation status.
 */
benchmark_status_t benchmark_parse_impl(const char *text, enum benchmark_impl_type *implementation);

/**
 * @brief Runs one configured benchmark and returns elapsed wall-clock time.
 *
 * @param config Benchmark configuration.
 * @param elapsed_seconds Destination for elapsed time in seconds.
 * @return Operation status.
 */
benchmark_status_t benchmark_run(const struct benchmark_config *config, double *elapsed_seconds);

#endif /* LAB1_BENCHMARK_INCLUDE_BENCHMARK_H_NCLUDED */
