#ifndef LAB3_SORTINGS_H_INCLUDED
#define LAB3_SORTINGS_H_INCLUDED

#include <stddef.h>

typedef void (*sorting_fn)(int *arr, size_t n);

typedef enum pivot_strategy_type {
    PIVOT_CENTER = 0,
    PIVOT_MEDIAN3 = 1,
    PIVOT_RANDOM = 2,
    PIVOT_MEDIAN3_RANDOM = 3
} pivot_strategy_type;

void insertion_sort(int *arr, size_t n);
void bubble_sort(int *arr, size_t n);
void selection_sort(int *arr, size_t n);
void shell_knuth_sort(int *arr, size_t n);

void heap_kary_sort(int *arr, size_t n, size_t k);
void merge_recursive_sort(int *arr, size_t n);
void merge_iterative_sort(int *arr, size_t n);

void quick_lomuto_sort(int *arr, size_t n);
void quick_hoare_sort(int *arr, size_t n);
void quick_three_way_sort(int *arr, size_t n);
void quick_best_sort(int *arr, size_t n, pivot_strategy_type strategy);

void introsort(int *arr, size_t n, size_t threshold, size_t heap_k, double depth_coef);

void lsd_radix_sort(int *arr, size_t n);
void msd_radix_sort(int *arr, size_t n);

#endif
