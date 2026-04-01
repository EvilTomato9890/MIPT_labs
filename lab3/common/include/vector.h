#ifndef LAB3_COMMON_INCLUDE_VECTOR_H_NCLUDED
#define LAB3_COMMON_INCLUDE_VECTOR_H_NCLUDED

#include <stdbool.h>
#include <stddef.h>

enum vector_error_t {
    VEC_ERR_OK,
    VEC_ERR_MEM_ALLOC,
    VEC_ERR_FULL,
    VEC_ERR_BAD_ARG,
    VEC_ERR_NOT_FOUND,
    VEC_ERR_INTERNAL
};

typedef enum vector_error_t vector_error_t;

struct vector_t {
    void* data;

    size_t size;
    size_t capacity;

    size_t elem_size;

    bool is_static;
};

typedef struct vector_t vector_t;
typedef int (*compare_func_t)(void*, void*);

vector_error_t vector_init       (vector_t* vector, size_t capacity, size_t elem_size);
vector_error_t vector_static_init(vector_t* vector, void* data, size_t capacity, size_t elem_size);
vector_error_t vector_destroy    (vector_t* vector);

size_t vector_size    (const vector_t* vector);
size_t vector_capacity(const vector_t* vector);

      void* vector_get      (const vector_t* vector, size_t index);
const void* vector_get_const(const vector_t* vector, size_t index);
      void* vector_put      (const vector_t* vector, size_t index, void* value);
      void vector_clear     (vector_t*       vector);

vector_error_t vector_push_back(vector_t* vector, const void* elem);
vector_error_t vector_pop_back (vector_t* vector, void* elem_out);

vector_error_t vector_insert(vector_t* vector, size_t index, const void* elem);
vector_error_t vector_erase (vector_t* vector, size_t index, void* elem_out);

size_t vector_lower_bound(const vector_t* vector,
                          size_t left_border, size_t right_border,
                          const void* value, compare_func_t compare_func);
size_t vector_upper_bound(const vector_t* vector,
                          size_t left_border, size_t right_border,
                          const void* value, compare_func_t compare_func);

vector_error_t vector_qsort(vector_t* vector, compare_func_t compare_func);
vector_error_t vector_msort(vector_t* vector, compare_func_t compare_func);
vector_error_t vector_swap (vector_t* vector, size_t idx1, size_t idx2);
vector_error_t vector_lsd_sort(vector_t* vector);
vector_error_t vector_qsort_fq(vector_t* vector, compare_func_t compare_func);

size_t vector_required_bytes(size_t capacity, size_t elem_size);

#define SIMPLE_VECTOR_INIT(vector_, capacity_, elem_type_) \
    vector_init((vector_), (capacity_), sizeof(elem_type_))

#define SIMPLE_VECTOR_STATIC_INIT(vector_, data_, capacity_, elem_type_) \
    vector_static_init((vector_), (data_), (capacity_), sizeof(elem_type_))

#endif /* LAB3_COMMON_INCLUDE_VECTOR_H_NCLUDED */
