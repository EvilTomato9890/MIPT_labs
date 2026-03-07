#include "dynamic_array.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "asserts.h"
#include "logger.h"

#define DA_RETURN(status_value)                                                           \
    do {                                                                                  \
        dynamic_array_status_t status_to_return = (status_value);                         \
        if (status_to_return != DYNAMIC_ARRAY_STATUS_OK) {                                \
            LOGGER_ERROR("dynamic_array return status=%d", (int)status_to_return);      \
        }                                                                                 \
        return status_to_return;                                                          \
    } while (0)

static dynamic_array_status_t safe_multiply_size(size_t left, size_t right, size_t *result) {
    SOFT_ASSERT_FUNCTIONAL(result != NULL, "Result pointer must not be NULL",
                           DA_RETURN(DYNAMIC_ARRAY_STATUS_NULL_ARG));
    if (left == 0U || right == 0U) {
        *result = 0U;
        DA_RETURN(DYNAMIC_ARRAY_STATUS_OK);
    }
    if (left > SIZE_MAX / right) {
        DA_RETURN(DYNAMIC_ARRAY_STATUS_OVERFLOW);
    }

    *result = left * right;
    DA_RETURN(DYNAMIC_ARRAY_STATUS_OK);
}

static dynamic_array_status_t dynamic_array_resize(dynamic_array_t *array, size_t new_capacity) {
    SOFT_ASSERT_FUNCTIONAL(array != NULL, "Array pointer must not be NULL",
                           DA_RETURN(DYNAMIC_ARRAY_STATUS_NULL_ARG));
    if (array->data == NULL || array->element_size == 0U || array->capacity == 0U) {
        DA_RETURN(DYNAMIC_ARRAY_STATUS_BAD_STATE);
    }
    if (new_capacity < array->size || new_capacity == 0U) {
        DA_RETURN(DYNAMIC_ARRAY_STATUS_INVALID_ARG);
    }

    size_t new_bytes = 0U;
    dynamic_array_status_t status = safe_multiply_size(new_capacity, array->element_size, &new_bytes);
    if (status != DYNAMIC_ARRAY_STATUS_OK) {
        DA_RETURN(status);
    }

    unsigned char *new_block = (unsigned char *)realloc(array->data, new_bytes);
    if (new_block == NULL) {
        DA_RETURN(DYNAMIC_ARRAY_STATUS_ALLOC_FAIL);
    }

    array->data = new_block;
    array->capacity = new_capacity;
    DA_RETURN(DYNAMIC_ARRAY_STATUS_OK);
}

dynamic_array_status_t dynamic_array_ctor(dynamic_array_t *array, size_t initial_capacity, size_t element_size) {
    SOFT_ASSERT_FUNCTIONAL(array != NULL, "Array pointer must not be NULL",
                           DA_RETURN(DYNAMIC_ARRAY_STATUS_NULL_ARG));
    if (element_size == 0U) {
        DA_RETURN(DYNAMIC_ARRAY_STATUS_INVALID_ARG);
    }

    size_t normalized_capacity = (initial_capacity == 0U) ? 1U : initial_capacity;
    size_t bytes_count = 0U;
    dynamic_array_status_t status = safe_multiply_size(normalized_capacity, element_size, &bytes_count);
    if (status != DYNAMIC_ARRAY_STATUS_OK) {
        DA_RETURN(status);
    }

    unsigned char *data = (unsigned char *)calloc(1U, bytes_count);
    if (data == NULL) {
        DA_RETURN(DYNAMIC_ARRAY_STATUS_ALLOC_FAIL);
    }

    dynamic_array_t created = {
        .data = data,
        .size = 0U,
        .capacity = normalized_capacity,
        .minimum_capacity = normalized_capacity,
        .element_size = element_size
    };

    *array = created;
    DA_RETURN(DYNAMIC_ARRAY_STATUS_OK);
}

dynamic_array_status_t dynamic_array_dtor(dynamic_array_t *array) {
    SOFT_ASSERT_FUNCTIONAL(array != NULL, "Array pointer must not be NULL",
                           DA_RETURN(DYNAMIC_ARRAY_STATUS_NULL_ARG));

    free(array->data);
    array->data             = NULL;
    array->size             = 0U;
    array->capacity         = 0U;
    array->minimum_capacity = 0U;
    array->element_size     = 0U;
    DA_RETURN(DYNAMIC_ARRAY_STATUS_OK);
}

dynamic_array_status_t dynamic_array_push_back(dynamic_array_t *array, const void *element) {
    SOFT_ASSERT_FUNCTIONAL(array != NULL && element != NULL, "Push arguments must not be NULL",
                           DA_RETURN(DYNAMIC_ARRAY_STATUS_NULL_ARG));
    if (array->data == NULL || array->element_size == 0U || array->capacity == 0U) {
        DA_RETURN(DYNAMIC_ARRAY_STATUS_BAD_STATE);
    }

    if (array->size == array->capacity) {
        if (array->capacity > SIZE_MAX / 2U) {
            DA_RETURN(DYNAMIC_ARRAY_STATUS_OVERFLOW);
        }

        dynamic_array_status_t resize_status = dynamic_array_resize(array, array->capacity * 2U);
        if (resize_status != DYNAMIC_ARRAY_STATUS_OK) {
            DA_RETURN(resize_status);
        }
    }

    memcpy(array->data + array->size * array->element_size, element, array->element_size);
    array->size += 1U;
    DA_RETURN(DYNAMIC_ARRAY_STATUS_OK);
}

dynamic_array_status_t dynamic_array_back(const dynamic_array_t *array, void *out_element) {
    SOFT_ASSERT_FUNCTIONAL(array != NULL && out_element != NULL, "Back arguments must not be NULL",
                           DA_RETURN(DYNAMIC_ARRAY_STATUS_NULL_ARG));
    if (array->data == NULL || array->element_size == 0U || array->capacity == 0U) {
        DA_RETURN(DYNAMIC_ARRAY_STATUS_BAD_STATE);
    }
    if (array->size == 0U) {
        DA_RETURN(DYNAMIC_ARRAY_STATUS_EMPTY);
    }

    memcpy(out_element, array->data + (array->size - 1U) * array->element_size, array->element_size);
    DA_RETURN(DYNAMIC_ARRAY_STATUS_OK);
}

dynamic_array_status_t dynamic_array_pop_back(dynamic_array_t *array) {
    SOFT_ASSERT_FUNCTIONAL(array != NULL, "Array pointer must not be NULL",
                           DA_RETURN(DYNAMIC_ARRAY_STATUS_NULL_ARG));
    if (array->data == NULL || array->element_size == 0U || array->capacity == 0U) {
        DA_RETURN(DYNAMIC_ARRAY_STATUS_BAD_STATE);
    }
    if (array->size == 0U) {
        DA_RETURN(DYNAMIC_ARRAY_STATUS_EMPTY);
    }

    size_t old_size = array->size;
    size_t new_size = old_size - 1U;

    if (array->capacity > array->minimum_capacity && new_size <= array->capacity / 4U) {
        size_t new_capacity = array->capacity / 2U;
        if (new_capacity < array->minimum_capacity) {
            new_capacity = array->minimum_capacity;
        }

        if (new_capacity != array->capacity) {
            dynamic_array_status_t resize_status = dynamic_array_resize(array, new_capacity);
            if (resize_status != DYNAMIC_ARRAY_STATUS_OK) {
                DA_RETURN(resize_status);
            }
        }
    }

    array->size = new_size;
    DA_RETURN(DYNAMIC_ARRAY_STATUS_OK);
}

dynamic_array_status_t dynamic_array_size(const dynamic_array_t *array, size_t *out_size) {
    SOFT_ASSERT_FUNCTIONAL(array != NULL && out_size != NULL, "Size arguments must not be NULL",
                           DA_RETURN(DYNAMIC_ARRAY_STATUS_NULL_ARG));
    if (array->data == NULL || array->element_size == 0U || array->capacity == 0U) {
        DA_RETURN(DYNAMIC_ARRAY_STATUS_BAD_STATE);
    }

    *out_size = array->size;
    DA_RETURN(DYNAMIC_ARRAY_STATUS_OK);
}
