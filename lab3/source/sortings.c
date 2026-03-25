#include "sortings.h"

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "asserts.h"
#include "logger.h"

typedef struct range_type {
    ptrdiff_t left;
    ptrdiff_t right;
} range_type;

static void swap_int(int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

void insertion_sort(int *arr, size_t n) {
    HARD_ASSERT(arr != NULL || n == 0U, "insertion_sort: invalid args");
    for (size_t i = 1; i < n; ++i) {
        int key = arr[i];
        size_t j = i;
        while (j > 0U && arr[j - 1U] > key) {
            arr[j] = arr[j - 1U];
            --j;
        }
        arr[j] = key;
    }
}

void bubble_sort(int *arr, size_t n) {
    HARD_ASSERT(arr != NULL || n == 0U, "bubble_sort: invalid args");
    for (size_t i = 0; i < n; ++i) {
        int swapped = 0;
        for (size_t j = 1; j < (n - i); ++j) {
            if (arr[j - 1U] > arr[j]) {
                swap_int(&arr[j - 1U], &arr[j]);
                swapped = 1;
            }
        }
        if (!swapped) {
            break;
        }
    }
}

void selection_sort(int *arr, size_t n) {
    HARD_ASSERT(arr != NULL || n == 0U, "selection_sort: invalid args");
    for (size_t i = 0; i < n; ++i) {
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
}

void shell_knuth_sort(int *arr, size_t n) {
    HARD_ASSERT(arr != NULL || n == 0U, "shell_knuth_sort: invalid args");
    size_t gap = 1U;
    while (gap < (n / 3U)) {
        gap = gap * 3U + 1U;
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
        gap = (gap - 1U) / 3U;
    }
}

static void sift_down_kary(int *arr, size_t n, size_t k, size_t root) {
    while (1) {
        size_t best = root;
        size_t first = root * k + 1U;
        for (size_t i = 0; i < k; ++i) {
            size_t child = first + i;
            if (child < n && arr[child] > arr[best]) {
                best = child;
            }
        }
        if (best == root) {
            return;
        }
        swap_int(&arr[root], &arr[best]);
        root = best;
    }
}

void heap_kary_sort(int *arr, size_t n, size_t k) {
    HARD_ASSERT(arr != NULL || n == 0U, "heap_kary_sort: invalid args");
    HARD_ASSERT(k >= 2U, "heap_kary_sort: k must be >= 2");
    if (n < 2U) {
        return;
    }

    size_t last_parent = (n - 2U) / k;
    for (size_t i = last_parent + 1U; i > 0U; --i) {
        sift_down_kary(arr, n, k, i - 1U);
    }

    for (size_t end = n; end > 1U; --end) {
        swap_int(&arr[0], &arr[end - 1U]);
        sift_down_kary(arr, end - 1U, k, 0U);
    }
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

void merge_recursive_sort(int *arr, size_t n) {
    HARD_ASSERT(arr != NULL || n == 0U, "merge_recursive_sort: invalid args");
    int *buf = (int *)calloc(n, sizeof(int));
    SOFT_ASSERT_FUNCTIONAL(buf != NULL || n == 0U,
                           "merge_recursive_sort: no memory",
                           return);
    merge_recursive_impl(arr, buf, 0U, n);
    free(buf);
}

void merge_iterative_sort(int *arr, size_t n) {
    HARD_ASSERT(arr != NULL || n == 0U, "merge_iterative_sort: invalid args");
    int *buf = (int *)calloc(n, sizeof(int));
    SOFT_ASSERT_FUNCTIONAL(buf != NULL || n == 0U,
                           "merge_iterative_sort: no memory",
                           return);

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
}

static ptrdiff_t pick_pivot(int *arr, ptrdiff_t l, ptrdiff_t r, pivot_strategy_type s) {
    ptrdiff_t c = l + (r - l) / 2;
    if (s == PIVOT_CENTER) {
        return c;
    }
    if (s == PIVOT_RANDOM) {
        return l + (ptrdiff_t)(rand() % (int)(r - l + 1));
    }

    ptrdiff_t a = l;
    ptrdiff_t b = c;
    ptrdiff_t d = r;
    if (s == PIVOT_MEDIAN3_RANDOM) {
        a = l + (ptrdiff_t)(rand() % (int)(r - l + 1));
        b = l + (ptrdiff_t)(rand() % (int)(r - l + 1));
        d = l + (ptrdiff_t)(rand() % (int)(r - l + 1));
    }

    int x = arr[a];
    int y = arr[b];
    int z = arr[d];
    if ((x <= y && y <= z) || (z <= y && y <= x)) {
        return b;
    }
    if ((y <= x && x <= z) || (z <= x && x <= y)) {
        return a;
    }
    return d;
}

static ptrdiff_t partition_lomuto(int *arr, ptrdiff_t l, ptrdiff_t r, pivot_strategy_type s) {
    ptrdiff_t p = pick_pivot(arr, l, r, s);
    int pv = arr[p];
    swap_int(&arr[p], &arr[r]);
    ptrdiff_t i = l;
    for (ptrdiff_t j = l; j < r; ++j) {
        if (arr[j] <= pv) {
            swap_int(&arr[i], &arr[j]);
            ++i;
        }
    }
    swap_int(&arr[i], &arr[r]);
    return i;
}

static ptrdiff_t partition_hoare(int *arr, ptrdiff_t l, ptrdiff_t r, pivot_strategy_type s) {
    int pivot = arr[pick_pivot(arr, l, r, s)];
    ptrdiff_t i = l - 1;
    ptrdiff_t j = r + 1;
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

static range_type partition_three_way(int *arr, ptrdiff_t l, ptrdiff_t r, pivot_strategy_type s) {
    int pivot = arr[pick_pivot(arr, l, r, s)];
    ptrdiff_t lt = l;
    ptrdiff_t i = l;
    ptrdiff_t gt = r;
    while (i <= gt) {
        if (arr[i] < pivot) {
            swap_int(&arr[lt++], &arr[i++]);
        } else if (arr[i] > pivot) {
            swap_int(&arr[i], &arr[gt--]);
        } else {
            ++i;
        }
    }
    range_type rg = {lt, gt};
    return rg;
}

static void quick_lomuto_impl(int *arr, ptrdiff_t l, ptrdiff_t r, pivot_strategy_type s) {
    while (l < r) {
        ptrdiff_t p = partition_lomuto(arr, l, r, s);
        if (p - l < r - p) {
            quick_lomuto_impl(arr, l, p - 1, s);
            l = p + 1;
        } else {
            quick_lomuto_impl(arr, p + 1, r, s);
            r = p - 1;
        }
    }
}

static void quick_hoare_impl(int *arr, ptrdiff_t l, ptrdiff_t r, pivot_strategy_type s) {
    while (l < r) {
        ptrdiff_t p = partition_hoare(arr, l, r, s);
        if (p - l < r - p) {
            quick_hoare_impl(arr, l, p, s);
            l = p + 1;
        } else {
            quick_hoare_impl(arr, p + 1, r, s);
            r = p;
        }
    }
}

static void quick_three_way_impl(int *arr, ptrdiff_t l, ptrdiff_t r, pivot_strategy_type s) {
    while (l < r) {
        range_type p = partition_three_way(arr, l, r, s);
        if ((p.left - l) < (r - p.right)) {
            quick_three_way_impl(arr, l, p.left - 1, s);
            l = p.right + 1;
        } else {
            quick_three_way_impl(arr, p.right + 1, r, s);
            r = p.left - 1;
        }
    }
}

void quick_lomuto_sort(int *arr, size_t n) {
    HARD_ASSERT(arr != NULL || n == 0U, "quick_lomuto_sort: invalid args");
    if (n > 1U) {
        quick_lomuto_impl(arr, 0, (ptrdiff_t)n - 1, PIVOT_CENTER);
    }
}

void quick_hoare_sort(int *arr, size_t n) {
    HARD_ASSERT(arr != NULL || n == 0U, "quick_hoare_sort: invalid args");
    if (n > 1U) {
        quick_hoare_impl(arr, 0, (ptrdiff_t)n - 1, PIVOT_CENTER);
    }
}

void quick_three_way_sort(int *arr, size_t n) {
    HARD_ASSERT(arr != NULL || n == 0U, "quick_three_way_sort: invalid args");
    if (n > 1U) {
        quick_three_way_impl(arr, 0, (ptrdiff_t)n - 1, PIVOT_CENTER);
    }
}

void quick_best_sort(int *arr, size_t n, pivot_strategy_type strategy) {
    HARD_ASSERT(arr != NULL || n == 0U, "quick_best_sort: invalid args");
    if (n > 1U) {
        quick_three_way_impl(arr, 0, (ptrdiff_t)n - 1, strategy);
    }
}

static size_t depth_limit(size_t n, double coef) {
    if (n < 2U) {
        return 0U;
    }
    double v = coef * log2((double)n);
    return (size_t)((v < 1.0) ? 1.0 : v);
}

static void introsort_impl(int *arr, ptrdiff_t l, ptrdiff_t r,
                           size_t threshold, size_t heap_k, size_t depth) {
    while (l < r) {
        size_t len = (size_t)(r - l + 1);
        if (len <= threshold) {
            insertion_sort(arr + l, len);
            return;
        }
        if (depth == 0U) {
            heap_kary_sort(arr + l, len, heap_k);
            return;
        }
        range_type p = partition_three_way(arr, l, r, PIVOT_MEDIAN3);
        --depth;
        introsort_impl(arr, l, p.left - 1, threshold, heap_k, depth);
        l = p.right + 1;
    }
}

void introsort(int *arr, size_t n, size_t threshold, size_t heap_k, double depth_coef) {
    HARD_ASSERT(arr != NULL || n == 0U, "introsort: invalid args");
    HARD_ASSERT(threshold >= 2U, "introsort: threshold must be >= 2");
    HARD_ASSERT(heap_k >= 2U, "introsort: heap_k must be >= 2");
    if (n > 1U) {
        introsort_impl(arr, 0, (ptrdiff_t)n - 1,
                       threshold, heap_k, depth_limit(n, depth_coef));
    }
}

void lsd_radix_sort(int *arr, size_t n) {
    HARD_ASSERT(arr != NULL || n == 0U, "lsd_radix_sort: invalid args");
    int *buf = (int *)calloc(n, sizeof(int));
    SOFT_ASSERT_FUNCTIONAL(buf != NULL || n == 0U,
                           "lsd_radix_sort: no memory",
                           return);

    for (size_t byte = 0U; byte < sizeof(int); ++byte) {
        size_t count[256] = {0};
        for (size_t i = 0U; i < n; ++i) {
            uint32_t u = (uint32_t)arr[i] ^ 0x80000000U;
            size_t b = (size_t)((u >> (byte * 8U)) & 0xFFU);
            ++count[b];
        }
        size_t sum = 0U;
        for (size_t i = 0U; i < 256U; ++i) {
            size_t cur = count[i];
            count[i] = sum;
            sum += cur;
        }
        for (size_t i = 0U; i < n; ++i) {
            uint32_t u = (uint32_t)arr[i] ^ 0x80000000U;
            size_t b = (size_t)((u >> (byte * 8U)) & 0xFFU);
            buf[count[b]++] = arr[i];
        }
        memcpy(arr, buf, n * sizeof(arr[0]));
    }
    free(buf);
}

static void msd_byte_sort(int *arr, int *buf, size_t n, size_t byte) {
    if (n < 2U || byte >= sizeof(int)) {
        return;
    }

    size_t count[257] = {0};
    for (size_t i = 0U; i < n; ++i) {
        uint32_t u = (uint32_t)arr[i] ^ 0x80000000U;
        size_t b = (size_t)((u >> ((sizeof(int) - byte - 1U) * 8U)) & 0xFFU);
        ++count[b + 1U];
    }
    for (size_t i = 1U; i < 257U; ++i) {
        count[i] += count[i - 1U];
    }

    size_t begin[257] = {0};
    memcpy(begin, count, sizeof(begin));
    for (size_t i = 0U; i < n; ++i) {
        uint32_t u = (uint32_t)arr[i] ^ 0x80000000U;
        size_t b = (size_t)((u >> ((sizeof(int) - byte - 1U) * 8U)) & 0xFFU);
        buf[count[b]++] = arr[i];
    }
    memcpy(arr, buf, n * sizeof(arr[0]));

    for (size_t b = 0U; b < 256U; ++b) {
        size_t l = begin[b];
        size_t r = begin[b + 1U];
        if (r > l) {
            msd_byte_sort(arr + l, buf + l, r - l, byte + 1U);
        }
    }
}

void msd_radix_sort(int *arr, size_t n) {
    HARD_ASSERT(arr != NULL || n == 0U, "msd_radix_sort: invalid args");
    int *buf = (int *)calloc(n, sizeof(int));
    SOFT_ASSERT_FUNCTIONAL(buf != NULL || n == 0U,
                           "msd_radix_sort: no memory",
                           return);
    msd_byte_sort(arr, buf, n, 0U);
    free(buf);
}

typedef struct run_type {
    size_t left;
    size_t right;
} run_type;

static size_t min_run_value(size_t n) {
    size_t r = 0U;
    while (n >= 64U) {
        r |= n & 1U;
        n >>= 1U;
    }
    return n + r;
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

static void merge_inplace_buffer(int *arr, int *buf, size_t l, size_t m, size_t r) {
    size_t len = m - l;
    memcpy(buf, arr + l, len * sizeof(arr[0]));
    size_t i = 0U;
    size_t j = m;
    size_t p = l;
    while (i < len && j < r) {
        arr[p++] = (buf[i] <= arr[j]) ? buf[i++] : arr[j++];
    }
    while (i < len) {
        arr[p++] = buf[i++];
    }
}

static void merge_at(int *arr, int *buf, run_type *stack, size_t idx) {
    size_t l = stack[idx].left;
    size_t m = stack[idx].right;
    size_t r = stack[idx + 1U].right;
    merge_inplace_buffer(arr, buf, l, m, r);
    stack[idx].right = r;
}

void timsort_sort(int *arr, size_t n) {
    HARD_ASSERT(arr != NULL || n == 0U, "timsort_sort: invalid args");
    if (n < 2U) {
        return;
    }

    int *buf = (int *)calloc(n, sizeof(int));
    SOFT_ASSERT_FUNCTIONAL(buf != NULL,
                           "timsort_sort: no memory",
                           return);

    size_t minrun = min_run_value(n);
    run_type stack[128] = {{0U, 0U}};
    size_t top = 0U;
    size_t pos = 0U;

    while (pos < n) {
        size_t run_end = find_run(arr, n, pos);
        size_t need = pos + minrun;
        if (run_end < need) {
            if (need > n) {
                need = n;
            }
            insertion_sort(arr + pos, need - pos);
            run_end = need;
        }

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
            stack[x - 1U] = stack[x - 1U];
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
}

static void pdqsort_impl(int *arr, ptrdiff_t l, ptrdiff_t r, size_t bad) {
    while (l < r) {
        size_t len = (size_t)(r - l + 1);
        if (len < 24U) {
            insertion_sort(arr + l, len);
            return;
        }
        if (bad == 0U) {
            heap_kary_sort(arr + l, len, 4U);
            return;
        }

        range_type p = partition_three_way(arr, l, r, PIVOT_MEDIAN3_RANDOM);
        size_t left_len = (size_t)((p.left > l) ? (p.left - l) : 0);
        size_t right_len = (size_t)((r > p.right) ? (r - p.right) : 0);
        size_t bigger = (left_len > right_len) ? left_len : right_len;
        if (bigger * 8U > len * 7U) {
            --bad;
        }

        if (left_len < right_len) {
            pdqsort_impl(arr, l, p.left - 1, bad);
            l = p.right + 1;
        } else {
            pdqsort_impl(arr, p.right + 1, r, bad);
            r = p.left - 1;
        }
    }
}

void pdqsort_sort(int *arr, size_t n) {
    HARD_ASSERT(arr != NULL || n == 0U, "pdqsort_sort: invalid args");
    if (n > 1U) {
        size_t bad = depth_limit(n, 2.0);
        pdqsort_impl(arr, 0, (ptrdiff_t)n - 1, bad + 4U);
    }
}
