#include "singly_linked_list.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "asserts.h"
#include "logger.h"

#define SLL_RETURN(status_value)                                                           \
    do {                                                                                   \
        singly_linked_list_status_t status_to_return = (status_value);                     \
        if (status_to_return != SINGLY_LINKED_LIST_STATUS_OK) {                            \
            LOGGER_ERROR("singly_linked_list return status=%d", (int)status_to_return);  \
        }                                                                                  \
        return status_to_return;                                                           \
    } while (0)

struct singly_linked_list_node {
    singly_linked_list_node_t *next;
    unsigned char payload[];
};

static singly_linked_list_status_t safe_add_size(size_t left, size_t right, size_t *result) {
    SOFT_ASSERT_FUNCTIONAL(result != NULL, "Result pointer must not be NULL",
                           SLL_RETURN(SINGLY_LINKED_LIST_STATUS_NULL_ARG));
    if (left > SIZE_MAX - right) {
        SLL_RETURN(SINGLY_LINKED_LIST_STATUS_OVERFLOW);
    }

    *result = left + right;
    SLL_RETURN(SINGLY_LINKED_LIST_STATUS_OK);
}

singly_linked_list_status_t singly_linked_list_ctor(singly_linked_list_t *list, size_t element_size) {
    SOFT_ASSERT_FUNCTIONAL(list != NULL, "List pointer must not be NULL",
                           SLL_RETURN(SINGLY_LINKED_LIST_STATUS_NULL_ARG));
    if (element_size == 0U) {
        SLL_RETURN(SINGLY_LINKED_LIST_STATUS_INVALID_ARG);
    }

    singly_linked_list_t created = {
        .head = NULL,
        .size = 0U,
        .element_size = element_size
    };

    *list = created;
    SLL_RETURN(SINGLY_LINKED_LIST_STATUS_OK);
}

singly_linked_list_status_t singly_linked_list_dtor(singly_linked_list_t *list) {
    SOFT_ASSERT_FUNCTIONAL(list != NULL, "List pointer must not be NULL",
                           SLL_RETURN(SINGLY_LINKED_LIST_STATUS_NULL_ARG));

    while (list->head != NULL) {
        singly_linked_list_node_t *node = list->head;
        list->head = node->next;
        free(node);
    }

    list->size = 0U;
    list->element_size = 0U;
    SLL_RETURN(SINGLY_LINKED_LIST_STATUS_OK);
}

singly_linked_list_status_t singly_linked_list_push_front(singly_linked_list_t *list, const void *element) {
    SOFT_ASSERT_FUNCTIONAL(list != NULL && element != NULL, "Push arguments must not be NULL",
                           SLL_RETURN(SINGLY_LINKED_LIST_STATUS_NULL_ARG));
    if (list->element_size == 0U) {
        SLL_RETURN(SINGLY_LINKED_LIST_STATUS_BAD_STATE);
    }

    size_t node_bytes = 0U;
    singly_linked_list_status_t status = safe_add_size(sizeof(singly_linked_list_node_t),
                                                       list->element_size, &node_bytes);
    if (status != SINGLY_LINKED_LIST_STATUS_OK) {
        SLL_RETURN(status);
    }

    singly_linked_list_node_t *new_node = (singly_linked_list_node_t *)calloc(1U, node_bytes);
    if (new_node == NULL) {
        SLL_RETURN(SINGLY_LINKED_LIST_STATUS_ALLOC_FAIL);
    }

    memcpy(new_node->payload, element, list->element_size);
    new_node->next = list->head;

    list->head = new_node;
    list->size += 1U;
    SLL_RETURN(SINGLY_LINKED_LIST_STATUS_OK);
}

singly_linked_list_status_t singly_linked_list_front(const singly_linked_list_t *list, void *out_element) {
    SOFT_ASSERT_FUNCTIONAL(list != NULL && out_element != NULL, "Front arguments must not be NULL",
                           SLL_RETURN(SINGLY_LINKED_LIST_STATUS_NULL_ARG));
    if (list->element_size == 0U) {
        SLL_RETURN(SINGLY_LINKED_LIST_STATUS_BAD_STATE);
    }
    if (list->head == NULL) {
        SLL_RETURN(SINGLY_LINKED_LIST_STATUS_EMPTY);
    }

    memcpy(out_element, list->head->payload, list->element_size);
    SLL_RETURN(SINGLY_LINKED_LIST_STATUS_OK);
}

singly_linked_list_status_t singly_linked_list_pop_front(singly_linked_list_t *list) {
    SOFT_ASSERT_FUNCTIONAL(list != NULL, "List pointer must not be NULL",
                           SLL_RETURN(SINGLY_LINKED_LIST_STATUS_NULL_ARG));
    if (list->element_size == 0U) {
        SLL_RETURN(SINGLY_LINKED_LIST_STATUS_BAD_STATE);
    }
    if (list->head == NULL) {
        SLL_RETURN(SINGLY_LINKED_LIST_STATUS_EMPTY);
    }

    singly_linked_list_node_t *node = list->head;
    list->head = node->next;
    free(node);
    list->size -= 1U;
    SLL_RETURN(SINGLY_LINKED_LIST_STATUS_OK);
}

singly_linked_list_status_t singly_linked_list_size(const singly_linked_list_t *list, size_t *out_size) {
    SOFT_ASSERT_FUNCTIONAL(list != NULL && out_size != NULL, "Size arguments must not be NULL",
                           SLL_RETURN(SINGLY_LINKED_LIST_STATUS_NULL_ARG));
    if (list->element_size == 0U) {
        SLL_RETURN(SINGLY_LINKED_LIST_STATUS_BAD_STATE);
    }

    *out_size = list->size;
    SLL_RETURN(SINGLY_LINKED_LIST_STATUS_OK);
}
