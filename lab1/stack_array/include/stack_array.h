#ifndef LAB1_STACK_ARRAY_INCLUDE_STACK_ARRAY_H_NCLUDED
#define LAB1_STACK_ARRAY_INCLUDE_STACK_ARRAY_H_NCLUDED

#include <stddef.h>

#include "dynamic_array.h"

typedef enum stack_array_status {
    STACK_ARRAY_STATUS_OK = 0,
    STACK_ARRAY_STATUS_NULL_ARG,
    STACK_ARRAY_STATUS_INVALID_ARG,
    STACK_ARRAY_STATUS_ALLOC_FAIL,
    STACK_ARRAY_STATUS_EMPTY,
    STACK_ARRAY_STATUS_BAD_STATE,
    STACK_ARRAY_STATUS_INTERNAL_ERROR
} stack_array_status_t;

typedef struct stack_array {
    dynamic_array_t storage;
} stack_array;

stack_array_status_t stack_array_ctr(size_t size, size_t element_size, stack_array **out_stack);
stack_array_status_t stack_array_push      (stack_array *stack, const void *buffer);
stack_array_status_t stack_array_top (const stack_array *stack,       void *buffer);
stack_array_status_t stack_array_pop       (stack_array *stack);
stack_array_status_t stack_array_size(const stack_array *stack, size_t *out_size);
stack_array_status_t stack_array_dtr       (stack_array **stack);

#endif /* LAB1_STACK_ARRAY_INCLUDE_STACK_ARRAY_H_NCLUDED */
