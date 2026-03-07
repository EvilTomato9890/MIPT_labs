#ifndef LAB1_DYNAMIC_ARRAY_INCLUDE_DYNAMIC_ARRAY_H_INCLUDED
#define LAB1_DYNAMIC_ARRAY_INCLUDE_DYNAMIC_ARRAY_H_INCLUDED

#include <stddef.h>

typedef enum dynamic_array_status {
    DYNAMIC_ARRAY_STATUS_OK = 0,
    DYNAMIC_ARRAY_STATUS_NULL_ARG,
    DYNAMIC_ARRAY_STATUS_INVALID_ARG,
    DYNAMIC_ARRAY_STATUS_OVERFLOW,
    DYNAMIC_ARRAY_STATUS_ALLOC_FAIL,
    DYNAMIC_ARRAY_STATUS_EMPTY,
    DYNAMIC_ARRAY_STATUS_BAD_STATE
} dynamic_array_status_t;

typedef struct dynamic_array {
    unsigned char *data;
    size_t size;
    size_t capacity;
    size_t minimum_capacity;
    size_t element_size;
} dynamic_array_t;

dynamic_array_status_t dynamic_array_ctor      (dynamic_array_t *array, size_t initial_capacity, size_t element_size);
dynamic_array_status_t dynamic_array_dtor      (dynamic_array_t *array);
dynamic_array_status_t dynamic_array_push_back (dynamic_array_t *array, const void *element);
dynamic_array_status_t dynamic_array_back(const dynamic_array_t *array,       void *out_element);
dynamic_array_status_t dynamic_array_pop_back  (dynamic_array_t *array);
dynamic_array_status_t dynamic_array_size(const dynamic_array_t *array, size_t *out_size);

#endif /* LAB1_DYNAMIC_ARRAY_INCLUDE_DYNAMIC_ARRAY_H_INCLUDED */
