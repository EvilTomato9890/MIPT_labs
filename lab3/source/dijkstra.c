#include "dijkstra.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "asserts.h"
#include "fibonacci_heap.h"
#include "return_macros.h"
#include "vector.h"

#define DIJKSTRA_INFINITY         UINT64_MAX
#define EDGE_WEIGHT_LIMIT         1000U
#define SPARSE_RANDOM_OUT_DEGREE  7U
#define BINARY_QUEUE_ABSENT       SIZE_MAX

typedef struct graph_edge {
    size_t to;
    uint32_t weight;
} graph_edge_t;

struct graph {
    size_t vertex_count;
    graph_kind_t kind;
    unsigned seed;
    vector_t dense_weights;
    vector_t sparse_offsets;
    vector_t sparse_edges;
};

typedef struct vertex_distance {
    size_t vertex;
    uint64_t distance;
} vertex_distance_t;

typedef struct binary_queue {
    vector_t items;
    size_t *positions;
    size_t vertex_count;
} binary_queue_t;

typedef struct binomial_queue_node binomial_queue_node_t;

typedef struct binomial_queue {
    vector_t roots;
    binomial_queue_node_t **handles;
    size_t size;
    size_t vertex_count;
} binomial_queue_t;

struct binomial_queue_node {
    size_t vertex;
    uint64_t key;
    size_t degree;
    binomial_queue_node_t *parent;
    binomial_queue_node_t *child;
    binomial_queue_node_t *sibling;
};

typedef struct priority_queue_ops {
    dijkstra_status_t (*insert)(void *queue, size_t vertex, uint64_t key);
    dijkstra_status_t (*decrease_key)(void *queue, size_t vertex, uint64_t new_key);
    dijkstra_status_t (*extract_min)(void *queue, size_t *out_vertex, uint64_t *out_key);
    bool (*contains)(const void *queue, size_t vertex);
    bool (*is_empty)(const void *queue);
} priority_queue_ops_t;

static dijkstra_status_t checked_mul_size(size_t lhs, size_t rhs, size_t *out) {
    HARD_ASSERT(out != NULL, "out is NULL");

    RETURN_IF_ERROR(lhs != 0U && rhs > SIZE_MAX / lhs,
                    DIJKSTRA_STATUS_INVALID_ARG,
                    "dijkstra: multiplication overflow for %zu * %zu", lhs, rhs);

    *out = lhs * rhs;
    return DIJKSTRA_STATUS_OK;
}

static dijkstra_status_t translate_vector_status(vector_error_t status) {
    switch (status) {
        case VEC_ERR_OK:
            return DIJKSTRA_STATUS_OK;
        case VEC_ERR_MEM_ALLOC:
            return DIJKSTRA_STATUS_ALLOC_FAIL;
        case VEC_ERR_BAD_ARG:
        case VEC_ERR_FULL:
        case VEC_ERR_NOT_FOUND:
            return DIJKSTRA_STATUS_INVALID_ARG;
        case VEC_ERR_INTERNAL:
        default:
            return DIJKSTRA_STATUS_INTERNAL;
    }
}

static uint64_t splitmix64(uint64_t value) {
    value += 0x9E3779B97F4A7C15ULL;
    value = (value ^ (value >> 30U)) * 0xBF58476D1CE4E5B9ULL;
    value = (value ^ (value >> 27U)) * 0x94D049BB133111EBULL;
    return value ^ (value >> 31U);
}

static uint32_t edge_weight(unsigned seed, size_t from, size_t to, uint64_t salt) {
    uint64_t mixed = ((uint64_t)seed << 32U) ^
                     ((uint64_t)from * 0x9E3779B97F4A7C15ULL) ^
                     ((uint64_t)to * 0xBF58476D1CE4E5B9ULL) ^
                     salt;
    return (uint32_t)(splitmix64(mixed) % EDGE_WEIGHT_LIMIT) + 1U;
}

static void graph_release(graph_t *graph) {
    HARD_ASSERT(graph != NULL, "graph is NULL");

    (void)vector_destroy(&graph->dense_weights);
    (void)vector_destroy(&graph->sparse_offsets);
    (void)vector_destroy(&graph->sparse_edges);
}

static dijkstra_status_t graph_init_dense(graph_t *graph) {
    HARD_ASSERT(graph != NULL, "graph is NULL");

    size_t total_weights = 0U;
    dijkstra_status_t status = checked_mul_size(graph->vertex_count, graph->vertex_count, &total_weights);
    RETURN_IF_ERROR(status != DIJKSTRA_STATUS_OK,
                    status,
                    "dijkstra: dense graph size overflow for n=%zu", graph->vertex_count);

    status = translate_vector_status(vector_init(&graph->dense_weights,
                                                 (total_weights == 0U) ? 1U : total_weights,
                                                 sizeof(uint32_t)));
    RETURN_IF_ERROR(status != DIJKSTRA_STATUS_OK,
                    status,
                    "dijkstra: failed to allocate dense matrix for n=%zu", graph->vertex_count);

    graph->dense_weights.size = total_weights;
    uint32_t *weights = (uint32_t *)graph->dense_weights.data;

    for (size_t from = 0U; from < graph->vertex_count; ++from) {
        size_t row_offset = from * graph->vertex_count;
        for (size_t to = 0U; to < graph->vertex_count; ++to) {
            weights[row_offset + to] = (from == to) ? 0U : edge_weight(graph->seed, from, to, 0xA5A5A5A5ULL);
        }
    }

    return DIJKSTRA_STATUS_OK;
}

static bool sparse_has_target(const graph_t *graph, size_t offset_begin, size_t target) {
    HARD_ASSERT(graph != NULL, "graph is NULL");

    const graph_edge_t *edges = (const graph_edge_t *)graph->sparse_edges.data;
    for (size_t index = offset_begin; index < graph->sparse_edges.size; ++index) {
        if (edges[index].to == target) {
            return true;
        }
    }
    return false;
}

static dijkstra_status_t graph_add_sparse_edge(graph_t *graph, size_t from, size_t to, uint64_t salt) {
    HARD_ASSERT(graph != NULL, "graph is NULL");

    graph_edge_t edge = {
        .to = to,
        .weight = edge_weight(graph->seed, from, to, salt)
    };
    return translate_vector_status(vector_push_back(&graph->sparse_edges, &edge));
}

static dijkstra_status_t graph_init_sparse(graph_t *graph) {
    HARD_ASSERT(graph != NULL, "graph is NULL");

    size_t edge_capacity = 0U;
    dijkstra_status_t status = checked_mul_size(graph->vertex_count,
                                                SPARSE_RANDOM_OUT_DEGREE + 1U,
                                                &edge_capacity);
    RETURN_IF_ERROR(status != DIJKSTRA_STATUS_OK,
                    status,
                    "dijkstra: sparse edge capacity overflow for n=%zu", graph->vertex_count);

    status = translate_vector_status(vector_init(&graph->sparse_offsets,
                                                 graph->vertex_count + 1U,
                                                 sizeof(size_t)));
    RETURN_IF_ERROR(status != DIJKSTRA_STATUS_OK,
                    status,
                    "dijkstra: failed to allocate sparse offsets for n=%zu", graph->vertex_count);

    status = translate_vector_status(vector_init(&graph->sparse_edges,
                                                 (edge_capacity == 0U) ? 1U : edge_capacity,
                                                 sizeof(graph_edge_t)));
    RETURN_IF_ERROR(status != DIJKSTRA_STATUS_OK,
                    status,
                    "dijkstra: failed to allocate sparse edges for n=%zu", graph->vertex_count);

    for (size_t from = 0U; from < graph->vertex_count; ++from) {
        size_t offset = graph->sparse_edges.size;
        status = translate_vector_status(vector_push_back(&graph->sparse_offsets, &offset));
        RETURN_IF_ERROR(status != DIJKSTRA_STATUS_OK,
                        status,
                        "dijkstra: failed to append sparse offset for vertex=%zu", from);

        if (graph->vertex_count == 1U) {
            continue;
        }

        size_t cycle_target = (from + 1U) % graph->vertex_count;
        status = graph_add_sparse_edge(graph, from, cycle_target, 0x12345678ULL);
        RETURN_IF_ERROR(status != DIJKSTRA_STATUS_OK,
                        status,
                        "dijkstra: failed to append cycle edge from=%zu to=%zu", from, cycle_target);

        size_t desired_extra = graph->vertex_count - 1U;
        if (desired_extra > 0U) {
            desired_extra--;
        }
        if (desired_extra > SPARSE_RANDOM_OUT_DEGREE) {
            desired_extra = SPARSE_RANDOM_OUT_DEGREE;
        }

        size_t added = 0U;
        size_t attempt = 0U;
        while (added < desired_extra) {
            size_t candidate = (size_t)(splitmix64(((uint64_t)graph->seed << 32U) ^
                                                   ((uint64_t)from * 0x9E3779B97F4A7C15ULL) ^
                                                   ((uint64_t)attempt * 0xBF58476D1CE4E5B9ULL)) %
                                        graph->vertex_count);
            attempt++;

            if (candidate == from || candidate == cycle_target) {
                continue;
            }
            if (sparse_has_target(graph, offset, candidate)) {
                continue;
            }

            status = graph_add_sparse_edge(graph, from, candidate, (uint64_t)attempt);
            RETURN_IF_ERROR(status != DIJKSTRA_STATUS_OK,
                            status,
                            "dijkstra: failed to append sparse edge from=%zu to=%zu", from, candidate);
            added++;
        }
    }

    size_t tail = graph->sparse_edges.size;
    return translate_vector_status(vector_push_back(&graph->sparse_offsets, &tail));
}

dijkstra_status_t graph_create_random(size_t vertex_count, graph_kind_t kind, unsigned seed, graph_t **out_graph) {
    SOFT_ASSERT_FUNCTIONAL(out_graph != NULL, "out_graph must not be NULL", return DIJKSTRA_STATUS_NULL_ARG);
    RETURN_IF_ERROR(vertex_count == 0U,
                    DIJKSTRA_STATUS_INVALID_ARG,
                    "dijkstra: vertex_count must be greater than zero");

    *out_graph = NULL;
    graph_t *graph = calloc(1U, sizeof(*graph));
    RETURN_IF_ERROR(graph == NULL,
                    DIJKSTRA_STATUS_ALLOC_FAIL,
                    "dijkstra: cannot allocate graph for %zu vertices", vertex_count);

    graph->vertex_count = vertex_count;
    graph->kind = kind;
    graph->seed = seed;

    dijkstra_status_t status = DIJKSTRA_STATUS_OK;
    switch (kind) {
        case GRAPH_KIND_SPARSE:
            status = graph_init_sparse(graph);
            break;
        case GRAPH_KIND_DENSE:
            status = graph_init_dense(graph);
            break;
        default:
            status = DIJKSTRA_STATUS_INVALID_ARG;
            break;
    }

    RETURN_IF_ERROR_CLEANUP(status != DIJKSTRA_STATUS_OK,
                            status,
                            graph_release(graph); free(graph),
                            "dijkstra: graph initialization failed with status=%d", (int)status);

    *out_graph = graph;
    return DIJKSTRA_STATUS_OK;
}

void graph_destroy(graph_t *graph) {
    SOFT_ASSERT_FUNCTIONAL(graph != NULL, "graph must not be NULL", return);

    graph_release(graph);
    free(graph);
}

size_t graph_vertex_count(const graph_t *graph) {
    SOFT_ASSERT_FUNCTIONAL(graph != NULL, "graph must not be NULL", return 0U);
    return graph->vertex_count;
}

static void initialize_distances(size_t vertex_count, size_t source, uint64_t *distances_out) {
    HARD_ASSERT(distances_out != NULL, "distances_out is NULL");

    for (size_t vertex = 0U; vertex < vertex_count; ++vertex) {
        distances_out[vertex] = DIJKSTRA_INFINITY;
    }
    distances_out[source] = 0U;
}

static const uint32_t *graph_dense_weights(const graph_t *graph) {
    HARD_ASSERT(graph != NULL, "graph is NULL");
    return (const uint32_t *)graph->dense_weights.data;
}

static const size_t *graph_sparse_offsets(const graph_t *graph) {
    HARD_ASSERT(graph != NULL, "graph is NULL");
    return (const size_t *)graph->sparse_offsets.data;
}

static const graph_edge_t *graph_sparse_edges(const graph_t *graph) {
    HARD_ASSERT(graph != NULL, "graph is NULL");
    return (const graph_edge_t *)graph->sparse_edges.data;
}

static dijkstra_status_t run_naive_relaxation(const graph_t *graph, size_t vertex, bool *visited,
                                              uint64_t *distances_out) {
    HARD_ASSERT(graph != NULL, "graph is NULL");
    HARD_ASSERT(visited != NULL && distances_out != NULL, "buffers must not be NULL");

    uint64_t base_distance = distances_out[vertex];
    if (graph->kind == GRAPH_KIND_DENSE) {
        const uint32_t *weights = graph_dense_weights(graph);
        size_t row_offset = vertex * graph->vertex_count;

        for (size_t to = 0U; to < graph->vertex_count; ++to) {
            if (to == vertex || visited[to]) {
                continue;
            }

            uint64_t candidate = base_distance + weights[row_offset + to];
            if (candidate < distances_out[to]) {
                distances_out[to] = candidate;
            }
        }
        return DIJKSTRA_STATUS_OK;
    }

    const size_t *offsets = graph_sparse_offsets(graph);
    const graph_edge_t *edges = graph_sparse_edges(graph);
    for (size_t edge_index = offsets[vertex]; edge_index < offsets[vertex + 1U]; ++edge_index) {
        size_t to = edges[edge_index].to;
        if (visited[to]) {
            continue;
        }

        uint64_t candidate = base_distance + edges[edge_index].weight;
        if (candidate < distances_out[to]) {
            distances_out[to] = candidate;
        }
    }

    return DIJKSTRA_STATUS_OK;
}

dijkstra_status_t dijkstra_run_naive(const graph_t *graph, size_t source, uint64_t *distances_out) {
    SOFT_ASSERT_FUNCTIONAL(graph != NULL && distances_out != NULL,
                           "graph and distances_out must not be NULL",
                           return DIJKSTRA_STATUS_NULL_ARG);
    RETURN_IF_ERROR(source >= graph->vertex_count,
                    DIJKSTRA_STATUS_INVALID_ARG,
                    "dijkstra: source=%zu is out of range for n=%zu", source, graph->vertex_count);

    bool *visited = calloc(graph->vertex_count, sizeof(*visited));
    RETURN_IF_ERROR(visited == NULL,
                    DIJKSTRA_STATUS_ALLOC_FAIL,
                    "dijkstra: cannot allocate visited array for n=%zu", graph->vertex_count);

    initialize_distances(graph->vertex_count, source, distances_out);

    for (size_t iteration = 0U; iteration < graph->vertex_count; ++iteration) {
        size_t best_vertex = graph->vertex_count;
        uint64_t best_distance = DIJKSTRA_INFINITY;

        for (size_t vertex = 0U; vertex < graph->vertex_count; ++vertex) {
            if (!visited[vertex] && distances_out[vertex] < best_distance) {
                best_distance = distances_out[vertex];
                best_vertex = vertex;
            }
        }

        if (best_vertex == graph->vertex_count) {
            break;
        }

        visited[best_vertex] = true;
        dijkstra_status_t status = run_naive_relaxation(graph, best_vertex, visited, distances_out);
        RETURN_IF_ERROR_CLEANUP(status != DIJKSTRA_STATUS_OK,
                                status,
                                free(visited),
                                "dijkstra: naive relaxation failed for vertex=%zu with status=%d",
                                best_vertex, (int)status);
    }

    free(visited);
    return DIJKSTRA_STATUS_OK;
}

static vertex_distance_t *binary_queue_items(binary_queue_t *queue) {
    HARD_ASSERT(queue != NULL, "queue is NULL");
    return (vertex_distance_t *)queue->items.data;
}

static const vertex_distance_t *binary_queue_items_const(const binary_queue_t *queue) {
    HARD_ASSERT(queue != NULL, "queue is NULL");
    return (const vertex_distance_t *)queue->items.data;
}

static void binary_queue_swap(binary_queue_t *queue, size_t lhs, size_t rhs) {
    HARD_ASSERT(queue != NULL, "queue is NULL");

    vertex_distance_t *items = binary_queue_items(queue);
    vertex_distance_t temp = items[lhs];
    items[lhs] = items[rhs];
    items[rhs] = temp;
    queue->positions[items[lhs].vertex] = lhs;
    queue->positions[items[rhs].vertex] = rhs;
}

static void binary_queue_sift_up(binary_queue_t *queue, size_t index) {
    HARD_ASSERT(queue != NULL, "queue is NULL");

    while (index > 0U) {
        size_t parent = (index - 1U) / 2U;
        const vertex_distance_t *items = binary_queue_items_const(queue);
        if (items[parent].distance <= items[index].distance) {
            return;
        }

        binary_queue_swap(queue, parent, index);
        index = parent;
    }
}

static void binary_queue_sift_down(binary_queue_t *queue, size_t index) {
    HARD_ASSERT(queue != NULL, "queue is NULL");

    while (true) {
        size_t left = index * 2U + 1U;
        if (left >= queue->items.size) {
            return;
        }

        size_t right = left + 1U;
        size_t smallest = left;
        const vertex_distance_t *items = binary_queue_items_const(queue);
        if (right < queue->items.size && items[right].distance < items[left].distance) {
            smallest = right;
        }

        if (items[index].distance <= items[smallest].distance) {
            return;
        }

        binary_queue_swap(queue, index, smallest);
        index = smallest;
    }
}

static dijkstra_status_t binary_queue_init(binary_queue_t *queue, size_t vertex_count) {
    SOFT_ASSERT_FUNCTIONAL(queue != NULL, "queue must not be NULL", return DIJKSTRA_STATUS_NULL_ARG);

    memset(queue, 0, sizeof(*queue));
    queue->positions = malloc(((vertex_count == 0U) ? 1U : vertex_count) * sizeof(queue->positions[0]));
    RETURN_IF_ERROR(queue->positions == NULL,
                    DIJKSTRA_STATUS_ALLOC_FAIL,
                    "dijkstra: cannot allocate binary queue positions for n=%zu", vertex_count);

    for (size_t vertex = 0U; vertex < vertex_count; ++vertex) {
        queue->positions[vertex] = BINARY_QUEUE_ABSENT;
    }

    dijkstra_status_t status = translate_vector_status(vector_init(&queue->items,
                                                                   (vertex_count == 0U) ? 1U : vertex_count,
                                                                   sizeof(vertex_distance_t)));
    RETURN_IF_ERROR_CLEANUP(status != DIJKSTRA_STATUS_OK,
                            status,
                            free(queue->positions); memset(queue, 0, sizeof(*queue)),
                            "dijkstra: cannot initialize binary queue items for n=%zu", vertex_count);

    queue->vertex_count = vertex_count;
    return DIJKSTRA_STATUS_OK;
}

static void binary_queue_destroy(binary_queue_t *queue) {
    SOFT_ASSERT_FUNCTIONAL(queue != NULL, "queue must not be NULL", return);

    free(queue->positions);
    (void)vector_destroy(&queue->items);
    memset(queue, 0, sizeof(*queue));
}

static bool binary_queue_contains(const void *queue_ptr, size_t vertex) {
    const binary_queue_t *queue = (const binary_queue_t *)queue_ptr;
    SOFT_ASSERT_FUNCTIONAL(queue != NULL, "queue must not be NULL", return false);
    if (vertex >= queue->vertex_count || queue->positions == NULL) {
        return false;
    }
    return queue->positions[vertex] != BINARY_QUEUE_ABSENT;
}

static bool binary_queue_is_empty(const void *queue_ptr) {
    const binary_queue_t *queue = (const binary_queue_t *)queue_ptr;
    SOFT_ASSERT_FUNCTIONAL(queue != NULL, "queue must not be NULL", return true);
    return queue->items.size == 0U;
}

static dijkstra_status_t binary_queue_insert(void *queue_ptr, size_t vertex, uint64_t key) {
    binary_queue_t *queue = (binary_queue_t *)queue_ptr;
    SOFT_ASSERT_FUNCTIONAL(queue != NULL, "queue must not be NULL", return DIJKSTRA_STATUS_NULL_ARG);
    RETURN_IF_ERROR(vertex >= queue->vertex_count || binary_queue_contains(queue, vertex),
                    DIJKSTRA_STATUS_INVALID_ARG,
                    "dijkstra: invalid binary queue insert for vertex=%zu", vertex);

    vertex_distance_t item = {.vertex = vertex, .distance = key};
    dijkstra_status_t status = translate_vector_status(vector_push_back(&queue->items, &item));
    RETURN_IF_ERROR(status != DIJKSTRA_STATUS_OK,
                    status,
                    "dijkstra: binary queue push_back failed for vertex=%zu with status=%d",
                    vertex, (int)status);

    size_t index = queue->items.size - 1U;
    queue->positions[vertex] = index;
    binary_queue_sift_up(queue, index);
    return DIJKSTRA_STATUS_OK;
}

static dijkstra_status_t binary_queue_decrease_key(void *queue_ptr, size_t vertex, uint64_t new_key) {
    binary_queue_t *queue = (binary_queue_t *)queue_ptr;
    SOFT_ASSERT_FUNCTIONAL(queue != NULL, "queue must not be NULL", return DIJKSTRA_STATUS_NULL_ARG);
    RETURN_IF_ERROR(!binary_queue_contains(queue, vertex),
                    DIJKSTRA_STATUS_INVALID_ARG,
                    "dijkstra: binary queue decrease_key for missing vertex=%zu", vertex);

    size_t index = queue->positions[vertex];
    vertex_distance_t *items = binary_queue_items(queue);
    RETURN_IF_ERROR(new_key > items[index].distance,
                    DIJKSTRA_STATUS_INVALID_ARG,
                    "dijkstra: binary queue received non-decreasing key for vertex=%zu", vertex);

    items[index].distance = new_key;
    binary_queue_sift_up(queue, index);
    return DIJKSTRA_STATUS_OK;
}

static dijkstra_status_t binary_queue_extract_min(void *queue_ptr, size_t *out_vertex, uint64_t *out_key) {
    binary_queue_t *queue = (binary_queue_t *)queue_ptr;
    SOFT_ASSERT_FUNCTIONAL(queue != NULL && out_vertex != NULL && out_key != NULL,
                           "queue, out_vertex and out_key must not be NULL",
                           return DIJKSTRA_STATUS_NULL_ARG);
    RETURN_IF_ERROR(queue->items.size == 0U,
                    DIJKSTRA_STATUS_INVALID_ARG,
                    "dijkstra: binary queue extract_min called on empty queue");

    vertex_distance_t *items = binary_queue_items(queue);
    vertex_distance_t min_item = items[0];
    queue->positions[min_item.vertex] = BINARY_QUEUE_ABSENT;

    if (queue->items.size == 1U) {
        queue->items.size = 0U;
    } else {
        items[0] = items[queue->items.size - 1U];
        queue->items.size--;
        queue->positions[items[0].vertex] = 0U;
        binary_queue_sift_down(queue, 0U);
    }

    *out_vertex = min_item.vertex;
    *out_key = min_item.distance;
    return DIJKSTRA_STATUS_OK;
}

static size_t binomial_required_slots(size_t size) {
    size_t slots = 1U;
    while (size > 0U) {
        slots++;
        size >>= 1U;
    }
    return slots;
}

static dijkstra_status_t binomial_reserve_slots(binomial_queue_t *queue, size_t required_slots) {
    HARD_ASSERT(queue != NULL, "queue is NULL");

    while (queue->roots.size < required_slots) {
        binomial_queue_node_t *empty = NULL;
        dijkstra_status_t status = translate_vector_status(vector_push_back(&queue->roots, &empty));
        RETURN_IF_ERROR(status != DIJKSTRA_STATUS_OK,
                        status,
                        "dijkstra: failed to reserve binomial root slot with status=%d", (int)status);
    }
    return DIJKSTRA_STATUS_OK;
}

static binomial_queue_node_t **binomial_root_slot(binomial_queue_t *queue, size_t degree) {
    HARD_ASSERT(queue != NULL, "queue is NULL");
    return (binomial_queue_node_t **)vector_get(&queue->roots, degree);
}

static binomial_queue_node_t *binomial_link(binomial_queue_node_t *lhs, binomial_queue_node_t *rhs) {
    HARD_ASSERT(lhs != NULL && rhs != NULL, "binomial trees must not be NULL");

    if (rhs->key < lhs->key) {
        binomial_queue_node_t *temp = lhs;
        lhs = rhs;
        rhs = temp;
    }

    rhs->parent = lhs;
    rhs->sibling = lhs->child;
    lhs->child = rhs;
    lhs->degree++;
    return lhs;
}

static dijkstra_status_t binomial_absorb_tree(binomial_queue_t *queue, binomial_queue_node_t *node) {
    HARD_ASSERT(queue != NULL && node != NULL, "queue and node must not be NULL");

    while (true) {
        binomial_queue_node_t **slot = binomial_root_slot(queue, node->degree);
        if (*slot == NULL) {
            *slot = node;
            return DIJKSTRA_STATUS_OK;
        }

        node = binomial_link(*slot, node);
        *slot = NULL;
    }
}

static void binomial_destroy_tree(binomial_queue_node_t *node) {
    while (node != NULL) {
        binomial_queue_node_t *next = node->sibling;
        binomial_destroy_tree(node->child);
        free(node);
        node = next;
    }
}

static dijkstra_status_t binomial_queue_init(binomial_queue_t *queue, size_t vertex_count) {
    SOFT_ASSERT_FUNCTIONAL(queue != NULL, "queue must not be NULL", return DIJKSTRA_STATUS_NULL_ARG);

    memset(queue, 0, sizeof(*queue));
    queue->handles = calloc((vertex_count == 0U) ? 1U : vertex_count, sizeof(queue->handles[0]));
    RETURN_IF_ERROR(queue->handles == NULL,
                    DIJKSTRA_STATUS_ALLOC_FAIL,
                    "dijkstra: cannot allocate binomial queue handles for n=%zu", vertex_count);

    dijkstra_status_t status = translate_vector_status(vector_init(&queue->roots,
                                                                   binomial_required_slots(vertex_count + 1U),
                                                                   sizeof(binomial_queue_node_t *)));
    RETURN_IF_ERROR_CLEANUP(status != DIJKSTRA_STATUS_OK,
                            status,
                            free(queue->handles); memset(queue, 0, sizeof(*queue)),
                            "dijkstra: cannot initialize binomial queue roots for n=%zu", vertex_count);

    status = binomial_reserve_slots(queue, binomial_required_slots(vertex_count + 1U));
    RETURN_IF_ERROR_CLEANUP(status != DIJKSTRA_STATUS_OK,
                            status,
                            free(queue->handles); (void)vector_destroy(&queue->roots); memset(queue, 0, sizeof(*queue)),
                            "dijkstra: cannot reserve binomial queue root slots for n=%zu", vertex_count);

    queue->vertex_count = vertex_count;
    return DIJKSTRA_STATUS_OK;
}

static void binomial_queue_destroy(binomial_queue_t *queue) {
    SOFT_ASSERT_FUNCTIONAL(queue != NULL, "queue must not be NULL", return);

    for (size_t degree = 0U; degree < queue->roots.size; ++degree) {
        binomial_queue_node_t **slot = (binomial_queue_node_t **)vector_get(&queue->roots, degree);
        if (slot != NULL && *slot != NULL) {
            binomial_destroy_tree(*slot);
        }
    }

    free(queue->handles);
    (void)vector_destroy(&queue->roots);
    memset(queue, 0, sizeof(*queue));
}

static bool binomial_queue_contains(const void *queue_ptr, size_t vertex) {
    const binomial_queue_t *queue = (const binomial_queue_t *)queue_ptr;
    SOFT_ASSERT_FUNCTIONAL(queue != NULL, "queue must not be NULL", return false);
    if (vertex >= queue->vertex_count || queue->handles == NULL) {
        return false;
    }
    return queue->handles[vertex] != NULL;
}

static bool binomial_queue_is_empty(const void *queue_ptr) {
    const binomial_queue_t *queue = (const binomial_queue_t *)queue_ptr;
    SOFT_ASSERT_FUNCTIONAL(queue != NULL, "queue must not be NULL", return true);
    return queue->size == 0U;
}

static dijkstra_status_t binomial_queue_insert(void *queue_ptr, size_t vertex, uint64_t key) {
    binomial_queue_t *queue = (binomial_queue_t *)queue_ptr;
    SOFT_ASSERT_FUNCTIONAL(queue != NULL, "queue must not be NULL", return DIJKSTRA_STATUS_NULL_ARG);
    RETURN_IF_ERROR(vertex >= queue->vertex_count || binomial_queue_contains(queue, vertex),
                    DIJKSTRA_STATUS_INVALID_ARG,
                    "dijkstra: invalid binomial queue insert for vertex=%zu", vertex);

    dijkstra_status_t status = binomial_reserve_slots(queue, binomial_required_slots(queue->size + 1U));
    RETURN_IF_ERROR(status != DIJKSTRA_STATUS_OK,
                    status,
                    "dijkstra: binomial queue reserve slots failed with status=%d", (int)status);

    binomial_queue_node_t *node = calloc(1U, sizeof(*node));
    RETURN_IF_ERROR(node == NULL,
                    DIJKSTRA_STATUS_ALLOC_FAIL,
                    "dijkstra: cannot allocate binomial queue node for vertex=%zu", vertex);

    node->vertex = vertex;
    node->key = key;
    status = binomial_absorb_tree(queue, node);
    RETURN_IF_ERROR_CLEANUP(status != DIJKSTRA_STATUS_OK,
                            status,
                            free(node),
                            "dijkstra: binomial absorb_tree failed for vertex=%zu with status=%d",
                            vertex, (int)status);

    queue->handles[vertex] = node;
    queue->size++;
    return DIJKSTRA_STATUS_OK;
}

static void binomial_swap_payloads(binomial_queue_t *queue,
                                   binomial_queue_node_t *lhs,
                                   binomial_queue_node_t *rhs) {
    HARD_ASSERT(queue != NULL && lhs != NULL && rhs != NULL, "arguments must not be NULL");

    size_t vertex = lhs->vertex;
    uint64_t key = lhs->key;

    lhs->vertex = rhs->vertex;
    lhs->key = rhs->key;
    rhs->vertex = vertex;
    rhs->key = key;

    queue->handles[lhs->vertex] = lhs;
    queue->handles[rhs->vertex] = rhs;
}

static dijkstra_status_t binomial_queue_decrease_key(void *queue_ptr, size_t vertex, uint64_t new_key) {
    binomial_queue_t *queue = (binomial_queue_t *)queue_ptr;
    SOFT_ASSERT_FUNCTIONAL(queue != NULL, "queue must not be NULL", return DIJKSTRA_STATUS_NULL_ARG);
    RETURN_IF_ERROR(!binomial_queue_contains(queue, vertex),
                    DIJKSTRA_STATUS_INVALID_ARG,
                    "dijkstra: binomial queue decrease_key for missing vertex=%zu", vertex);

    binomial_queue_node_t *node = queue->handles[vertex];
    RETURN_IF_ERROR(new_key > node->key,
                    DIJKSTRA_STATUS_INVALID_ARG,
                    "dijkstra: binomial queue received non-decreasing key for vertex=%zu", vertex);

    node->key = new_key;
    while (node->parent != NULL && node->key < node->parent->key) {
        binomial_swap_payloads(queue, node, node->parent);
        node = node->parent;
    }

    return DIJKSTRA_STATUS_OK;
}

static dijkstra_status_t binomial_queue_extract_min(void *queue_ptr, size_t *out_vertex, uint64_t *out_key) {
    binomial_queue_t *queue = (binomial_queue_t *)queue_ptr;
    SOFT_ASSERT_FUNCTIONAL(queue != NULL && out_vertex != NULL && out_key != NULL,
                           "queue, out_vertex and out_key must not be NULL",
                           return DIJKSTRA_STATUS_NULL_ARG);
    RETURN_IF_ERROR(queue->size == 0U,
                    DIJKSTRA_STATUS_INVALID_ARG,
                    "dijkstra: binomial queue extract_min called on empty queue");

    size_t best_degree = queue->roots.size;
    binomial_queue_node_t *best = NULL;
    for (size_t degree = 0U; degree < queue->roots.size; ++degree) {
        binomial_queue_node_t **slot = binomial_root_slot(queue, degree);
        if (slot == NULL || *slot == NULL) {
            continue;
        }
        if (best == NULL || (*slot)->key < best->key) {
            best = *slot;
            best_degree = degree;
        }
    }

    RETURN_IF_ERROR(best == NULL || best_degree == queue->roots.size,
                    DIJKSTRA_STATUS_INTERNAL,
                    "dijkstra: no minimum root found in non-empty binomial queue");

    *binomial_root_slot(queue, best_degree) = NULL;
    queue->handles[best->vertex] = NULL;
    queue->size--;
    *out_vertex = best->vertex;
    *out_key = best->key;

    binomial_queue_node_t *child = best->child;
    while (child != NULL) {
        binomial_queue_node_t *next = child->sibling;
        child->parent = NULL;
        child->sibling = NULL;
        dijkstra_status_t status = binomial_absorb_tree(queue, child);
        RETURN_IF_ERROR_CLEANUP(status != DIJKSTRA_STATUS_OK,
                                status,
                                free(best),
                                "dijkstra: binomial absorb_tree during extract_min failed with status=%d",
                                (int)status);
        child = next;
    }

    free(best);
    return DIJKSTRA_STATUS_OK;
}

static bool fibonacci_queue_contains_wrapper(const void *queue_ptr, size_t vertex) {
    return fibonacci_heap_contains((const fibonacci_heap_t *)queue_ptr, vertex);
}

static bool fibonacci_queue_is_empty_wrapper(const void *queue_ptr) {
    return fibonacci_heap_is_empty((const fibonacci_heap_t *)queue_ptr);
}

static dijkstra_status_t fibonacci_queue_insert_wrapper(void *queue_ptr, size_t vertex, uint64_t key) {
    return fibonacci_heap_insert((fibonacci_heap_t *)queue_ptr, vertex, key);
}

static dijkstra_status_t fibonacci_queue_decrease_wrapper(void *queue_ptr, size_t vertex, uint64_t new_key) {
    return fibonacci_heap_decrease_key((fibonacci_heap_t *)queue_ptr, vertex, new_key);
}

static dijkstra_status_t fibonacci_queue_extract_wrapper(void *queue_ptr, size_t *out_vertex, uint64_t *out_key) {
    return fibonacci_heap_extract_min((fibonacci_heap_t *)queue_ptr, out_vertex, out_key);
}

static dijkstra_status_t run_heap_dijkstra(const graph_t *graph, size_t source, uint64_t *distances_out,
                                           void *queue, const priority_queue_ops_t *ops) {
    SOFT_ASSERT_FUNCTIONAL(graph != NULL && distances_out != NULL && queue != NULL && ops != NULL,
                           "graph, distances_out, queue and ops must not be NULL",
                           return DIJKSTRA_STATUS_NULL_ARG);
    RETURN_IF_ERROR(source >= graph->vertex_count,
                    DIJKSTRA_STATUS_INVALID_ARG,
                    "dijkstra: source=%zu is out of range for n=%zu", source, graph->vertex_count);

    bool *visited = calloc(graph->vertex_count, sizeof(*visited));
    RETURN_IF_ERROR(visited == NULL,
                    DIJKSTRA_STATUS_ALLOC_FAIL,
                    "dijkstra: cannot allocate visited array for n=%zu", graph->vertex_count);

    initialize_distances(graph->vertex_count, source, distances_out);
    dijkstra_status_t status = ops->insert(queue, source, 0U);
    RETURN_IF_ERROR_CLEANUP(status != DIJKSTRA_STATUS_OK,
                            status,
                            free(visited),
                            "dijkstra: initial queue insert failed with status=%d", (int)status);

    while (!ops->is_empty(queue)) {
        size_t vertex = 0U;
        uint64_t distance = 0U;
        status = ops->extract_min(queue, &vertex, &distance);
        RETURN_IF_ERROR_CLEANUP(status != DIJKSTRA_STATUS_OK,
                                status,
                                free(visited),
                                "dijkstra: extract_min failed with status=%d", (int)status);
        RETURN_IF_ERROR_CLEANUP(vertex >= graph->vertex_count || visited[vertex] || distance != distances_out[vertex],
                                DIJKSTRA_STATUS_INTERNAL,
                                free(visited),
                                "dijkstra: queue returned inconsistent state for vertex=%zu", vertex);

        visited[vertex] = true;
        if (graph->kind == GRAPH_KIND_DENSE) {
            const uint32_t *weights = graph_dense_weights(graph);
            size_t row_offset = vertex * graph->vertex_count;

            for (size_t to = 0U; to < graph->vertex_count; ++to) {
                if (to == vertex || visited[to]) {
                    continue;
                }

                uint64_t candidate = distance + weights[row_offset + to];
                if (candidate >= distances_out[to]) {
                    continue;
                }

                distances_out[to] = candidate;
                status = ops->contains(queue, to)
                         ? ops->decrease_key(queue, to, candidate)
                         : ops->insert(queue, to, candidate);
                RETURN_IF_ERROR_CLEANUP(status != DIJKSTRA_STATUS_OK,
                                        status,
                                        free(visited),
                                        "dijkstra: queue update failed for vertex=%zu with status=%d",
                                        to, (int)status);
            }
            continue;
        }

        const size_t *offsets = graph_sparse_offsets(graph);
        const graph_edge_t *edges = graph_sparse_edges(graph);
        for (size_t edge_index = offsets[vertex]; edge_index < offsets[vertex + 1U]; ++edge_index) {
            size_t to = edges[edge_index].to;
            if (visited[to]) {
                continue;
            }

            uint64_t candidate = distance + edges[edge_index].weight;
            if (candidate >= distances_out[to]) {
                continue;
            }

            distances_out[to] = candidate;
            status = ops->contains(queue, to)
                     ? ops->decrease_key(queue, to, candidate)
                     : ops->insert(queue, to, candidate);
            RETURN_IF_ERROR_CLEANUP(status != DIJKSTRA_STATUS_OK,
                                    status,
                                    free(visited),
                                    "dijkstra: queue update failed for vertex=%zu with status=%d",
                                    to, (int)status);
        }
    }

    free(visited);
    return DIJKSTRA_STATUS_OK;
}

static const priority_queue_ops_t BINARY_QUEUE_OPS = {
    .insert = binary_queue_insert,
    .decrease_key = binary_queue_decrease_key,
    .extract_min = binary_queue_extract_min,
    .contains = binary_queue_contains,
    .is_empty = binary_queue_is_empty
};

static const priority_queue_ops_t BINOMIAL_QUEUE_OPS = {
    .insert = binomial_queue_insert,
    .decrease_key = binomial_queue_decrease_key,
    .extract_min = binomial_queue_extract_min,
    .contains = binomial_queue_contains,
    .is_empty = binomial_queue_is_empty
};

static const priority_queue_ops_t FIBONACCI_QUEUE_OPS = {
    .insert = fibonacci_queue_insert_wrapper,
    .decrease_key = fibonacci_queue_decrease_wrapper,
    .extract_min = fibonacci_queue_extract_wrapper,
    .contains = fibonacci_queue_contains_wrapper,
    .is_empty = fibonacci_queue_is_empty_wrapper
};

dijkstra_status_t dijkstra_run_binary_heap(const graph_t *graph, size_t source, uint64_t *distances_out) {
    SOFT_ASSERT_FUNCTIONAL(graph != NULL, "graph must not be NULL", return DIJKSTRA_STATUS_NULL_ARG);

    binary_queue_t queue = {0};
    dijkstra_status_t status = binary_queue_init(&queue, graph->vertex_count);
    RETURN_IF_ERROR(status != DIJKSTRA_STATUS_OK,
                    status,
                    "dijkstra: binary queue init failed with status=%d", (int)status);

    status = run_heap_dijkstra(graph, source, distances_out, &queue, &BINARY_QUEUE_OPS);
    binary_queue_destroy(&queue);
    return status;
}

dijkstra_status_t dijkstra_run_binomial_heap(const graph_t *graph, size_t source, uint64_t *distances_out) {
    SOFT_ASSERT_FUNCTIONAL(graph != NULL, "graph must not be NULL", return DIJKSTRA_STATUS_NULL_ARG);

    binomial_queue_t queue = {0};
    dijkstra_status_t status = binomial_queue_init(&queue, graph->vertex_count);
    RETURN_IF_ERROR(status != DIJKSTRA_STATUS_OK,
                    status,
                    "dijkstra: binomial queue init failed with status=%d", (int)status);

    status = run_heap_dijkstra(graph, source, distances_out, &queue, &BINOMIAL_QUEUE_OPS);
    binomial_queue_destroy(&queue);
    return status;
}

dijkstra_status_t dijkstra_run_fibonacci_heap(const graph_t *graph, size_t source, uint64_t *distances_out) {
    SOFT_ASSERT_FUNCTIONAL(graph != NULL, "graph must not be NULL", return DIJKSTRA_STATUS_NULL_ARG);

    fibonacci_heap_t heap = {0};
    dijkstra_status_t status = fibonacci_heap_init(&heap, graph->vertex_count);
    RETURN_IF_ERROR(status != DIJKSTRA_STATUS_OK,
                    status,
                    "dijkstra: fibonacci heap init failed with status=%d", (int)status);

    status = run_heap_dijkstra(graph, source, distances_out, &heap, &FIBONACCI_QUEUE_OPS);
    fibonacci_heap_destroy(&heap);
    return status;
}
