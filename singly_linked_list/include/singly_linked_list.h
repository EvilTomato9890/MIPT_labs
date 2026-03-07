#ifndef LAB1_SINGLY_LINKED_LIST_INCLUDE_SINGLY_LINKED_LIST_H_INCLUDED
#define LAB1_SINGLY_LINKED_LIST_INCLUDE_SINGLY_LINKED_LIST_H_INCLUDED

#include <stddef.h>

typedef enum singly_linked_list_status {
    SINGLY_LINKED_LIST_STATUS_OK = 0,
    SINGLY_LINKED_LIST_STATUS_NULL_ARG,
    SINGLY_LINKED_LIST_STATUS_INVALID_ARG,
    SINGLY_LINKED_LIST_STATUS_OVERFLOW,
    SINGLY_LINKED_LIST_STATUS_ALLOC_FAIL,
    SINGLY_LINKED_LIST_STATUS_EMPTY,
    SINGLY_LINKED_LIST_STATUS_BAD_STATE
} singly_linked_list_status_t;

typedef struct singly_linked_list_node singly_linked_list_node_t;

typedef struct singly_linked_list {
    singly_linked_list_node_t *head;
    size_t size;
    size_t element_size;
} singly_linked_list_t;

singly_linked_list_status_t singly_linked_list_ctor       (singly_linked_list_t *list, size_t element_size);
singly_linked_list_status_t singly_linked_list_dtor       (singly_linked_list_t *list);
singly_linked_list_status_t singly_linked_list_push_front (singly_linked_list_t *list, const void *element);
singly_linked_list_status_t singly_linked_list_front(const singly_linked_list_t *list,       void *out_element);
singly_linked_list_status_t singly_linked_list_pop_front  (singly_linked_list_t *list);
singly_linked_list_status_t singly_linked_list_size (const singly_linked_list_t *list, size_t *out_size);

#endif /* LAB1_SINGLY_LINKED_LIST_INCLUDE_SINGLY_LINKED_LIST_H_INCLUDED */
