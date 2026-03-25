#ifndef LAB3_TESTING_H_INCLUDED
#define LAB3_TESTING_H_INCLUDED

#include <stddef.h>

#include "sortings.h"

typedef struct dataset_config {
    const char *tests_dir;
    const char *result_csv;
    size_t from;
    size_t to;
    size_t step;
    size_t copies;
} dataset_config;

typedef struct timing_array {
    double *values;
    size_t count;
} timing_array;

timing_array run_sorting_dataset(const dataset_config *cfg, sorting_fn sorter, const char *name);
void timing_array_free(timing_array *times);

#endif
