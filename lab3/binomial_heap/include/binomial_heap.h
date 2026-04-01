#ifndef LAB4_BINOMIAL_HEAP_INCLUDE_BINOMIAL_HEAP_H_INCLUDED
#define LAB4_BINOMIAL_HEAP_INCLUDE_BINOMIAL_HEAP_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>

#include "heap_common.h"
#include "vector.h"

typedef struct binomial_heap_node binomial_heap_node_t;

typedef struct binomial_heap {
    vector_t roots;
    size_t size;
} binomial_heap_t;

heap_status_t binomial_heap_init(binomial_heap_t *heap);
void binomial_heap_destroy(binomial_heap_t *heap);

heap_status_t binomial_heap_insert(binomial_heap_t *heap, int value);
heap_status_t binomial_heap_build_inserts(binomial_heap_t *heap, const int *values, size_t count);
heap_status_t binomial_heap_extract_min(binomial_heap_t *heap, int *out_value);
bool binomial_heap_is_valid(const binomial_heap_t *heap);

heap_status_t binomial_heap_benchmark_inserts(int *work_arr, size_t n, double *build_seconds, int *sorted_out);

#endif /* LAB4_BINOMIAL_HEAP_INCLUDE_BINOMIAL_HEAP_H_INCLUDED */
