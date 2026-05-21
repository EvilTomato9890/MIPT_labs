/**
 * @file singly_linked_list.h
 * @brief Generic singly linked list with push/pop operations at the front.
 */

#ifndef LAB1_SINGLY_LINKED_LIST_INCLUDE_SINGLY_LINKED_LIST_H_INCLUDED
#define LAB1_SINGLY_LINKED_LIST_INCLUDE_SINGLY_LINKED_LIST_H_INCLUDED

#include <stddef.h>

/**
 * @brief Status codes returned by singly linked list operations.
 */
typedef enum singly_linked_list_status {
    /** Operation completed successfully. */
    SINGLY_LINKED_LIST_STATUS_OK = 0,
    /** A required pointer argument was NULL. */
    SINGLY_LINKED_LIST_STATUS_NULL_ARG,
    /** An argument value is invalid. */
    SINGLY_LINKED_LIST_STATUS_INVALID_ARG,
    /** Size calculation overflowed. */
    SINGLY_LINKED_LIST_STATUS_OVERFLOW,
    /** Memory allocation failed. */
    SINGLY_LINKED_LIST_STATUS_ALLOC_FAIL,
    /** Operation requires a non-empty list. */
    SINGLY_LINKED_LIST_STATUS_EMPTY,
    /** List object is not initialized or is corrupted. */
    SINGLY_LINKED_LIST_STATUS_BAD_STATE
} singly_linked_list_status_t;

/**
 * @brief Opaque list node type.
 */
typedef struct singly_linked_list_node singly_linked_list_node_t;

/**
 * @brief Generic singly linked list object.
 */
typedef struct singly_linked_list {
    /** First node of the list. */
    singly_linked_list_node_t *head;
    /** Current number of elements. */
    size_t size;
    /** Size of one element in bytes. */
    size_t element_size;
} singly_linked_list_t;

/**
 * @brief Constructs an empty singly linked list.
 *
 * @param list List object to initialize.
 * @param element_size Size of one element in bytes.
 * @return Operation status.
 */
singly_linked_list_status_t singly_linked_list_ctor       (singly_linked_list_t *list, size_t element_size);

/**
 * @brief Destroys all nodes and clears the list fields.
 *
 * @param list List object to destroy.
 * @return Operation status.
 */
singly_linked_list_status_t singly_linked_list_dtor       (singly_linked_list_t *list);

/**
 * @brief Inserts an element at the front of the list.
 *
 * @param list Initialized list.
 * @param element Pointer to the element bytes to copy.
 * @return Operation status.
 */
singly_linked_list_status_t singly_linked_list_push_front (singly_linked_list_t *list, const void *element);

/**
 * @brief Copies the front element without removing it.
 *
 * @param list Initialized list.
 * @param out_element Destination buffer for the copied element.
 * @return Operation status.
 */
singly_linked_list_status_t singly_linked_list_front(const singly_linked_list_t *list,       void *out_element);

/**
 * @brief Removes the front element.
 *
 * @param list Initialized list.
 * @return Operation status.
 */
singly_linked_list_status_t singly_linked_list_pop_front  (singly_linked_list_t *list);

/**
 * @brief Returns the current number of elements.
 *
 * @param list Initialized list.
 * @param out_size Destination for the size value.
 * @return Operation status.
 */
singly_linked_list_status_t singly_linked_list_size (const singly_linked_list_t *list, size_t *out_size);

#endif /* LAB1_SINGLY_LINKED_LIST_INCLUDE_SINGLY_LINKED_LIST_H_INCLUDED */
