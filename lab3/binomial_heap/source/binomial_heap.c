#include "binomial_heap.h"

#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#include "asserts.h"
#include "return_macros.h"

struct binomial_heap_node {
    int key;
    size_t degree;

    struct binomial_heap_node *parent;
    struct binomial_heap_node *child;
    struct binomial_heap_node *sibling;
};

static heap_status_t translate_vector_status(vector_error_t status) {
    switch (status) {
        case VEC_ERR_OK:        return HEAP_STATUS_OK;
        case VEC_ERR_MEM_ALLOC: return HEAP_STATUS_ALLOC_FAIL;
        case VEC_ERR_BAD_ARG:
        case VEC_ERR_FULL:      return HEAP_STATUS_INVALID_ARG;
        case VEC_ERR_NOT_FOUND: return HEAP_STATUS_EMPTY;
        case VEC_ERR_INTERNAL:
        default:                return HEAP_STATUS_INTERNAL;
    }
}

static size_t required_root_slots(size_t size) {
    size_t slots = 1U;
    while (size > 0U) {
        ++slots;
        size >>= 1U;
    }
    return slots;
}

static heap_status_t reserve_root_slots(binomial_heap_t *heap, size_t required_slots) {
    HARD_ASSERT(heap != NULL, "heap is NULL");

    while (heap->roots.size < required_slots) {
        binomial_heap_node_t *empty_root = NULL;
        heap_status_t status = translate_vector_status(vector_push_back(&heap->roots, &empty_root));
        RETURN_IF_ERROR(status != HEAP_STATUS_OK,
                        status,
                        "binomial_heap: failed to reserve root slot with status=%d", (int)status);
    }
    return HEAP_STATUS_OK;
}

static binomial_heap_node_t **root_slot(binomial_heap_t *heap, size_t degree) {
    HARD_ASSERT(heap != NULL, "heap is NULL");
    HARD_ASSERT(degree < heap->roots.size, "degree is out of range");
    return (binomial_heap_node_t **)vector_get(&heap->roots, degree);
}

static const binomial_heap_node_t *const *root_slot_const(const binomial_heap_t *heap, size_t degree) {
    HARD_ASSERT(heap != NULL, "heap is NULL");
    HARD_ASSERT(degree < heap->roots.size, "degree is out of range");
    return (const binomial_heap_node_t *const *)vector_get_const(&heap->roots, degree);
}

static binomial_heap_node_t *link_trees(binomial_heap_node_t *lhs, binomial_heap_node_t *rhs) {
    HARD_ASSERT(lhs != NULL && rhs != NULL, "tree root is NULL");
    HARD_ASSERT(lhs->degree == rhs->degree, "tree degrees must match");

    if (rhs->key < lhs->key) {
        binomial_heap_node_t *temp = lhs;
        lhs = rhs;
        rhs = temp;
    }

    rhs->parent  = lhs;
    rhs->sibling = lhs->child;
    lhs->child   = rhs;
    lhs->degree++;
    return lhs;
}

static heap_status_t absorb_tree(binomial_heap_t *heap, binomial_heap_node_t *carry) {
    HARD_ASSERT(heap != NULL, "heap is NULL");
    HARD_ASSERT(carry != NULL, "carry is NULL");

    while (true) {
        binomial_heap_node_t **slot = root_slot(heap, carry->degree);
        if (*slot == NULL) {
            *slot = carry;
            return HEAP_STATUS_OK;
        }

        carry = link_trees(*slot, carry);
        *slot = NULL;
    }
}

static void destroy_tree(binomial_heap_node_t *node) {
    while (node != NULL) {
        binomial_heap_node_t *next = node->sibling;
        destroy_tree(node->child);
        free(node);
        node = next;
    }
}

static bool validate_tree(const binomial_heap_node_t *node, size_t *node_count) {
    HARD_ASSERT(node != NULL, "node is NULL");
    HARD_ASSERT(node_count != NULL, "node_count is NULL");

    size_t children = 0U;
    size_t total = 1U;

    for (const binomial_heap_node_t *child = node->child; child != NULL; child = child->sibling) {
        if (child->parent != node)         return false;
        if (child->key    <  node->key)    return false;
        if (child->degree >= node->degree) return false;

        size_t subtree_count = 0U;
        if (!validate_tree(child, &subtree_count)) {
            return false;
        }

        total += subtree_count;
        children++;
    }

    if (children != node->degree) return false;

    *node_count = total;
    return true;
}

heap_status_t binomial_heap_init(binomial_heap_t *heap) {
    SOFT_ASSERT_FUNCTIONAL(heap != NULL, "heap must not be NULL", return HEAP_STATUS_NULL_ARG);

    binomial_heap_destroy(heap);

    heap_status_t status = translate_vector_status(vector_init(&heap->roots, 1U, sizeof(binomial_heap_node_t *)));
    RETURN_IF_ERROR(status != HEAP_STATUS_OK,
                    status,
                    "binomial_heap: init failed with status=%d", (int)status);

    heap->size = 0U;
    return HEAP_STATUS_OK;
}

void binomial_heap_destroy(binomial_heap_t *heap) {
    SOFT_ASSERT_FUNCTIONAL(heap != NULL, "heap must not be NULL", return);

    for (size_t degree = 0U; degree < heap->roots.size; ++degree) {
        binomial_heap_node_t **slot = (binomial_heap_node_t **)vector_get(&heap->roots, degree);
        if (slot != NULL && *slot != NULL) {
            destroy_tree(*slot);
            *slot = NULL;
        }
    }

    (void)vector_destroy(&heap->roots);
    heap->size = 0U;
}

heap_status_t binomial_heap_insert(binomial_heap_t *heap, int value) {
    SOFT_ASSERT_FUNCTIONAL(heap != NULL, "heap must not be NULL", return HEAP_STATUS_NULL_ARG);

    heap_status_t status = reserve_root_slots(heap, required_root_slots(heap->size + 1U));
    RETURN_IF_ERROR(status != HEAP_STATUS_OK,
                    status,
                    "binomial_heap: reserve_root_slots failed with status=%d", (int)status);

    binomial_heap_node_t *node = (binomial_heap_node_t *)calloc(1U, sizeof(*node));
    RETURN_IF_ERROR(node == NULL,
                    HEAP_STATUS_ALLOC_FAIL,
                    "binomial_heap: cannot allocate node");

    node->key = value;
    status = absorb_tree(heap, node);
    RETURN_IF_ERROR_CLEANUP(status != HEAP_STATUS_OK,
                            status,
                            free(node),
                            "binomial_heap: absorb_tree failed with status=%d", (int)status);

    heap->size++;
    return HEAP_STATUS_OK;
}

heap_status_t binomial_heap_build_inserts(binomial_heap_t *heap, const int *values, size_t count) {
    SOFT_ASSERT_FUNCTIONAL(heap != NULL, "heap must not be NULL", return HEAP_STATUS_NULL_ARG);
    SOFT_ASSERT_FUNCTIONAL(values != NULL || count == 0U,
                           "values must not be NULL when count > 0",
                           return HEAP_STATUS_NULL_ARG);

    heap_status_t status = binomial_heap_init(heap);
    RETURN_IF_ERROR(status != HEAP_STATUS_OK,
                    status,
                    "binomial_heap: init before build failed with status=%d", (int)status);

    status = reserve_root_slots(heap, required_root_slots(count));
    RETURN_IF_ERROR_CLEANUP(status != HEAP_STATUS_OK,
                            status,
                            binomial_heap_destroy(heap),
                            "binomial_heap: reserve_root_slots during build failed with status=%d", (int)status);

    for (size_t index = 0U; index < count; ++index) {
        status = binomial_heap_insert(heap, values[index]);
        RETURN_IF_ERROR_CLEANUP(status != HEAP_STATUS_OK,
                                status,
                                binomial_heap_destroy(heap),
                                "binomial_heap: insert failed at index=%zu with status=%d",
                                index, (int)status);
    }

    return HEAP_STATUS_OK;
}

heap_status_t binomial_heap_extract_min(binomial_heap_t *heap, int *out_value) {
    SOFT_ASSERT_FUNCTIONAL(heap != NULL && out_value != NULL,
                           "heap and out_value must not be NULL",
                           return HEAP_STATUS_NULL_ARG);
    RETURN_IF_ERROR(heap->size == 0U,
                    HEAP_STATUS_EMPTY,
                    "binomial_heap: extract_min called on empty heap");

    size_t best_degree = 0U;
    binomial_heap_node_t *best_root = NULL;

    for (size_t degree = 0U; degree < heap->roots.size; ++degree) {
        binomial_heap_node_t **slot = root_slot(heap, degree);
        if (*slot == NULL) {
            continue;
        }
        if (best_root == NULL || (*slot)->key < best_root->key) {
            best_root = *slot;
            best_degree = degree;
        }
    }

    RETURN_IF_ERROR(best_root == NULL,
                    HEAP_STATUS_INTERNAL,
                    "binomial_heap: no root found in non-empty heap");

    *root_slot(heap, best_degree) = NULL;
    *out_value = best_root->key;
    heap->size--;

    binomial_heap_node_t *child = best_root->child;
    while (child != NULL) {
        binomial_heap_node_t *next = child->sibling;
        child->parent  = NULL;
        child->sibling = NULL;

        heap_status_t status = absorb_tree(heap, child);
        RETURN_IF_ERROR_CLEANUP(status != HEAP_STATUS_OK,
                                status,
                                destroy_tree(next); free(best_root),
                                "binomial_heap: absorb_tree while extracting min failed with status=%d",
                                (int)status);

        child = next;
    }

    free(best_root);
    return HEAP_STATUS_OK;
}

bool binomial_heap_is_valid(const binomial_heap_t *heap) {
    SOFT_ASSERT_FUNCTIONAL(heap != NULL, "heap must not be NULL", return false);

    size_t total_nodes = 0U;
    for (size_t degree = 0U; degree < heap->roots.size; ++degree) {
        const binomial_heap_node_t *const *slot = root_slot_const(heap, degree);
        if (slot == NULL || *slot == NULL) continue;
        if ((*slot)->degree != degree) return false;

        size_t subtree_nodes = 0U;
        if (!validate_tree(*slot, &subtree_nodes)) {
            return false;
        }
        total_nodes += subtree_nodes;
    }

    return total_nodes == heap->size;
}

heap_status_t binomial_heap_benchmark_inserts(int *work_arr, size_t n, double *build_seconds, int *sorted_out) {
    SOFT_ASSERT_FUNCTIONAL(build_seconds != NULL,
                           "build_seconds must not be NULL",
                           return HEAP_STATUS_NULL_ARG);
    SOFT_ASSERT_FUNCTIONAL((work_arr != NULL && sorted_out != NULL) || n == 0U,
                           "work_arr and sorted_out must not be NULL when n > 0",
                           return HEAP_STATUS_NULL_ARG);

    binomial_heap_t heap = {0};

    clock_t start = clock();
    heap_status_t status = binomial_heap_build_inserts(&heap, work_arr, n);
    clock_t end = clock();
    *build_seconds = (double)(end - start) / (double)CLOCKS_PER_SEC;

    RETURN_IF_ERROR_CLEANUP(status != HEAP_STATUS_OK,
                            status,
                            binomial_heap_destroy(&heap),
                            "binomial_heap: build failed with status=%d", (int)status);
    RETURN_IF_ERROR_CLEANUP(!binomial_heap_is_valid(&heap),
                            HEAP_STATUS_INTERNAL,
                            binomial_heap_destroy(&heap),
                            "binomial_heap: heap invariant check failed after build");

    for (size_t index = 0U; index < n; ++index) {
        status = binomial_heap_extract_min(&heap, &sorted_out[index]);
        RETURN_IF_ERROR_CLEANUP(status != HEAP_STATUS_OK,
                                status,
                                binomial_heap_destroy(&heap),
                                "binomial_heap: extract_min failed at index=%zu with status=%d",
                                index, (int)status);
    }

    binomial_heap_destroy(&heap);
    return HEAP_STATUS_OK;
}
