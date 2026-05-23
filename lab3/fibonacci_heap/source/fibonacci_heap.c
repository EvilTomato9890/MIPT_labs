#include "fibonacci_heap.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "asserts.h"
#include "return_macros.h"

struct fibonacci_heap_node {
    size_t vertex;
    uint64_t key;
    size_t degree;
    bool mark;

    struct fibonacci_heap_node *parent;
    struct fibonacci_heap_node *child;
    struct fibonacci_heap_node *left;
    struct fibonacci_heap_node *right;
};

static void list_init_single(fibonacci_heap_node_t *node) {
    HARD_ASSERT(node != NULL, "node is NULL");

    node->left  = node;
    node->right = node;
}

static void list_remove(fibonacci_heap_node_t *node) {
    HARD_ASSERT(node != NULL, "node is NULL");

    node->left->right = node->right;
    node->right->left = node->left;
    list_init_single(node);
}

static void list_insert_after(fibonacci_heap_node_t *position, fibonacci_heap_node_t *node) {
    HARD_ASSERT(position != NULL && node != NULL, "position and node must not be NULL");

    node->right = position->right;
    node->left  = position;
    position->right->left = node;
    position->right       = node;
}

static void add_to_root_list(fibonacci_heap_t *heap, fibonacci_heap_node_t *node) {
    HARD_ASSERT(heap != NULL && node != NULL, "heap and node must not be NULL");

    node->parent = NULL;
    node->mark = false;
    if (heap->min == NULL) {
        list_init_single(node);
        heap->min = node;
        return;
    }

    list_insert_after(heap->min, node);
    if (node->key < heap->min->key) {
        heap->min = node;
    }
}

static void link_under_root(fibonacci_heap_t *heap, fibonacci_heap_node_t *child, fibonacci_heap_node_t *root) {
    HARD_ASSERT(heap != NULL && child != NULL && root != NULL, "arguments must not be NULL");

    if (child == heap->min) {
        heap->min = child->right;
    }
    list_remove(child);
    child->parent = root;
    child->mark = false;

    if (root->child == NULL) {
        root->child = child;
        list_init_single(child);
    } else {
        list_insert_after(root->child, child);
    }

    root->degree++;
}

static void cut_node(fibonacci_heap_t *heap, fibonacci_heap_node_t *node, fibonacci_heap_node_t *parent) {
    HARD_ASSERT(heap != NULL && node != NULL && parent != NULL, "arguments must not be NULL");

    if (node->right == node) {
        parent->child = NULL;
    } else {
        if (parent->child == node) {
            parent->child = node->right;
        }
        node->left->right = node->right;
        node->right->left = node->left;
    }

    parent->degree--;
    list_init_single(node);
    add_to_root_list(heap, node);
}

static void cascading_cut(fibonacci_heap_t *heap, fibonacci_heap_node_t *node) {
    HARD_ASSERT(heap != NULL && node != NULL, "heap and node must not be NULL");

    while (node->parent != NULL) {
        fibonacci_heap_node_t *parent = node->parent;
        if (!node->mark) {
            node->mark = true;
            return;
        }

        cut_node(heap, node, parent);
        node = parent;
    }
}

static void destroy_circular_list(fibonacci_heap_node_t *start) {
    SOFT_ASSERT_FUNCTIONAL(start != NULL, "start must not be NULL", return);

    fibonacci_heap_node_t *node = start->right;
    while (node != start) {
        fibonacci_heap_node_t *next = node->right;
        if (node->child != NULL) {
            destroy_circular_list(node->child);
        }
        free(node);
        node = next;
    }

    if (start->child != NULL) {
        destroy_circular_list(start->child);
    }
    free(start);
}

static dijkstra_status_t consolidate(fibonacci_heap_t *heap) {
    HARD_ASSERT(heap != NULL, "heap is NULL");

    if (heap->min == NULL || heap->size < 2U) {
        return DIJKSTRA_STATUS_OK;
    }

    size_t root_count = 1U;
    for (fibonacci_heap_node_t *node = heap->min->right; node != heap->min; node = node->right) {
        root_count++;
    }

    fibonacci_heap_node_t **roots = calloc(root_count, sizeof(*roots));
    RETURN_IF_ERROR(roots == NULL,
                    DIJKSTRA_STATUS_ALLOC_FAIL,
                    "fibonacci_heap: cannot allocate temporary root list");

    size_t slot_count = heap->size + 1U;
    fibonacci_heap_node_t **slots = calloc(slot_count, sizeof(*slots));
    RETURN_IF_ERROR_CLEANUP(slots == NULL,
                            DIJKSTRA_STATUS_ALLOC_FAIL,
                            free(roots),
                            "fibonacci_heap: cannot allocate consolidate slots");

    size_t index = 0U;
    roots[index++] = heap->min;
    for (fibonacci_heap_node_t *node = heap->min->right; node != heap->min; node = node->right) {
        roots[index++] = node;
    }

    for (size_t i = 0U; i < root_count; ++i) {
        fibonacci_heap_node_t *current = roots[i];
        size_t degree = current->degree;

        while (slots[degree] != NULL) {
            fibonacci_heap_node_t *other = slots[degree];
            if (other->key < current->key) {
                fibonacci_heap_node_t *temp = current;
                current = other;
                other   = temp;
            }

            link_under_root(heap, other, current);
            slots[degree] = NULL;
            degree++;
        }

        slots[degree] = current;
    }

    heap->min = NULL;
    for (size_t i = 0U; i < slot_count; ++i) {
        if (slots[i] == NULL) continue;

        list_init_single(slots[i]);
        add_to_root_list(heap, slots[i]);
    }

    free(slots);
    free(roots);
    return DIJKSTRA_STATUS_OK;
}

dijkstra_status_t fibonacci_heap_init(fibonacci_heap_t *heap, size_t vertex_count) {
    SOFT_ASSERT_FUNCTIONAL(heap != NULL, "heap must not be NULL", return DIJKSTRA_STATUS_NULL_ARG);
    RETURN_IF_ERROR(vertex_count == 0U,
                    DIJKSTRA_STATUS_INVALID_ARG,
                    "fibonacci_heap: vertex_count must be greater than zero");

    fibonacci_heap_destroy(heap);

    heap->handles = calloc(vertex_count, sizeof(heap->handles[0]));
    RETURN_IF_ERROR(heap->handles == NULL,
                    DIJKSTRA_STATUS_ALLOC_FAIL,
                    "fibonacci_heap: cannot allocate handle table for %zu vertices", vertex_count);

    heap->min  = NULL;
    heap->size = 0U;
    heap->vertex_count = vertex_count;
    return DIJKSTRA_STATUS_OK;
}

void fibonacci_heap_destroy(fibonacci_heap_t *heap) {
    SOFT_ASSERT_FUNCTIONAL(heap != NULL, "heap must not be NULL", return);

    if (heap->min != NULL) {
        destroy_circular_list(heap->min);
    }
    free(heap->handles);
    memset(heap, 0, sizeof(*heap));
}

bool fibonacci_heap_is_empty(const fibonacci_heap_t *heap) {
    SOFT_ASSERT_FUNCTIONAL(heap != NULL, "heap must not be NULL", return true);
    return heap->size == 0U;
}

bool fibonacci_heap_contains(const fibonacci_heap_t *heap, size_t vertex) {
    SOFT_ASSERT_FUNCTIONAL(heap != NULL, "heap must not be NULL", return false);
    if (vertex >= heap->vertex_count || heap->handles == NULL) {
        return false;
    }
    return heap->handles[vertex] != NULL;
}

dijkstra_status_t fibonacci_heap_insert(fibonacci_heap_t *heap, size_t vertex, uint64_t key) {
    SOFT_ASSERT_FUNCTIONAL(heap != NULL, "heap must not be NULL", return DIJKSTRA_STATUS_NULL_ARG);
    RETURN_IF_ERROR(vertex >= heap->vertex_count || fibonacci_heap_contains(heap, vertex),
                    DIJKSTRA_STATUS_INVALID_ARG,
                    "fibonacci_heap: invalid insert for vertex=%zu", vertex);

    fibonacci_heap_node_t *node = calloc(1U, sizeof(*node));
    RETURN_IF_ERROR(node == NULL,
                    DIJKSTRA_STATUS_ALLOC_FAIL,
                    "fibonacci_heap: cannot allocate node for vertex=%zu", vertex);

    node->vertex = vertex;
    node->key    = key;
    list_init_single(node);

    add_to_root_list(heap, node);
    heap->handles[vertex] = node;
    heap->size++;
    return DIJKSTRA_STATUS_OK;
}

dijkstra_status_t fibonacci_heap_decrease_key(fibonacci_heap_t *heap, size_t vertex, uint64_t new_key) {
    SOFT_ASSERT_FUNCTIONAL(heap != NULL, "heap must not be NULL", return DIJKSTRA_STATUS_NULL_ARG);
    RETURN_IF_ERROR(!fibonacci_heap_contains(heap, vertex),
                    DIJKSTRA_STATUS_INVALID_ARG,
                    "fibonacci_heap: decrease_key for missing vertex=%zu", vertex);

    fibonacci_heap_node_t *node = heap->handles[vertex];
    RETURN_IF_ERROR(new_key > node->key,
                    DIJKSTRA_STATUS_INVALID_ARG,
                    "fibonacci_heap: decrease_key received non-decreasing key for vertex=%zu", vertex);

    node->key = new_key;
    fibonacci_heap_node_t *parent = node->parent;
    if (parent != NULL && node->key < parent->key) {
        cut_node(heap, node, parent);
        cascading_cut(heap, parent);
    }

    if (heap->min == NULL || node->key < heap->min->key) {
        heap->min = node;
    }
    return DIJKSTRA_STATUS_OK;
}

dijkstra_status_t fibonacci_heap_extract_min(fibonacci_heap_t *heap, size_t *out_vertex, uint64_t *out_key) {
    SOFT_ASSERT_FUNCTIONAL(heap != NULL && out_vertex != NULL && out_key != NULL,
                           "heap, out_vertex and out_key must not be NULL",
                           return DIJKSTRA_STATUS_NULL_ARG);
    RETURN_IF_ERROR(heap->min == NULL || heap->size == 0U,
                    DIJKSTRA_STATUS_INVALID_ARG,
                    "fibonacci_heap: extract_min called on empty heap");

    fibonacci_heap_node_t *min_node = heap->min;
    if (min_node->child != NULL) {
        fibonacci_heap_node_t *child = min_node->child->right;
        while (child != min_node->child) {
            fibonacci_heap_node_t *next = child->right;
            list_remove(child);
            add_to_root_list(heap, child);
            child = next;
        }

        list_remove(min_node->child);
        add_to_root_list(heap, min_node->child);
        min_node->child = NULL;
    }

    if (min_node->right == min_node) {
        heap->min = NULL;
    } else {
        heap->min = min_node->right;
        min_node->left->right = min_node->right;
        min_node->right->left = min_node->left;
        list_init_single(min_node);
    }

    *out_vertex = min_node->vertex;
    *out_key    = min_node->key;
    heap->handles[min_node->vertex] = NULL;
    heap->size--;

    dijkstra_status_t status = consolidate(heap);
    RETURN_IF_ERROR_CLEANUP(status != DIJKSTRA_STATUS_OK,
                            status,
                            free(min_node),
                            "fibonacci_heap: consolidate failed with status=%d", (int)status);

    free(min_node);
    return DIJKSTRA_STATUS_OK;
}
