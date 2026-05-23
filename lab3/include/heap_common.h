#ifndef LAB4_INCLUDE_HEAP_COMMON_H_INCLUDED
#define LAB4_INCLUDE_HEAP_COMMON_H_INCLUDED

#include <stddef.h>

typedef enum heap_status {
    HEAP_STATUS_OK = 0,
    HEAP_STATUS_NULL_ARG,
    HEAP_STATUS_INVALID_ARG,
    HEAP_STATUS_ALLOC_FAIL,
    HEAP_STATUS_EMPTY,
    HEAP_STATUS_INTERNAL
} heap_status_t;

typedef heap_status_t (*heap_benchmark_fn)(int *work_arr, size_t n, double *build_seconds, int *sorted_out);

typedef struct named_heap_builder {
    const char *name;
    heap_benchmark_fn fn;
} named_heap_builder_t;

#endif /* LAB4_INCLUDE_HEAP_COMMON_H_INCLUDED */
