#ifndef LAB2_INCLUDE_SORTINGS_H_INCLUDED
#define LAB2_INCLUDE_SORTINGS_H_INCLUDED

#include <stddef.h>

typedef enum sorting_status {
    SORTING_STATUS_OK = 0,
    SORTING_STATUS_NULL_ARG,
    SORTING_STATUS_INVALID_ARG,
    SORTING_STATUS_ALLOC_FAIL,
    SORTING_STATUS_OVERFLOW
} sorting_status_t;

typedef void (*sorting_fn)(int *arr, size_t n);
typedef sorting_status_t (*sorting_checked_fn)(int *arr, size_t n);

typedef enum pivot_strategy_type {
    PIVOT_CENTER = 0,
    PIVOT_MEDIAN3 = 1,
    PIVOT_RANDOM = 2,
    PIVOT_MEDIAN3_RANDOM = 3
} pivot_strategy_type;

void insertion_sort  (int *arr, size_t n);
void bubble_sort     (int *arr, size_t n);
void selection_sort  (int *arr, size_t n);
void shell_knuth_sort(int *arr, size_t n);

void heap_kary_sort      (int *arr, size_t n, size_t k);
void merge_recursive_sort(int *arr, size_t n);
void merge_iterative_sort(int *arr, size_t n);

void quick_lomuto_sort   (int *arr, size_t n);
void quick_hoare_sort    (int *arr, size_t n);
void quick_three_way_sort(int *arr, size_t n);
void quick_best_sort     (int *arr, size_t n, pivot_strategy_type strategy);

void introsort            (int *arr, size_t n, size_t threshold, size_t heap_k, double depth_coef);
void introsort_config_sort(int *arr, size_t n, size_t threshold, size_t heap_k, double depth_coef,
                           pivot_strategy_type pivot_strategy, sorting_fn small_sort);

void lsd_radix_sort(int *arr, size_t n);
void msd_radix_sort(int *arr, size_t n);

void timsort_sort(int *arr, size_t n);
void pdqsort_sort(int *arr, size_t n);

sorting_status_t insertion_sort_checked  (int *arr, size_t n);
sorting_status_t bubble_sort_checked     (int *arr, size_t n);
sorting_status_t selection_sort_checked  (int *arr, size_t n);
sorting_status_t shell_knuth_sort_checked(int *arr, size_t n);

sorting_status_t heap_kary_sort_checked      (int *arr, size_t n, size_t k);
sorting_status_t merge_recursive_sort_checked(int *arr, size_t n);
sorting_status_t merge_iterative_sort_checked(int *arr, size_t n);

sorting_status_t quick_lomuto_sort_checked   (int *arr, size_t n);
sorting_status_t quick_hoare_sort_checked    (int *arr, size_t n);
sorting_status_t quick_three_way_sort_checked(int *arr, size_t n);
sorting_status_t quick_best_sort_checked     (int *arr, size_t n, pivot_strategy_type strategy);

sorting_status_t introsort_checked            (int *arr, size_t n, size_t threshold, size_t heap_k, double depth_coef);
sorting_status_t introsort_config_sort_checked(int *arr, size_t n, size_t threshold, size_t heap_k,
                                               double depth_coef, pivot_strategy_type pivot_strategy,
                                               sorting_fn small_sort);

sorting_status_t lsd_radix_sort_checked(int *arr, size_t n);
sorting_status_t msd_radix_sort_checked(int *arr, size_t n);

sorting_status_t timsort_sort_checked(int *arr, size_t n);
sorting_status_t pdqsort_sort_checked(int *arr, size_t n);

#endif /* LAB2_INCLUDE_SORTINGS_H_INCLUDED */
