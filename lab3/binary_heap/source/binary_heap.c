#include "binary_heap.h"

#include <stddef.h>
#include <time.h>

#include "asserts.h"
#include "return_macros.h"

typedef heap_status_t (*binary_build_fn)(binary_heap_t *heap, int *buffer, size_t count);

static void binary_heap_attach_buffer(binary_heap_t *heap, int *buffer, size_t size, size_t capacity) {
    HARD_ASSERT(heap != NULL, "heap is NULL");
    HARD_ASSERT(buffer != NULL || capacity == 0U, "buffer is NULL");

    heap->storage.data      = buffer;
    heap->storage.size      = size;
    heap->storage.capacity  = capacity;
    heap->storage.elem_size = sizeof(int);
    heap->storage.is_static = true;
}

static int *binary_heap_data(binary_heap_t *heap) {
    HARD_ASSERT(heap != NULL, "heap is NULL");
    return (int *)heap->storage.data;
}

static const int *binary_heap_data_const(const binary_heap_t *heap) {
    HARD_ASSERT(heap != NULL, "heap is NULL");
    return (const int *)heap->storage.data;
}

static void swap_int(int *lhs, int *rhs) {
    int temp = *lhs;
    *lhs = *rhs;
    *rhs = temp;
}

static void binary_heap_sift_up(int *data, size_t index) {
    HARD_ASSERT(data != NULL || index == 0U, "data is NULL");

    while (index > 0U) {
        size_t parent = (index - 1U) / 2U;
        if (data[parent] <= data[index]) {
            break;
        }
        swap_int(&data[parent], &data[index]);
        index = parent;
    }
}

static void binary_heap_sift_down(int *data, size_t count, size_t index) {
    HARD_ASSERT(data != NULL || count == 0U, "data is NULL");

    while (true) {
        size_t left = index * 2U + 1U;
        if (left >= count) return;

        size_t right = left + 1U;
        size_t smallest = left;
        if (right < count && data[right] < data[left]) {
            smallest = right;
        }

        if (data[index] <= data[smallest]) return;

        swap_int(&data[index], &data[smallest]);
        index = smallest;
    }
}

static heap_status_t binary_heap_benchmark_impl(int *work_arr, size_t n, double *build_seconds,
                                                int *sorted_out, binary_build_fn build_fn) {
    SOFT_ASSERT_FUNCTIONAL(build_seconds != NULL && build_fn != NULL,
                           "build_seconds and build_fn must not be NULL",
                           return HEAP_STATUS_NULL_ARG);
    SOFT_ASSERT_FUNCTIONAL((work_arr != NULL && sorted_out != NULL) || n == 0U,
                           "work_arr and sorted_out must not be NULL when n > 0",
                           return HEAP_STATUS_NULL_ARG);

    binary_heap_t heap = {0};

    clock_t start = clock();
    heap_status_t status = build_fn(&heap, work_arr, n);
    clock_t end = clock();
    *build_seconds = (double)(end - start) / (double)CLOCKS_PER_SEC;

    RETURN_IF_ERROR_CLEANUP(status != HEAP_STATUS_OK,
                            status,
                            binary_heap_reset(&heap),
                            "binary_heap: build function failed with status=%d", (int)status);
    RETURN_IF_ERROR_CLEANUP(!binary_heap_is_valid(&heap),
                            HEAP_STATUS_INTERNAL,
                            binary_heap_reset(&heap),
                            "binary_heap: heap invariant check failed after build");

    for (size_t i = 0U; i < n; ++i) {
        status = binary_heap_extract_min(&heap, &sorted_out[i]);
        RETURN_IF_ERROR_CLEANUP(status != HEAP_STATUS_OK,
                                status,
                                binary_heap_reset(&heap),
                                "binary_heap: extract_min failed at index=%zu with status=%d",
                                i, (int)status);
    }

    binary_heap_reset(&heap);
    return HEAP_STATUS_OK;
}

void binary_heap_reset(binary_heap_t *heap) {
    SOFT_ASSERT_FUNCTIONAL(heap != NULL, "heap must not be NULL", return);
    (void)vector_destroy(&heap->storage);
}

heap_status_t binary_heap_build_linear(binary_heap_t *heap, int *buffer, size_t count) {
    SOFT_ASSERT_FUNCTIONAL(heap != NULL, "heap must not be NULL", return HEAP_STATUS_NULL_ARG);
    SOFT_ASSERT_FUNCTIONAL(buffer != NULL || count == 0U,
                           "buffer must not be NULL when count > 0",
                           return HEAP_STATUS_NULL_ARG);

    binary_heap_reset(heap);
    binary_heap_attach_buffer(heap, buffer, count, count);

    if (count < 2U) return HEAP_STATUS_OK;

    for (size_t index = count / 2U; index > 0U; --index) {
        binary_heap_sift_down(binary_heap_data(heap), count, index - 1U);
    }
    return HEAP_STATUS_OK;
}

heap_status_t binary_heap_build_inserts(binary_heap_t *heap, int *buffer, size_t count) {
    SOFT_ASSERT_FUNCTIONAL(heap != NULL, "heap must not be NULL", return HEAP_STATUS_NULL_ARG);
    SOFT_ASSERT_FUNCTIONAL(buffer != NULL || count == 0U,
                           "buffer must not be NULL when count > 0",
                           return HEAP_STATUS_NULL_ARG);

    binary_heap_reset(heap);
    binary_heap_attach_buffer(heap, buffer, 0U, count);

    for (size_t index = 0U; index < count; ++index) {
        heap->storage.size++;
        binary_heap_sift_up(binary_heap_data(heap), index);
    }
    return HEAP_STATUS_OK;
}

heap_status_t binary_heap_extract_min(binary_heap_t *heap, int *out_value) {
    SOFT_ASSERT_FUNCTIONAL(heap != NULL && out_value != NULL,
                           "heap and out_value must not be NULL",
                           return HEAP_STATUS_NULL_ARG);
    RETURN_IF_ERROR(heap->storage.size == 0U,
                    HEAP_STATUS_EMPTY,
                    "binary_heap: extract_min called on empty heap");

    int *data = binary_heap_data(heap);
    *out_value = data[0];

    if (heap->storage.size == 1U) {
        heap->storage.size = 0U;
        return HEAP_STATUS_OK;
    }

    data[0] = data[heap->storage.size - 1U];
    heap->storage.size--;
    binary_heap_sift_down(data, heap->storage.size, 0U);
    return HEAP_STATUS_OK;
}

bool binary_heap_is_valid(const binary_heap_t *heap) {
    SOFT_ASSERT_FUNCTIONAL(heap != NULL, "heap must not be NULL", return false);

    const int *data = binary_heap_data_const(heap);
    for (size_t index = 0U; index < heap->storage.size; ++index) {
        size_t left = index * 2U + 1U;
        size_t right = left + 1U;

        if (left < heap->storage.size && data[index] > data[left]) {
            return false;
        }
        if (right < heap->storage.size && data[index] > data[right]) {
            return false;
        }
    }

    return true;
}

heap_status_t binary_heap_benchmark_linear(int *work_arr, size_t n, double *build_seconds, int *sorted_out) {
    return binary_heap_benchmark_impl(work_arr, n, build_seconds, sorted_out, binary_heap_build_linear);
}

heap_status_t binary_heap_benchmark_inserts(int *work_arr, size_t n, double *build_seconds, int *sorted_out) {
    return binary_heap_benchmark_impl(work_arr, n, build_seconds, sorted_out, binary_heap_build_inserts);
}
