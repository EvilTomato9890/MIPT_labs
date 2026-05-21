/**
 * @file stack_array.h
 * @brief Stack implementation backed by the dynamic array.
 */

#ifndef LAB1_STACK_ARRAY_INCLUDE_STACK_ARRAY_H_NCLUDED
#define LAB1_STACK_ARRAY_INCLUDE_STACK_ARRAY_H_NCLUDED

#include <stddef.h>

#include "dynamic_array.h"

/**
 * @brief Status codes returned by array-backed stack operations.
 */
typedef enum stack_array_status {
    /** Operation completed successfully. */
    STACK_ARRAY_STATUS_OK = 0,
    /** A required pointer argument was NULL. */
    STACK_ARRAY_STATUS_NULL_ARG,
    /** An argument value is invalid. */
    STACK_ARRAY_STATUS_INVALID_ARG,
    /** Memory allocation failed. */
    STACK_ARRAY_STATUS_ALLOC_FAIL,
    /** Operation requires a non-empty stack. */
    STACK_ARRAY_STATUS_EMPTY,
    /** Stack object is not initialized or is corrupted. */
    STACK_ARRAY_STATUS_BAD_STATE,
    /** Unexpected error from an internal dependency. */
    STACK_ARRAY_STATUS_INTERNAL_ERROR
} stack_array_status_t;

/**
 * @brief Stack object that stores elements in a dynamic array.
 */
typedef struct stack_array {
    /** Underlying dynamic array storage. */
    dynamic_array_t storage;
} stack_array;

/**
 * @brief Constructs an array-backed stack.
 *
 * @param size Initial capacity hint.
 * @param element_size Size of one element in bytes.
 * @param out_stack Destination for the allocated stack pointer.
 * @return Operation status.
 */
stack_array_status_t stack_array_ctr(size_t size, size_t element_size, stack_array **out_stack);

/**
 * @brief Pushes an element onto the stack.
 *
 * @param stack Initialized stack.
 * @param buffer Pointer to the element bytes to copy.
 * @return Operation status.
 */
stack_array_status_t stack_array_push      (stack_array *stack, const void *buffer);

/**
 * @brief Copies the top element without removing it.
 *
 * @param stack Initialized stack.
 * @param buffer Destination buffer for the copied element.
 * @return Operation status.
 */
stack_array_status_t stack_array_top (const stack_array *stack,       void *buffer);

/**
 * @brief Removes the top element.
 *
 * @param stack Initialized stack.
 * @return Operation status.
 */
stack_array_status_t stack_array_pop       (stack_array *stack);

/**
 * @brief Returns the number of elements in the stack.
 *
 * @param stack Initialized stack.
 * @param out_size Destination for the size value.
 * @return Operation status.
 */
stack_array_status_t stack_array_size(const stack_array *stack, size_t *out_size);

/**
 * @brief Destroys an array-backed stack and sets the pointer to NULL.
 *
 * @param stack Pointer to the stack pointer.
 * @return Operation status.
 */
stack_array_status_t stack_array_dtr       (stack_array **stack);

#endif /* LAB1_STACK_ARRAY_INCLUDE_STACK_ARRAY_H_NCLUDED */
