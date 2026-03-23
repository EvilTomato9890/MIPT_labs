#ifndef LAB1_BENCHMARK_INCLUDE_BENCHMARK_H_NCLUDED
#define LAB1_BENCHMARK_INCLUDE_BENCHMARK_H_NCLUDED

#include <stddef.h>
#include <stdint.h>

enum benchmark_impl_type {
    BENCHMARK_IMPL_ARRAY = 1,
    BENCHMARK_IMPL_LIST = 2
};

typedef enum benchmark_status {
    BENCHMARK_STATUS_OK = 0,
    BENCHMARK_STATUS_NULL_ARG,
    BENCHMARK_STATUS_INVALID_ARG,
    BENCHMARK_STATUS_ALLOC_FAIL,
    BENCHMARK_STATUS_BAD_STATE,
    BENCHMARK_STATUS_INTERNAL_ERROR
} benchmark_status_t;

struct benchmark_config {
    enum benchmark_impl_type implementation;
    int test_id;
    size_t test4_push_count;
    const uint8_t *test3_operations;
    size_t test3_operation_count;
};

benchmark_status_t benchmark_parse_impl(const char *text, enum benchmark_impl_type *implementation);
benchmark_status_t benchmark_run(const struct benchmark_config *config, double *elapsed_seconds);

#endif /* LAB1_BENCHMARK_INCLUDE_BENCHMARK_H_NCLUDED */
