#include "stack_array.h"

#include <stdlib.h>

#include "asserts.h"
#include "logger.h"

#define SA_RETURN(status_value)                                                            \
    do {                                                                                   \
        stack_array_status_t status_to_return = (status_value);                            \
        if (status_to_return != STACK_ARRAY_STATUS_OK) {                                   \
            LOGGER_ERROR("stack_array return status=%d", (int)status_to_return);         \
        }                                                                                  \
        return status_to_return;                                                           \
    } while (0)

static stack_array_status_t stack_array_map_dynamic_status(dynamic_array_status_t status) {
    switch (status) {
        case DYNAMIC_ARRAY_STATUS_OK:          return STACK_ARRAY_STATUS_OK;
        case DYNAMIC_ARRAY_STATUS_NULL_ARG:    return STACK_ARRAY_STATUS_NULL_ARG;
        case DYNAMIC_ARRAY_STATUS_INVALID_ARG: return STACK_ARRAY_STATUS_INVALID_ARG;
        case DYNAMIC_ARRAY_STATUS_ALLOC_FAIL:  return STACK_ARRAY_STATUS_ALLOC_FAIL;
        case DYNAMIC_ARRAY_STATUS_EMPTY:       return STACK_ARRAY_STATUS_EMPTY;
        case DYNAMIC_ARRAY_STATUS_BAD_STATE:   return STACK_ARRAY_STATUS_BAD_STATE;
        case DYNAMIC_ARRAY_STATUS_OVERFLOW:    return STACK_ARRAY_STATUS_INTERNAL_ERROR;
        default:                               return STACK_ARRAY_STATUS_INTERNAL_ERROR;
    }
}

stack_array_status_t stack_array_ctr(size_t size, size_t element_size, stack_array **out_stack) {
    SOFT_ASSERT_FUNCTIONAL(out_stack != NULL, "Output stack pointer must not be NULL",
                           SA_RETURN(STACK_ARRAY_STATUS_NULL_ARG));

    stack_array *created_stack = (stack_array *)calloc(1U, sizeof(stack_array));
    if (created_stack == NULL) {
        SA_RETURN(STACK_ARRAY_STATUS_ALLOC_FAIL);
    }

    dynamic_array_status_t init_status = dynamic_array_ctor(&created_stack->storage, size, element_size);
    if (init_status != DYNAMIC_ARRAY_STATUS_OK) {
        free(created_stack);
        SA_RETURN(stack_array_map_dynamic_status(init_status));
    }

    *out_stack = created_stack;
    SA_RETURN(STACK_ARRAY_STATUS_OK);
}

stack_array_status_t stack_array_push(stack_array *stack, const void *buffer) {
    SOFT_ASSERT_FUNCTIONAL(stack != NULL && buffer != NULL, "Push arguments must not be NULL",
                           SA_RETURN(STACK_ARRAY_STATUS_NULL_ARG));

    dynamic_array_status_t status = dynamic_array_push_back(&stack->storage, buffer);
    SA_RETURN(stack_array_map_dynamic_status(status));
}

stack_array_status_t stack_array_top(const stack_array *stack, void *buffer) {
    SOFT_ASSERT_FUNCTIONAL(stack != NULL && buffer != NULL, "Top arguments must not be NULL",
                           SA_RETURN(STACK_ARRAY_STATUS_NULL_ARG));

    dynamic_array_status_t status = dynamic_array_back(&stack->storage, buffer);
    SA_RETURN(stack_array_map_dynamic_status(status));
}

stack_array_status_t stack_array_pop(stack_array *stack) {
    SOFT_ASSERT_FUNCTIONAL(stack != NULL, "Stack pointer must not be NULL",
                           SA_RETURN(STACK_ARRAY_STATUS_NULL_ARG));

    dynamic_array_status_t status = dynamic_array_pop_back(&stack->storage);
    SA_RETURN(stack_array_map_dynamic_status(status));
}

stack_array_status_t stack_array_size(const stack_array *stack, size_t *out_size) {
    SOFT_ASSERT_FUNCTIONAL(stack != NULL && out_size != NULL, "Size arguments must not be NULL",
                           SA_RETURN(STACK_ARRAY_STATUS_NULL_ARG));

    dynamic_array_status_t status = dynamic_array_size(&stack->storage, out_size);
    SA_RETURN(stack_array_map_dynamic_status(status));
}

stack_array_status_t stack_array_dtr(stack_array **stack) {
    SOFT_ASSERT_FUNCTIONAL(stack != NULL, "Stack output pointer must not be NULL",
                           SA_RETURN(STACK_ARRAY_STATUS_NULL_ARG));
    if (*stack == NULL) {
        SA_RETURN(STACK_ARRAY_STATUS_OK);
    }

    stack_array *object = *stack;
    dynamic_array_status_t status = dynamic_array_dtor(&object->storage);
    if (status != DYNAMIC_ARRAY_STATUS_OK) {
        SA_RETURN(stack_array_map_dynamic_status(status));
    }

    free(object);
    *stack = NULL;
    SA_RETURN(STACK_ARRAY_STATUS_OK);
}
