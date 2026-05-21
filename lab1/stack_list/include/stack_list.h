/**
 * @file stack_list.h
 * @brief Stack implementation backed by the singly linked list.
 */

#ifndef LAB1_STACK_LIST_INCLUDE_STACK_LIST_H_NCLUDED
#define LAB1_STACK_LIST_INCLUDE_STACK_LIST_H_NCLUDED

#include <stddef.h>

#include "singly_linked_list.h"

/**
 * @brief Status codes returned by list-backed stack operations.
 */
typedef enum stack_list_status {
    /** Operation completed successfully. */
    STACK_LIST_STATUS_OK = 0,
    /** A required pointer argument was NULL. */
    STACK_LIST_STATUS_NULL_ARG,
    /** An argument value is invalid. */
    STACK_LIST_STATUS_INVALID_ARG,
    /** Memory allocation failed. */
    STACK_LIST_STATUS_ALLOC_FAIL,
    /** Operation requires a non-empty stack. */
    STACK_LIST_STATUS_EMPTY,
    /** Stack object is not initialized or is corrupted. */
    STACK_LIST_STATUS_BAD_STATE,
    /** Unexpected error from an internal dependency. */
    STACK_LIST_STATUS_INTERNAL_ERROR
} stack_list_status_t;

/**
 * @brief Stack object that stores elements in a singly linked list.
 */
typedef struct stack_list {
    /** Underlying singly linked list storage. */
    singly_linked_list_t storage;
} stack_list;

/**
 * @brief Constructs a list-backed stack.
 *
 * @param size Initial capacity hint. Ignored for the list-backed stack.
 * @param element_size Size of one element in bytes.
 * @param out_stack Destination for the allocated stack pointer.
 * @return Operation status.
 */
stack_list_status_t stack_list_ctor(size_t size, size_t element_size, stack_list **out_stack);

/**
 * @brief Pushes an element onto the stack.
 *
 * @param stack Initialized stack.
 * @param buffer Pointer to the element bytes to copy.
 * @return Operation status.
 */
stack_list_status_t stack_list_push      (stack_list *stack, const void *buffer);

/**
 * @brief Copies the top element without removing it.
 *
 * @param stack Initialized stack.
 * @param buffer Destination buffer for the copied element.
 * @return Operation status.
 */
stack_list_status_t stack_list_top (const stack_list *stack,       void *buffer);

/**
 * @brief Removes the top element.
 *
 * @param stack Initialized stack.
 * @return Operation status.
 */
stack_list_status_t stack_list_pop       (stack_list *stack);

/**
 * @brief Returns the number of elements in the stack.
 *
 * @param stack Initialized stack.
 * @param out_size Destination for the size value.
 * @return Operation status.
 */
stack_list_status_t stack_list_size(const stack_list *stack, size_t *out_size);

/**
 * @brief Destroys a list-backed stack and sets the pointer to NULL.
 *
 * @param stack Pointer to the stack pointer.
 * @return Operation status.
 */
stack_list_status_t stack_list_dtor      (stack_list **stack);

#endif /* LAB1_STACK_LIST_INCLUDE_STACK_LIST_H_NCLUDED */
