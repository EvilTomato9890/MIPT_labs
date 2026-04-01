#ifndef LAB3_BINARY_HEAP_INCLUDE_BINARY_HEAP_H_NCLUDED
#define LAB3_BINARY_HEAP_INCLUDE_BINARY_HEAP_H_NCLUDED

#include <stdbool.h>
#include <stddef.h>

#include "heap_common.h"
#include "vector.h"

typedef struct binary_heap {
    vector_t storage;
} binary_heap_t;

void binary_heap_reset(binary_heap_t *heap);

heap_status_t binary_heap_build_linear(binary_heap_t *heap, int *buffer, size_t count);
heap_status_t binary_heap_build_inserts(binary_heap_t *heap, int *buffer, size_t count);
heap_status_t binary_heap_extract_min(binary_heap_t *heap, int *out_value);
bool binary_heap_is_valid(const binary_heap_t *heap);

heap_status_t binary_heap_benchmark_linear(int *work_arr, size_t n, double *build_seconds, int *sorted_out);
heap_status_t binary_heap_benchmark_inserts(int *work_arr, size_t n, double *build_seconds, int *sorted_out);

#endif /* LAB3_BINARY_HEAP_INCLUDE_BINARY_HEAP_H_NCLUDED */
