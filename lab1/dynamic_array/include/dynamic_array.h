/**
 * @file dynamic_array.h
 * @brief Generic dynamic array with automatic capacity growth and shrinkage.
 */

#ifndef LAB1_DYNAMIC_ARRAY_INCLUDE_DYNAMIC_ARRAY_H_INCLUDED
#define LAB1_DYNAMIC_ARRAY_INCLUDE_DYNAMIC_ARRAY_H_INCLUDED

#include <stddef.h>

/**
 * @brief Status codes returned by dynamic array operations.
 */
typedef enum dynamic_array_status {
    /** Operation completed successfully. */
    DYNAMIC_ARRAY_STATUS_OK = 0,
    /** A required pointer argument was NULL. */
    DYNAMIC_ARRAY_STATUS_NULL_ARG,
    /** An argument value is invalid. */
    DYNAMIC_ARRAY_STATUS_INVALID_ARG,
    /** Size calculation overflowed. */
    DYNAMIC_ARRAY_STATUS_OVERFLOW,
    /** Memory allocation failed. */
    DYNAMIC_ARRAY_STATUS_ALLOC_FAIL,
    /** Operation requires a non-empty array. */
    DYNAMIC_ARRAY_STATUS_EMPTY,
    /** Array object is not initialized or is corrupted. */
    DYNAMIC_ARRAY_STATUS_BAD_STATE
} dynamic_array_status_t;

/**
 * @brief Generic dynamic array storage.
 */
typedef struct dynamic_array {
    /** Raw byte buffer with elements stored sequentially. */
    unsigned char *data;
    /** Current number of elements. */
    size_t size;
    /** Allocated element capacity. */
    size_t capacity;
    /** Minimum capacity kept after shrinking. */
    size_t minimum_capacity;
    /** Size of one element in bytes. */
    size_t element_size;
} dynamic_array_t;

/**
 * @brief Constructs a dynamic array.
 *
 * @param array Array object to initialize.
 * @param initial_capacity Initial capacity. Zero is normalized to the default capacity.
 * @param element_size Size of one element in bytes.
 * @return Operation status.
 */
dynamic_array_status_t dynamic_array_ctor      (dynamic_array_t *array, size_t initial_capacity, size_t element_size);

/**
 * @brief Destroys a dynamic array and clears its fields.
 *
 * @param array Array object to destroy.
 * @return Operation status.
 */
dynamic_array_status_t dynamic_array_dtor      (dynamic_array_t *array);

/**
 * @brief Appends an element to the end of the array.
 *
 * @param array Initialized array.
 * @param element Pointer to the element bytes to copy.
 * @return Operation status.
 */
dynamic_array_status_t dynamic_array_push_back (dynamic_array_t *array, const void *element);

/**
 * @brief Copies the last element without removing it.
 *
 * @param array Initialized array.
 * @param out_element Destination buffer for the copied element.
 * @return Operation status.
 */
dynamic_array_status_t dynamic_array_back(const dynamic_array_t *array,       void *out_element);

/**
 * @brief Removes the last element from the array.
 *
 * @param array Initialized array.
 * @return Operation status.
 */
dynamic_array_status_t dynamic_array_pop_back  (dynamic_array_t *array);

/**
 * @brief Returns the current number of elements.
 *
 * @param array Initialized array.
 * @param out_size Destination for the size value.
 * @return Operation status.
 */
dynamic_array_status_t dynamic_array_size(const dynamic_array_t *array, size_t *out_size);

#endif /* LAB1_DYNAMIC_ARRAY_INCLUDE_DYNAMIC_ARRAY_H_INCLUDED */
