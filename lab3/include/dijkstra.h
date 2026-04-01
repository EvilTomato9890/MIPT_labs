#ifndef LAB4_INCLUDE_DIJKSTRA_H_INCLUDED
#define LAB4_INCLUDE_DIJKSTRA_H_INCLUDED

#include <stddef.h>
#include <stdint.h>

typedef enum graph_kind {
    GRAPH_KIND_SPARSE = 0,
    GRAPH_KIND_DENSE = 1
} graph_kind_t;

typedef enum dijkstra_status {
    DIJKSTRA_STATUS_OK = 0,
    DIJKSTRA_STATUS_NULL_ARG,
    DIJKSTRA_STATUS_INVALID_ARG,
    DIJKSTRA_STATUS_ALLOC_FAIL,
    DIJKSTRA_STATUS_INTERNAL
} dijkstra_status_t;

typedef struct graph graph_t;

typedef dijkstra_status_t (*dijkstra_runner_fn)(const graph_t *graph, size_t source, uint64_t *distances_out);

typedef struct named_dijkstra_runner {
    const char *name;
    dijkstra_runner_fn fn;
} named_dijkstra_runner_t;

typedef struct graph_benchmark_config {
    const char *result_csv;
    size_t from;
    size_t to;
    size_t step;
    size_t copies;
    unsigned seed;
    graph_kind_t kind;
} graph_benchmark_config_t;

dijkstra_status_t graph_create_random(size_t vertex_count, graph_kind_t kind, unsigned seed, graph_t **out_graph);
void graph_destroy(graph_t *graph);
size_t graph_vertex_count(const graph_t *graph);

dijkstra_status_t dijkstra_run_naive(const graph_t *graph, size_t source, uint64_t *distances_out);
dijkstra_status_t dijkstra_run_binary_heap(const graph_t *graph, size_t source, uint64_t *distances_out);
dijkstra_status_t dijkstra_run_binomial_heap(const graph_t *graph, size_t source, uint64_t *distances_out);
dijkstra_status_t dijkstra_run_fibonacci_heap(const graph_t *graph, size_t source, uint64_t *distances_out);

#endif /* LAB4_INCLUDE_DIJKSTRA_H_INCLUDED */
