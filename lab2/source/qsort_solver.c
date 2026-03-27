#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "asserts.h"
#include "logger.h"
#include "return_macros.h"

#define SORTED_OUTPUT_BUFFER_EXTRA_CHARS 32U
#define SORTED_CHARS_PER_NUMBER          13U
#define MIN_ALLOCATED_LENGTH             1U

typedef enum qsort_solver_status {
    QSORT_SOLVER_STATUS_OK = 0,
    QSORT_SOLVER_STATUS_NULL_ARG,
    QSORT_SOLVER_STATUS_INVALID_INPUT,
    QSORT_SOLVER_STATUS_OVERFLOW,
    QSORT_SOLVER_STATUS_ALLOC_FAIL,
    QSORT_SOLVER_STATUS_IO_FAIL
} qsort_solver_status_t;

static int cmp_int(const void *lhs, const void *rhs) {
    int a = *(const int *)lhs;
    int b = *(const int *)rhs;
    return (a > b) - (a < b);
}

static qsort_solver_status_t write_sorted_array(const int *arr, size_t n) {
    SOFT_ASSERT_FUNCTIONAL(arr != NULL || n == 0U,
                           "Array pointer must not be NULL",
                           return QSORT_SOLVER_STATUS_NULL_ARG);

    RETURN_IF_ERROR(n > (SIZE_MAX - SORTED_OUTPUT_BUFFER_EXTRA_CHARS) / SORTED_CHARS_PER_NUMBER,
                    QSORT_SOLVER_STATUS_OVERFLOW,
                    "qsort_solver: requested array size %zu is too large", n);

    size_t buffer_size = SORTED_OUTPUT_BUFFER_EXTRA_CHARS + n * SORTED_CHARS_PER_NUMBER;
    char *buffer = calloc(buffer_size, sizeof(buffer[0]));
    RETURN_IF_ERROR(buffer == NULL, QSORT_SOLVER_STATUS_ALLOC_FAIL,
                    "qsort_solver: no memory for output buffer");

    int written = snprintf(buffer, buffer_size, "%zu\n", n);
    RETURN_IF_ERROR_CLEANUP(written <= 0 || (size_t)written >= buffer_size,
                            QSORT_SOLVER_STATUS_IO_FAIL,
                            free(buffer),
                            "qsort_solver: failed to format size");
    size_t pos = (size_t)written;

    for (size_t i = 0U; i < n; ++i) {
        written = snprintf(buffer + pos, buffer_size - pos, "%d%c",
                           arr[i], (i + 1U == n) ? '\n' : ' ');
        RETURN_IF_ERROR_CLEANUP(written <= 0 || (size_t)written >= (buffer_size - pos),
                                QSORT_SOLVER_STATUS_IO_FAIL,
                                free(buffer),
                                "qsort_solver: output buffer overflow while writing item %zu", i);
        pos += (size_t)written;
    }

    size_t write_pos = fwrite(buffer, sizeof(buffer[0]), pos, stdout);
    RETURN_IF_ERROR_CLEANUP(write_pos != pos,
                            QSORT_SOLVER_STATUS_IO_FAIL,
                            free(buffer),
                            "qsort_solver: fwrite failed");
    free(buffer);
    return QSORT_SOLVER_STATUS_OK;
}

int main(void) {
    size_t n = 0U;
    int args_cnt = fscanf(stdin, "%zu", &n);
    RETURN_IF_ERROR(args_cnt != 1,
                    QSORT_SOLVER_STATUS_INVALID_INPUT,
                    "qsort_solver: failed to read array size");

    size_t alloc_n = (n == 0U) ? MIN_ALLOCATED_LENGTH : n;
    int *arr = calloc(alloc_n, sizeof(arr[0]));
    RETURN_IF_ERROR(arr == NULL, QSORT_SOLVER_STATUS_ALLOC_FAIL,
                    "qsort_solver: no memory for %zu integers", n);

    for (size_t i = 0U; i < n; ++i) {
        args_cnt = fscanf(stdin, "%d", &arr[i]);
        RETURN_IF_ERROR_CLEANUP(args_cnt != 1,
                                QSORT_SOLVER_STATUS_INVALID_INPUT,
                                free(arr),
                                "qsort_solver: failed to read item %zu", i);
    }

    if (n > 1U) qsort(arr, n, sizeof(arr[0]), cmp_int);

    qsort_solver_status_t status = write_sorted_array(arr, n);
    free(arr);
    return (int)status;
}
