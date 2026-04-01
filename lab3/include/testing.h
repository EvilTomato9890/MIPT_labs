#ifndef LAB4_INCLUDE_TESTING_H_INCLUDED
#define LAB4_INCLUDE_TESTING_H_INCLUDED

#include <stddef.h>

#include "dijkstra.h"
#include "heap_common.h"

typedef enum testing_status {
    TESTING_STATUS_OK = 0,
    TESTING_STATUS_NULL_ARG,
    TESTING_STATUS_INVALID_ARG,
    TESTING_STATUS_OVERFLOW,
    TESTING_STATUS_ALLOC_FAIL,
    TESTING_STATUS_IO_FAIL,
    TESTING_STATUS_DATA_MISMATCH,
    TESTING_STATUS_HEAP_FAIL
} testing_status_t;

typedef struct dataset_config {
    const char *tests_dir;
    const char *result_csv;
    size_t from;
    size_t to;
    size_t step;
    size_t copies;
} dataset_config_t;

testing_status_t run_heap_group(const dataset_config_t *cfg,
                                const named_heap_builder_t *builders,
                                size_t count);

testing_status_t run_dijkstra_group(const graph_benchmark_config_t *cfg,
                                    const named_dijkstra_runner_t *runners,
                                    size_t count);

#endif /* LAB4_INCLUDE_TESTING_H_INCLUDED */
