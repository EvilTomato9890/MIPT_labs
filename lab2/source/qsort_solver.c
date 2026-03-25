#include <stdio.h>
#include <stdlib.h>

#include "asserts.h"
#include "return_macros.h"

#define SORTED_OUTPUT_BUFFER_EXTRA_CHARS 32U
#define SORTED_CHARS_PER_NUMBER 13U
#define MIN_ALLOCATED_LENGTH 1U

static int cmp_int(const void *lhs, const void *rhs) {
    int a = *(const int *)lhs;
    int b = *(const int *)rhs;
    return (a > b) - (a < b);
}

static void write_sorted_array(const int *arr, size_t n) {
    size_t buffer_size = SORTED_OUTPUT_BUFFER_EXTRA_CHARS + n * SORTED_CHARS_PER_NUMBER;
    char *buffer = (char *)calloc(buffer_size, sizeof(char));
    RETURN_IF_FAIL(buffer != NULL, "qsort_solver: no memory for output buffer");

    int written = snprintf(buffer, buffer_size, "%zu\n", n);
    HARD_ASSERT(written > 0, "qsort_solver: failed to format size");
    size_t pos = (size_t)written;

    for (size_t i = 0U; i < n; ++i) {
        written = snprintf(buffer + pos, buffer_size - pos, "%d%c",
                           arr[i], (i + 1U == n) ? '\n' : ' ');
        HARD_ASSERT(written > 0 && (size_t)written < (buffer_size - pos),
                    "qsort_solver: output buffer overflow");
        pos += (size_t)written;
    }

    HARD_ASSERT(fwrite(buffer, sizeof(char), pos, stdout) == pos,
                "qsort_solver: fwrite failed");
    free(buffer);
}

int main(void) {
    size_t n = 0U;
    HARD_ASSERT(fscanf(stdin, "%zu", &n) == 1, "qsort_solver: bad size");
    size_t alloc_n = (n == 0U) ? MIN_ALLOCATED_LENGTH : n;
    int *arr = (int *)calloc(alloc_n, sizeof(int));
    RETURN_VAL_IF_FAIL(arr != NULL, 1, "qsort_solver: no memory");
    for (size_t i = 0U; i < n; ++i) {
        HARD_ASSERT(fscanf(stdin, "%d", &arr[i]) == 1, "qsort_solver: bad input");
    }

    if (n > 1U) {
        qsort(arr, n, sizeof(arr[0]), cmp_int);
    }
    write_sorted_array(arr, n);
    free(arr);
    return 0;
}
