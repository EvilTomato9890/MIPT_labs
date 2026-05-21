/**
 * @file stack_list.c
 * @brief Implementation of the stack backed by a singly linked list.
 */

#include "stack_list.h"

#include <stdlib.h>

#include "asserts.h"
#include "logger.h"

#define SL_RETURN(status_value)                                                            \
    do {                                                                                   \
        stack_list_status_t status_to_return = (status_value);                             \
        if (status_to_return != STACK_LIST_STATUS_OK) {                                    \
            LOGGER_ERROR("stack_list return status=%d", (int)status_to_return);          \
        }                                                                                  \
        return status_to_return;                                                           \
    } while (0)

static stack_list_status_t stack_list_map_list_status(singly_linked_list_status_t status) {
    switch (status) {
        case SINGLY_LINKED_LIST_STATUS_OK:          return STACK_LIST_STATUS_OK;
        case SINGLY_LINKED_LIST_STATUS_NULL_ARG:    return STACK_LIST_STATUS_NULL_ARG;
        case SINGLY_LINKED_LIST_STATUS_INVALID_ARG: return STACK_LIST_STATUS_INVALID_ARG;
        case SINGLY_LINKED_LIST_STATUS_ALLOC_FAIL:  return STACK_LIST_STATUS_ALLOC_FAIL;
        case SINGLY_LINKED_LIST_STATUS_EMPTY:       return STACK_LIST_STATUS_EMPTY;
        case SINGLY_LINKED_LIST_STATUS_BAD_STATE:   return STACK_LIST_STATUS_BAD_STATE;
        case SINGLY_LINKED_LIST_STATUS_OVERFLOW:    return STACK_LIST_STATUS_INTERNAL_ERROR;
        default: return STACK_LIST_STATUS_INTERNAL_ERROR;
    }
}

stack_list_status_t stack_list_ctor(size_t size, size_t element_size, stack_list **out_stack) {
    (void)size;

    SOFT_ASSERT_FUNCTIONAL(out_stack != NULL, "Output stack pointer must not be NULL",
                           SL_RETURN(STACK_LIST_STATUS_NULL_ARG));

    stack_list *created_stack = (stack_list *)calloc(1U, sizeof(stack_list));
    if (created_stack == NULL) {
        SL_RETURN(STACK_LIST_STATUS_ALLOC_FAIL);
    }

    singly_linked_list_status_t init_status = singly_linked_list_ctor(&created_stack->storage, element_size);
    if (init_status != SINGLY_LINKED_LIST_STATUS_OK) {
        free(created_stack);
        SL_RETURN(stack_list_map_list_status(init_status));
    }

    *out_stack = created_stack;
    SL_RETURN(STACK_LIST_STATUS_OK);
}

stack_list_status_t stack_list_push(stack_list *stack, const void *buffer) {
    SOFT_ASSERT_FUNCTIONAL(stack != NULL && buffer != NULL, "Push arguments must not be NULL",
                           SL_RETURN(STACK_LIST_STATUS_NULL_ARG));

    singly_linked_list_status_t status = singly_linked_list_push_front(&stack->storage, buffer);
    SL_RETURN(stack_list_map_list_status(status));
}

stack_list_status_t stack_list_top(const stack_list *stack, void *buffer) {
    SOFT_ASSERT_FUNCTIONAL(stack != NULL && buffer != NULL, "Top arguments must not be NULL",
                           SL_RETURN(STACK_LIST_STATUS_NULL_ARG));

    singly_linked_list_status_t status = singly_linked_list_front(&stack->storage, buffer);
    SL_RETURN(stack_list_map_list_status(status));
}

stack_list_status_t stack_list_pop(stack_list *stack) {
    SOFT_ASSERT_FUNCTIONAL(stack != NULL, "Stack pointer must not be NULL",
                           SL_RETURN(STACK_LIST_STATUS_NULL_ARG));

    singly_linked_list_status_t status = singly_linked_list_pop_front(&stack->storage);
    SL_RETURN(stack_list_map_list_status(status));
}

stack_list_status_t stack_list_size(const stack_list *stack, size_t *out_size) {
    SOFT_ASSERT_FUNCTIONAL(stack != NULL && out_size != NULL, "Size arguments must not be NULL",
                           SL_RETURN(STACK_LIST_STATUS_NULL_ARG));

    singly_linked_list_status_t status = singly_linked_list_size(&stack->storage, out_size);
    SL_RETURN(stack_list_map_list_status(status));
}

stack_list_status_t stack_list_dtor(stack_list **stack) {
    SOFT_ASSERT_FUNCTIONAL(stack != NULL, "Stack output pointer must not be NULL",
                           SL_RETURN(STACK_LIST_STATUS_NULL_ARG));
    if (*stack == NULL) {
        SL_RETURN(STACK_LIST_STATUS_OK);
    }

    stack_list *object = *stack;
    singly_linked_list_status_t status = singly_linked_list_dtor(&object->storage);
    if (status != SINGLY_LINKED_LIST_STATUS_OK) {
        SL_RETURN(stack_list_map_list_status(status));
    }

    free(object);
    *stack = NULL;
    SL_RETURN(STACK_LIST_STATUS_OK);
}
