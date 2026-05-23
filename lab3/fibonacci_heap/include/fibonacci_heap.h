#ifndef LAB4_INCLUDE_FIBONACCI_HEAP_H_INCLUDED
#define LAB4_INCLUDE_FIBONACCI_HEAP_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dijkstra.h"

typedef struct fibonacci_heap_node fibonacci_heap_node_t;

typedef struct fibonacci_heap {
    fibonacci_heap_node_t *min;
    fibonacci_heap_node_t **handles;
    size_t size;
    size_t vertex_count;
} fibonacci_heap_t;

dijkstra_status_t fibonacci_heap_init(fibonacci_heap_t *heap, size_t vertex_count);
void fibonacci_heap_destroy(fibonacci_heap_t *heap);

bool fibonacci_heap_is_empty(const fibonacci_heap_t *heap);
bool fibonacci_heap_contains(const fibonacci_heap_t *heap, size_t vertex);

dijkstra_status_t fibonacci_heap_insert(fibonacci_heap_t *heap, size_t vertex, uint64_t key);
dijkstra_status_t fibonacci_heap_decrease_key(fibonacci_heap_t *heap, size_t vertex, uint64_t new_key);
dijkstra_status_t fibonacci_heap_extract_min(fibonacci_heap_t *heap, size_t *out_vertex, uint64_t *out_key);

#endif /* LAB4_INCLUDE_FIBONACCI_HEAP_H_INCLUDED */
