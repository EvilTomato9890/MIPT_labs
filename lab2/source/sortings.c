#include "sortings.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "asserts.h"
#include "logger.h"
#include "return_macros.h"

#define SHELL_KNUTH_BASE_GAP  1U
#define SHELL_KNUTH_FACTOR    3U
#define SHELL_KNUTH_INCREMENT 1U

#define MIN_HEAP_BRANCHING_FACTOR 2U
#define KARY_FIRST_CHILD_OFFSET   1U

#define RADIX_BUCKETS     256U
#define RADIX_COUNT_SIZE (RADIX_BUCKETS + 1U)
#define SIGN_BIT_MASK     0x80000000U
#define BYTE_MASK         0xFFU
#define BITS_PER_BYTE     8U

#define TIMSORT_MINRUN_THRESHOLD 64U
#define TIMSORT_STACK_CAPACITY   128U

#define PDQSORT_INSERTION_THRESHOLD       24U
#define PDQSORT_FALLBACK_HEAP_K           4U
#define PDQSORT_BAD_PARTITION_NUMERATOR   8U
#define PDQSORT_BAD_PARTITION_DENOMINATOR 7U
#define PDQSORT_DEPTH_COEF                2.0
#define PDQSORT_EXTRA_BAD_PARTITIONS      4U

typedef struct range_type {
    ptrdiff_t left;
    ptrdiff_t right;
} range_type;

typedef struct run_type {
    size_t left;
    size_t right;
} run_type;

static void report_sorting_failure(const char *function_name, sorting_status_t status) {
    LOGGER_ERROR("%s failed with status=%d", function_name, (int)status);
    SOFT_ASSERT_FUNCTIONAL(0, "Sorting function failed", return);
}

static void swap_int(int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

static size_t parent_kary(size_t idx, size_t k) {
    return (idx - 1U) / k;
}

static size_t max_child_kary(const int *arr, size_t n, size_t k, size_t root) {
    size_t first = root * k + KARY_FIRST_CHILD_OFFSET;
    size_t best = first;
    size_t last = first + k;
    if (last > n) {
        last = n;
    }

    for (size_t child = first + 1U; child < last; ++child) {
        if (arr[child] > arr[best]) {
            best = child;
        }
    }
    return best;
}

static void sift_down_kary_bottom_up(int *arr, size_t n, size_t k, size_t root) {
    int key = arr[root];
    size_t hole = root;

    while (1) {
        size_t first = hole * k + KARY_FIRST_CHILD_OFFSET;
        if (first >= n) {
            break;
        }
        size_t best = max_child_kary(arr, n, k, hole);
        arr[hole] = arr[best];
        hole = best;
    }

    while (hole > root) {
        size_t parent = parent_kary(hole, k);
        if (arr[parent] >= key) {
            break;
        }
        arr[hole] = arr[parent];
        hole = parent;
    }
    arr[hole] = key;
}

static void merge_two(const int *src, int *dst, size_t left, size_t mid, size_t right) {
    size_t i = left;
    size_t j = mid;
    size_t p = left;

    while (i < mid && j < right) {
        dst[p++] = (src[i] <= src[j]) ? src[i++] : src[j++];
    }
    while (i < mid) {
        dst[p++] = src[i++];
    }
    while (j < right) {
        dst[p++] = src[j++];
    }
}

static void merge_recursive_impl(int *arr, int *buf, size_t left, size_t right) {
    if (right - left < 2U) {
        return;
    }

    size_t mid = left + (right - left) / 2U;
    merge_recursive_impl(arr, buf, left, mid);
    merge_recursive_impl(arr, buf, mid, right);
    merge_two(arr, buf, left, mid, right);
    memcpy(arr + left, buf + left, (right - left) * sizeof(arr[0]));
}

static ptrdiff_t pick_pivot(int *arr, ptrdiff_t left, ptrdiff_t right, pivot_strategy_type strategy) {
    ptrdiff_t center = left + (right - left) / 2;
    if (strategy == PIVOT_CENTER) {
        return center;
    }
    if (strategy == PIVOT_RANDOM) {
        return left + (ptrdiff_t)(rand() % (int)(right - left + 1));
    }

    ptrdiff_t a = left;
    ptrdiff_t b = center;
    ptrdiff_t c = right;
    if (strategy == PIVOT_MEDIAN3_RANDOM) {
        a = left + (ptrdiff_t)(rand() % (int)(right - left + 1));
        b = left + (ptrdiff_t)(rand() % (int)(right - left + 1));
        c = left + (ptrdiff_t)(rand() % (int)(right - left + 1));
    }

    int x = arr[a];
    int y = arr[b];
    int z = arr[c];
    if ((x <= y && y <= z) || (z <= y && y <= x)) {
        return b;
    }
    if ((y <= x && x <= z) || (z <= x && x <= y)) {
        return a;
    }
    return c;
}

static ptrdiff_t partition_lomuto(int *arr, ptrdiff_t left, ptrdiff_t right, pivot_strategy_type strategy) {
    ptrdiff_t pivot_index = pick_pivot(arr, left, right, strategy);
    int pivot_value = arr[pivot_index];
    swap_int(&arr[pivot_index], &arr[right]);

    ptrdiff_t i = left;
    for (ptrdiff_t j = left; j < right; ++j) {
        if (arr[j] <= pivot_value) {
            swap_int(&arr[i], &arr[j]);
            ++i;
        }
    }
    swap_int(&arr[i], &arr[right]);
    return i;
}

static ptrdiff_t partition_hoare(int *arr, ptrdiff_t left, ptrdiff_t right, pivot_strategy_type strategy) {
    int pivot = arr[pick_pivot(arr, left, right, strategy)];
    ptrdiff_t i = left - 1;
    ptrdiff_t j = right + 1;

    while (1) {
        do {
            ++i;
        } while (arr[i] < pivot);

        do {
            --j;
        } while (arr[j] > pivot);

        if (i >= j) {
            return j;
        }
        swap_int(&arr[i], &arr[j]);
    }
}

static range_type partition_three_way(int *arr, ptrdiff_t left, ptrdiff_t right, pivot_strategy_type strategy) {
    int pivot = arr[pick_pivot(arr, left, right, strategy)];
    ptrdiff_t lt = left;
    ptrdiff_t i = left;
    ptrdiff_t gt = right;

    while (i <= gt) {
        if (arr[i] < pivot) {
            swap_int(&arr[lt++], &arr[i++]);
        } else if (arr[i] > pivot) {
            swap_int(&arr[i], &arr[gt--]);
        } else {
            ++i;
        }
    }

    range_type range = {lt, gt};
    return range;
}

static void quick_lomuto_impl(int *arr, ptrdiff_t left, ptrdiff_t right, pivot_strategy_type strategy) {
    while (left < right) {
        ptrdiff_t pivot = partition_lomuto(arr, left, right, strategy);
        if (pivot - left < right - pivot) {
            quick_lomuto_impl(arr, left, pivot - 1, strategy);
            left = pivot + 1;
        } else {
            quick_lomuto_impl(arr, pivot + 1, right, strategy);
            right = pivot - 1;
        }
    }
}

static void quick_hoare_impl(int *arr, ptrdiff_t left, ptrdiff_t right, pivot_strategy_type strategy) {
    while (left < right) {
        ptrdiff_t pivot = partition_hoare(arr, left, right, strategy);
        if (pivot - left < right - pivot) {
            quick_hoare_impl(arr, left, pivot, strategy);
            left = pivot + 1;
        } else {
            quick_hoare_impl(arr, pivot + 1, right, strategy);
            right = pivot;
        }
    }
}

static void quick_three_way_impl(int *arr, ptrdiff_t left, ptrdiff_t right, pivot_strategy_type strategy) {
    while (left < right) {
        range_type range = partition_three_way(arr, left, right, strategy);
        if ((range.left - left) < (right - range.right)) {
            quick_three_way_impl(arr, left, range.left - 1, strategy);
            left = range.right + 1;
        } else {
            quick_three_way_impl(arr, range.right + 1, right, strategy);
            right = range.left - 1;
        }
    }
}

static size_t depth_limit(size_t n, double coef) {
    if (n < 2U) {
        return 0U;
    }

    double value = coef * log2((double)n);
    return (size_t)((value < 1.0) ? 1.0 : value);
}

static size_t min_run_value(size_t n) {
    size_t remainder = 0U;
    while (n >= TIMSORT_MINRUN_THRESHOLD) {
        remainder |= n & 1U;
        n >>= 1U;
    }
    return n + remainder;
}

static void reverse_part(int *arr, size_t left, size_t right) {
    while (left < right) {
        swap_int(&arr[left], &arr[right]);
        ++left;
        --right;
    }
}

static size_t find_run(int *arr, size_t n, size_t start) {
    if (start + 1U >= n) {
        return n;
    }

    size_t i = start + 1U;
    if (arr[i] < arr[i - 1U]) {
        while (i < n && arr[i] < arr[i - 1U]) {
            ++i;
        }
        reverse_part(arr, start, i - 1U);
    } else {
        while (i < n && arr[i] >= arr[i - 1U]) {
            ++i;
        }
    }
    return i;
}

static void merge_inplace_buffer(int *arr, int *buf, size_t left, size_t mid, size_t right) {
    size_t left_len = mid - left;
    memcpy(buf, arr + left, left_len * sizeof(arr[0]));

    size_t i = 0U;
    size_t j = mid;
    size_t pos = left;
    while (i < left_len && j < right) {
        arr[pos++] = (buf[i] <= arr[j]) ? buf[i++] : arr[j++];
    }
    while (i < left_len) {
        arr[pos++] = buf[i++];
    }
}

static void merge_at(int *arr, int *buf, run_type *stack, size_t idx) {
    size_t left = stack[idx].left;
    size_t mid = stack[idx].right;
    size_t right = stack[idx + 1U].right;
    merge_inplace_buffer(arr, buf, left, mid, right);
    stack[idx].right = right;
}

sorting_status_t insertion_sort_checked(int *arr, size_t n) {
    SOFT_ASSERT_FUNCTIONAL(arr != NULL || n == 0U, "Array pointer must not be NULL",
                           return SORTING_STATUS_NULL_ARG);

    for (size_t i = 1U; i < n; ++i) {
        int key = arr[i];
        size_t j = i;
        while (j > 0U && arr[j - 1U] > key) {
            arr[j] = arr[j - 1U];
            --j;
        }
        arr[j] = key;
    }
    return SORTING_STATUS_OK;
}

sorting_status_t bubble_sort_checked(int *arr, size_t n) {
    SOFT_ASSERT_FUNCTIONAL(arr != NULL || n == 0U, "Array pointer must not be NULL",
                           return SORTING_STATUS_NULL_ARG);

    for (size_t i = 0U; i < n; ++i) {
        int swapped = 0;
        for (size_t j = 1U; j < n - i; ++j) {
            if (arr[j - 1U] > arr[j]) {
                swap_int(&arr[j - 1U], &arr[j]);
                swapped = 1;
            }
        }
        if (!swapped) {
            break;
        }
    }
    return SORTING_STATUS_OK;
}

sorting_status_t selection_sort_checked(int *arr, size_t n) {
    SOFT_ASSERT_FUNCTIONAL(arr != NULL || n == 0U, "Array pointer must not be NULL",
                           return SORTING_STATUS_NULL_ARG);

    for (size_t i = 0U; i < n; ++i) {
        size_t min_idx = i;
        for (size_t j = i + 1U; j < n; ++j) {
            if (arr[j] < arr[min_idx]) {
                min_idx = j;
            }
        }
        if (min_idx != i) {
            swap_int(&arr[i], &arr[min_idx]);
        }
    }
    return SORTING_STATUS_OK;
}

sorting_status_t shell_knuth_sort_checked(int *arr, size_t n) {
    SOFT_ASSERT_FUNCTIONAL(arr != NULL || n == 0U, "Array pointer must not be NULL",
                           return SORTING_STATUS_NULL_ARG);

    size_t gap = SHELL_KNUTH_BASE_GAP;
    while (gap < n / SHELL_KNUTH_FACTOR) {
        gap = gap * SHELL_KNUTH_FACTOR + SHELL_KNUTH_INCREMENT;
    }

    while (gap > 0U) {
        for (size_t i = gap; i < n; ++i) {
            int key = arr[i];
            size_t j = i;
            while (j >= gap && arr[j - gap] > key) {
                arr[j] = arr[j - gap];
                j -= gap;
            }
            arr[j] = key;
        }
        gap = (gap - SHELL_KNUTH_INCREMENT) / SHELL_KNUTH_FACTOR;
    }
    return SORTING_STATUS_OK;
}

sorting_status_t heap_kary_sort_checked(int *arr, size_t n, size_t k) {
    SOFT_ASSERT_FUNCTIONAL(arr != NULL || n == 0U, "Array pointer must not be NULL",
                           return SORTING_STATUS_NULL_ARG);
    SOFT_ASSERT_FUNCTIONAL(k >= MIN_HEAP_BRANCHING_FACTOR, "Heap branching factor must be at least 2",
                           return SORTING_STATUS_INVALID_ARG);

    if (n < 2U) {
        return SORTING_STATUS_OK;
    }

    size_t last_parent = (n - 2U) / k;
    for (size_t i = last_parent + 1U; i > 0U; --i) {
        sift_down_kary_bottom_up(arr, n, k, i - 1U);
    }

    for (size_t end = n; end > 1U; --end) {
        swap_int(&arr[0], &arr[end - 1U]);
        sift_down_kary_bottom_up(arr, end - 1U, k, 0U);
    }
    return SORTING_STATUS_OK;
}

sorting_status_t merge_recursive_sort_checked(int *arr, size_t n) {
    SOFT_ASSERT_FUNCTIONAL(arr != NULL || n == 0U, "Array pointer must not be NULL",
                           return SORTING_STATUS_NULL_ARG);

    if (n < 2U) {
        return SORTING_STATUS_OK;
    }

    int *buf = calloc(n, sizeof(buf[0]));
    RETURN_IF_ERROR(buf == NULL, SORTING_STATUS_ALLOC_FAIL,
                    "merge_recursive_sort: no memory for %zu integers", n);

    merge_recursive_impl(arr, buf, 0U, n);
    free(buf);
    return SORTING_STATUS_OK;
}

sorting_status_t merge_iterative_sort_checked(int *arr, size_t n) {
    SOFT_ASSERT_FUNCTIONAL(arr != NULL || n == 0U, "Array pointer must not be NULL",
                           return SORTING_STATUS_NULL_ARG);

    if (n < 2U) {
        return SORTING_STATUS_OK;
    }

    int *buf = calloc(n, sizeof(buf[0]));
    RETURN_IF_ERROR(buf == NULL, SORTING_STATUS_ALLOC_FAIL,
                    "merge_iterative_sort: no memory for %zu integers", n);

    int *src = arr;
    int *dst = buf;
    for (size_t width = 1U; width < n; width <<= 1U) {
        for (size_t left = 0U; left < n; left += (width << 1U)) {
            size_t mid = left + width;
            size_t right = left + (width << 1U);
            if (mid > n) {
                mid = n;
            }
            if (right > n) {
                right = n;
            }
            merge_two(src, dst, left, mid, right);
        }

        int *tmp = src;
        src = dst;
        dst = tmp;
    }

    if (src != arr) {
        memcpy(arr, src, n * sizeof(arr[0]));
    }
    free(buf);
    return SORTING_STATUS_OK;
}

sorting_status_t quick_lomuto_sort_checked(int *arr, size_t n) {
    SOFT_ASSERT_FUNCTIONAL(arr != NULL || n == 0U, "Array pointer must not be NULL",
                           return SORTING_STATUS_NULL_ARG);

    if (n > 1U) {
        quick_lomuto_impl(arr, 0, (ptrdiff_t)n - 1, PIVOT_CENTER);
    }
    return SORTING_STATUS_OK;
}

sorting_status_t quick_hoare_sort_checked(int *arr, size_t n) {
    SOFT_ASSERT_FUNCTIONAL(arr != NULL || n == 0U, "Array pointer must not be NULL",
                           return SORTING_STATUS_NULL_ARG);

    if (n > 1U) {
        quick_hoare_impl(arr, 0, (ptrdiff_t)n - 1, PIVOT_CENTER);
    }
    return SORTING_STATUS_OK;
}

sorting_status_t quick_three_way_sort_checked(int *arr, size_t n) {
    SOFT_ASSERT_FUNCTIONAL(arr != NULL || n == 0U, "Array pointer must not be NULL",
                           return SORTING_STATUS_NULL_ARG);

    if (n > 1U) {
        quick_three_way_impl(arr, 0, (ptrdiff_t)n - 1, PIVOT_CENTER);
    }
    return SORTING_STATUS_OK;
}

sorting_status_t quick_best_sort_checked(int *arr, size_t n, pivot_strategy_type strategy) {
    SOFT_ASSERT_FUNCTIONAL(arr != NULL || n == 0U, "Array pointer must not be NULL",
                           return SORTING_STATUS_NULL_ARG);

    if (n > 1U) {
        quick_three_way_impl(arr, 0, (ptrdiff_t)n - 1, strategy);
    }
    return SORTING_STATUS_OK;
}

static sorting_status_t introsort_impl(int *arr, ptrdiff_t left, ptrdiff_t right,
                                       size_t threshold, size_t heap_k, size_t depth,
                                       pivot_strategy_type pivot_strategy, sorting_fn small_sort) {
    while (left < right) {
        size_t len = (size_t)(right - left + 1);
        if (len <= threshold) {
            small_sort(arr + left, len);
            return SORTING_STATUS_OK;
        }
        if (depth == 0U) {
            return heap_kary_sort_checked(arr + left, len, heap_k);
        }

        range_type range = partition_three_way(arr, left, right, pivot_strategy);
        --depth;
        sorting_status_t status = introsort_impl(arr, left, range.left - 1, threshold, heap_k, depth,
                                                 pivot_strategy, small_sort);
        if (status != SORTING_STATUS_OK) {
            return status;
        }
        left = range.right + 1;
    }
    return SORTING_STATUS_OK;
}

sorting_status_t introsort_checked(int *arr, size_t n, size_t threshold, size_t heap_k, double depth_coef) {
    return introsort_config_sort_checked(arr, n, threshold, heap_k, depth_coef,
                                         PIVOT_MEDIAN3, insertion_sort);
}

sorting_status_t introsort_config_sort_checked(int *arr, size_t n, size_t threshold, size_t heap_k,
                                               double depth_coef, pivot_strategy_type pivot_strategy,
                                               sorting_fn small_sort) {
    SOFT_ASSERT_FUNCTIONAL(arr != NULL || n == 0U, "Array pointer must not be NULL",
                           return SORTING_STATUS_NULL_ARG);
    SOFT_ASSERT_FUNCTIONAL(threshold >= 2U, "Introsort threshold must be at least 2",
                           return SORTING_STATUS_INVALID_ARG);
    SOFT_ASSERT_FUNCTIONAL(heap_k >= 2U, "Heap branching factor must be at least 2",
                           return SORTING_STATUS_INVALID_ARG);
    SOFT_ASSERT_FUNCTIONAL(depth_coef > 0.0, "Introsort depth coefficient must be positive",
                           return SORTING_STATUS_INVALID_ARG);
    SOFT_ASSERT_FUNCTIONAL(small_sort != NULL, "Small_sort function must not be NULL",
                           return SORTING_STATUS_NULL_ARG);

    if (n > 1U) {
        return introsort_impl(arr, 0, (ptrdiff_t)n - 1,
                              threshold, heap_k, depth_limit(n, depth_coef),
                              pivot_strategy, small_sort);
    }
    return SORTING_STATUS_OK;
}

sorting_status_t lsd_radix_sort_checked(int *arr, size_t n) {
    SOFT_ASSERT_FUNCTIONAL(arr != NULL || n == 0U, "Array pointer must not be NULL",
                           return SORTING_STATUS_NULL_ARG);

    if (n < 2U) {
        return SORTING_STATUS_OK;
    }

    int *buf = calloc(n, sizeof(buf[0]));
    RETURN_IF_ERROR(buf == NULL, SORTING_STATUS_ALLOC_FAIL,
                    "lsd_radix_sort: no memory for %zu integers", n);

    for (size_t byte = 0U; byte < sizeof(int); ++byte) {
        size_t count[RADIX_BUCKETS] = {0};
        for (size_t i = 0U; i < n; ++i) {
            uint32_t value = (uint32_t)arr[i] ^ SIGN_BIT_MASK;
            size_t bucket = (size_t)((value >> (byte * BITS_PER_BYTE)) & BYTE_MASK);
            ++count[bucket];
        }

        size_t sum = 0U;
        for (size_t i = 0U; i < RADIX_BUCKETS; ++i) {
            size_t current = count[i];
            count[i] = sum;
            sum += current;
        }

        for (size_t i = 0U; i < n; ++i) {
            uint32_t value = (uint32_t)arr[i] ^ SIGN_BIT_MASK;
            size_t bucket = (size_t)((value >> (byte * BITS_PER_BYTE)) & BYTE_MASK);
            buf[count[bucket]++] = arr[i];
        }
        memcpy(arr, buf, n * sizeof(arr[0]));
    }

    free(buf);
    return SORTING_STATUS_OK;
}

static void msd_byte_sort(int *arr, int *buf, size_t n, size_t byte) {
    if (n < 2U || byte >= sizeof(int)) {
        return;
    }

    size_t count[RADIX_COUNT_SIZE] = {0};
    for (size_t i = 0U; i < n; ++i) {
        uint32_t value = (uint32_t)arr[i] ^ SIGN_BIT_MASK;
        size_t bucket = (size_t)((value >> ((sizeof(int) - byte - 1U) * BITS_PER_BYTE)) & BYTE_MASK);
        ++count[bucket + 1U];
    }

    for (size_t i = 1U; i < RADIX_COUNT_SIZE; ++i) {
        count[i] += count[i - 1U];
    }

    size_t begin[RADIX_COUNT_SIZE] = {0};
    memcpy(begin, count, sizeof(begin));
    for (size_t i = 0U; i < n; ++i) {
        uint32_t value = (uint32_t)arr[i] ^ SIGN_BIT_MASK;
        size_t bucket = (size_t)((value >> ((sizeof(int) - byte - 1U) * BITS_PER_BYTE)) & BYTE_MASK);
        buf[count[bucket]++] = arr[i];
    }
    memcpy(arr, buf, n * sizeof(arr[0]));

    for (size_t bucket = 0U; bucket < RADIX_BUCKETS; ++bucket) {
        size_t left = begin[bucket];
        size_t right = begin[bucket + 1U];
        if (right > left) {
            msd_byte_sort(arr + left, buf + left, right - left, byte + 1U);
        }
    }
}

sorting_status_t msd_radix_sort_checked(int *arr, size_t n) {
    SOFT_ASSERT_FUNCTIONAL(arr != NULL || n == 0U, "Array pointer must not be NULL",
                           return SORTING_STATUS_NULL_ARG);

    if (n < 2U) {
        return SORTING_STATUS_OK;
    }

    int *buf = calloc(n, sizeof(buf[0]));
    RETURN_IF_ERROR(buf == NULL, SORTING_STATUS_ALLOC_FAIL,
                    "msd_radix_sort: no memory for %zu integers", n);

    msd_byte_sort(arr, buf, n, 0U);
    free(buf);
    return SORTING_STATUS_OK;
}

sorting_status_t timsort_sort_checked(int *arr, size_t n) {
    SOFT_ASSERT_FUNCTIONAL(arr != NULL || n == 0U, "Array pointer must not be NULL",
                           return SORTING_STATUS_NULL_ARG);

    if (n < 2U) {
        return SORTING_STATUS_OK;
    }

    int *buf = calloc(n, sizeof(buf[0]));
    RETURN_IF_ERROR(buf == NULL, SORTING_STATUS_ALLOC_FAIL,
                    "timsort_sort: no memory for %zu integers", n);

    size_t minrun = min_run_value(n);
    run_type stack[TIMSORT_STACK_CAPACITY] = {{0U, 0U}};
    size_t top = 0U;
    size_t pos = 0U;

    while (pos < n) {
        size_t run_end = find_run(arr, n, pos);
        size_t need = pos + minrun;
        if (run_end < need) {
            if (need > n) {
                need = n;
            }
            sorting_status_t status = insertion_sort_checked(arr + pos, need - pos);
            if (status != SORTING_STATUS_OK) {
                free(buf);
                return status;
            }
            run_end = need;
        }

        RETURN_IF_ERROR_CLEANUP(top >= TIMSORT_STACK_CAPACITY,
                                SORTING_STATUS_OVERFLOW,
                                free(buf),
                                "timsort_sort: run stack capacity exceeded for n=%zu", n);
        stack[top++] = (run_type){pos, run_end};
        pos = run_end;

        while (top > 1U) {
            size_t x = top - 1U;
            size_t len_x = stack[x].right - stack[x].left;
            size_t len_y = stack[x - 1U].right - stack[x - 1U].left;
            if (len_y > len_x) {
                break;
            }
            merge_at(arr, buf, stack, x - 1U);
            for (size_t k = x; k + 1U < top; ++k) {
                stack[k] = stack[k + 1U];
            }
            --top;
        }
    }

    while (top > 1U) {
        merge_at(arr, buf, stack, top - 2U);
        --top;
    }

    free(buf);
    return SORTING_STATUS_OK;
}

static sorting_status_t pdqsort_impl(int *arr, ptrdiff_t left, ptrdiff_t right, size_t bad_partitions) {
    while (left < right) {
        size_t len = (size_t)(right - left + 1);
        if (len < PDQSORT_INSERTION_THRESHOLD) {
            return insertion_sort_checked(arr + left, len);
        }
        if (bad_partitions == 0U) {
            return heap_kary_sort_checked(arr + left, len, PDQSORT_FALLBACK_HEAP_K);
        }

        range_type range = partition_three_way(arr, left, right, PIVOT_MEDIAN3_RANDOM);
        size_t left_len = (size_t)((range.left > left) ? (range.left - left) : 0);
        size_t right_len = (size_t)((right > range.right) ? (right - range.right) : 0);
        size_t bigger = (left_len > right_len) ? left_len : right_len;
        if (bigger * PDQSORT_BAD_PARTITION_NUMERATOR > len * PDQSORT_BAD_PARTITION_DENOMINATOR) {
            --bad_partitions;
        }

        if (left_len < right_len) {
            sorting_status_t status = pdqsort_impl(arr, left, range.left - 1, bad_partitions);
            if (status != SORTING_STATUS_OK) {
                return status;
            }
            left = range.right + 1;
        } else {
            sorting_status_t status = pdqsort_impl(arr, range.right + 1, right, bad_partitions);
            if (status != SORTING_STATUS_OK) {
                return status;
            }
            right = range.left - 1;
        }
    }

    return SORTING_STATUS_OK;
}

sorting_status_t pdqsort_sort_checked(int *arr, size_t n) {
    SOFT_ASSERT_FUNCTIONAL(arr != NULL || n == 0U, "Array pointer must not be NULL",
                           return SORTING_STATUS_NULL_ARG);

    if (n > 1U) {
        size_t bad = depth_limit(n, PDQSORT_DEPTH_COEF);
        return pdqsort_impl(arr, 0, (ptrdiff_t)n - 1, bad + PDQSORT_EXTRA_BAD_PARTITIONS);
    }
    return SORTING_STATUS_OK;
}

#define DEFINE_SORT_WRAPPER(name)                                      \
    void name(int *arr, size_t n) {                                    \
        sorting_status_t status = name##_checked(arr, n);              \
        if (status != SORTING_STATUS_OK) {                             \
            report_sorting_failure(#name, status);                     \
        }                                                              \
    }

DEFINE_SORT_WRAPPER(insertion_sort)
DEFINE_SORT_WRAPPER(bubble_sort)
DEFINE_SORT_WRAPPER(selection_sort)
DEFINE_SORT_WRAPPER(shell_knuth_sort)
DEFINE_SORT_WRAPPER(merge_recursive_sort)
DEFINE_SORT_WRAPPER(merge_iterative_sort)
DEFINE_SORT_WRAPPER(quick_lomuto_sort)
DEFINE_SORT_WRAPPER(quick_hoare_sort)
DEFINE_SORT_WRAPPER(quick_three_way_sort)
DEFINE_SORT_WRAPPER(lsd_radix_sort)
DEFINE_SORT_WRAPPER(msd_radix_sort)
DEFINE_SORT_WRAPPER(timsort_sort)
DEFINE_SORT_WRAPPER(pdqsort_sort)

void heap_kary_sort(int *arr, size_t n, size_t k) {
    sorting_status_t status = heap_kary_sort_checked(arr, n, k);
    if (status != SORTING_STATUS_OK) {
        report_sorting_failure("heap_kary_sort", status);
    }
}

void quick_best_sort(int *arr, size_t n, pivot_strategy_type strategy) {
    sorting_status_t status = quick_best_sort_checked(arr, n, strategy);
    if (status != SORTING_STATUS_OK) {
        report_sorting_failure("quick_best_sort", status);
    }
}

void introsort(int *arr, size_t n, size_t threshold, size_t heap_k, double depth_coef) {
    sorting_status_t status = introsort_checked(arr, n, threshold, heap_k, depth_coef);
    if (status != SORTING_STATUS_OK) {
        report_sorting_failure("introsort", status);
    }
}

void introsort_config_sort(int *arr, size_t n, size_t threshold, size_t heap_k, double depth_coef,
                           pivot_strategy_type pivot_strategy, sorting_fn small_sort) {
    sorting_status_t status = introsort_config_sort_checked(arr, n, threshold, heap_k, depth_coef,
                                                            pivot_strategy, small_sort);
    if (status != SORTING_STATUS_OK) {
        report_sorting_failure("introsort_config_sort", status);
    }
}
