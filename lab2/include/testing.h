#ifndef LAB2_TESTING_H_INCLUDED
#define LAB2_TESTING_H_INCLUDED

#include <stddef.h>

#include "sortings.h"

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

void run_sorting_group(const dataset_config *cfg, const named_sorting *sorters, size_t count);

#endif
