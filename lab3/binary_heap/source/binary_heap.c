#include "binary_heap.h"

#include <stddef.h>
#include <time.h>

#include "asserts.h"

typedef heap_status_t (*binary_build_fn)(binary_heap_t *heap, int *buffer, size_t count);

static void binary_heap_attach_buffer(binary_heap_t *heap, int *buffer, size_t size, size_t capacity) {
    HARD_ASSERT(heap != NULL, "heap is NULL");
    HARD_ASSERT(buffer != NULL || capacity == 0U, "buffer is NULL");

    heap->storage.data = buffer;
    heap->storage.size = size;
    heap->storage.capacity = capacity;
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
        if (left >= count) {
            return;
        }

        size_t right = left + 1U;
        size_t smallest = left;
        if (right < count && data[right] < data[left]) {
            smallest = right;
        }

        if (data[index] <= data[smallest]) {
            return;
        }

        swap_int(&data[index], &data[smallest]);
        index = smallest;
    }
}

static double monotonic_seconds(void) {
    struct timespec time_spec = {0};
    clock_gettime(CLOCK_MONOTONIC, &time_spec);
    return (double)time_spec.tv_sec + (double)time_spec.tv_nsec / 1000000000.0;
}

static heap_status_t binary_heap_benchmark_impl(int *work_arr, size_t n, double *build_seconds,
                                                int *sorted_out, binary_build_fn build_fn) {
    if (build_seconds == NULL || build_fn == NULL) {
        return HEAP_STATUS_NULL_ARG;
    }
    if ((work_arr == NULL || sorted_out == NULL) && n != 0U) {
        return HEAP_STATUS_NULL_ARG;
    }

    binary_heap_t heap = {0};

    double start = monotonic_seconds();
    heap_status_t status = build_fn(&heap, work_arr, n);
    double end = monotonic_seconds();
    *build_seconds = end - start;

    if (status != HEAP_STATUS_OK) {
        binary_heap_reset(&heap);
        return status;
    }
    if (!binary_heap_is_valid(&heap)) {
        binary_heap_reset(&heap);
        return HEAP_STATUS_INTERNAL;
    }

    for (size_t i = 0U; i < n; ++i) {
        status = binary_heap_extract_min(&heap, &sorted_out[i]);
        if (status != HEAP_STATUS_OK) {
            binary_heap_reset(&heap);
            return status;
        }
    }

    binary_heap_reset(&heap);
    return HEAP_STATUS_OK;
}

void binary_heap_reset(binary_heap_t *heap) {
    if (heap == NULL) {
        return;
    }
    (void)vector_destroy(&heap->storage);
}

heap_status_t binary_heap_build_linear(binary_heap_t *heap, int *buffer, size_t count) {
    if (heap == NULL) {
        return HEAP_STATUS_NULL_ARG;
    }
    if (buffer == NULL && count != 0U) {
        return HEAP_STATUS_NULL_ARG;
    }

    binary_heap_reset(heap);
    binary_heap_attach_buffer(heap, buffer, count, count);

    if (count < 2U) {
        return HEAP_STATUS_OK;
    }

    for (size_t index = count / 2U; index > 0U; --index) {
        binary_heap_sift_down(binary_heap_data(heap), count, index - 1U);
    }
    return HEAP_STATUS_OK;
}

heap_status_t binary_heap_build_inserts(binary_heap_t *heap, int *buffer, size_t count) {
    if (heap == NULL) {
        return HEAP_STATUS_NULL_ARG;
    }
    if (buffer == NULL && count != 0U) {
        return HEAP_STATUS_NULL_ARG;
    }

    binary_heap_reset(heap);
    binary_heap_attach_buffer(heap, buffer, 0U, count);

    for (size_t index = 0U; index < count; ++index) {
        heap->storage.size++;
        binary_heap_sift_up(binary_heap_data(heap), index);
    }
    return HEAP_STATUS_OK;
}

heap_status_t binary_heap_extract_min(binary_heap_t *heap, int *out_value) {
    if (heap == NULL || out_value == NULL) {
        return HEAP_STATUS_NULL_ARG;
    }
    if (heap->storage.size == 0U) {
        return HEAP_STATUS_EMPTY;
    }

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
    if (heap == NULL) {
        return false;
    }

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
