#ifndef LAB1_STACK_LIST_INCLUDE_STACK_LIST_H_NCLUDED
#define LAB1_STACK_LIST_INCLUDE_STACK_LIST_H_NCLUDED

#include <stddef.h>

#include "singly_linked_list.h"

typedef enum stack_list_status {
    STACK_LIST_STATUS_OK = 0,
    STACK_LIST_STATUS_NULL_ARG,
    STACK_LIST_STATUS_INVALID_ARG,
    STACK_LIST_STATUS_ALLOC_FAIL,
    STACK_LIST_STATUS_EMPTY,
    STACK_LIST_STATUS_BAD_STATE,
    STACK_LIST_STATUS_INTERNAL_ERROR
} stack_list_status_t;

typedef struct stack_list {
    singly_linked_list_t storage;
} stack_list;

stack_list_status_t stack_list_ctor(size_t size, size_t element_size, stack_list **out_stack);
stack_list_status_t stack_list_push      (stack_list *stack, const void *buffer);
stack_list_status_t stack_list_top (const stack_list *stack,       void *buffer);
stack_list_status_t stack_list_pop       (stack_list *stack);
stack_list_status_t stack_list_size(const stack_list *stack, size_t *out_size);
stack_list_status_t stack_list_dtor      (stack_list **stack);

#endif /* LAB1_STACK_LIST_INCLUDE_STACK_LIST_H_NCLUDED */
