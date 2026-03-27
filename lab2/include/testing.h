#ifndef LAB2_INCLUDE_TESTING_H_INCLUDED
#define LAB2_INCLUDE_TESTING_H_INCLUDED

#include <stddef.h>

#include "sortings.h"

typedef enum testing_status {
    TESTING_STATUS_OK = 0,
    TESTING_STATUS_NULL_ARG,
    TESTING_STATUS_INVALID_ARG,
    TESTING_STATUS_OVERFLOW,
    TESTING_STATUS_ALLOC_FAIL,
    TESTING_STATUS_IO_FAIL,
    TESTING_STATUS_DATA_MISMATCH
} testing_status_t;

typedef struct named_sorting {
    const char *name;
    sorting_fn fn;
} named_sorting;

typedef struct dataset_config {
    const char *tests_dir;
    const char *result_csv;
    size_t from;
    size_t to;
    size_t step;
    size_t copies;
} dataset_config;

double *test_sorting(const char *tests_dir, sorting_fn sorter, const char *result_csv,
                     size_t from, size_t to, size_t step);

testing_status_t run_sorting_group(const dataset_config *cfg, const named_sorting *sorters, size_t count);

#endif /* LAB2_INCLUDE_TESTING_H_INCLUDED */
