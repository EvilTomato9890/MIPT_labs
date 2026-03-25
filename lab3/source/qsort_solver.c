#include <stdio.h>
#include <stdlib.h>

#include "asserts.h"

static int cmp_int(const void *lhs, const void *rhs) {
    int a = *(const int *)lhs;
    int b = *(const int *)rhs;
    return (a > b) - (a < b);
}

int main(void) {
    size_t n = 0U;
    HARD_ASSERT(fscanf(stdin, "%zu", &n) == 1, "qsort_solver: bad size");
    int *arr = (int *)calloc(n, sizeof(int));
    SOFT_ASSERT_FUNCTIONAL(arr != NULL || n == 0U,
                           "qsort_solver: no memory",
                           return 1);
    for (size_t i = 0U; i < n; ++i) {
        HARD_ASSERT(fscanf(stdin, "%d", &arr[i]) == 1, "qsort_solver: bad input");
    }

    qsort(arr, n, sizeof(arr[0]), cmp_int);
    printf("%zu\n", n);
    for (size_t i = 0U; i < n; ++i) {
        printf("%d%c", arr[i], (i + 1U == n) ? '\n' : ' ');
    }
    free(arr);
    return 0;
}
