#include <stdio.h>

#include "dynamic_array.h"
#include "singly_linked_list.h"
#include "stack_array.h"
#include "stack_list.h"

#define EXPECT_TRUE(condition)                                                             \
    do {                                                                                   \
        if (!(condition)) {                                                                 \
            fprintf(stderr, "%s:%d: expected %s\n", __FILE__, __LINE__, #condition);       \
            return 1;                                                                      \
        }                                                                                  \
    } while (0)

#define EXPECT_STATUS(actual, expected)                                                     \
    do {                                                                                   \
        int actual_value = (int)(actual);                                                   \
        int expected_value = (int)(expected);                                               \
        if (actual_value != expected_value) {                                               \
            fprintf(stderr, "%s:%d: expected status %d, got %d\n",                         \
                    __FILE__, __LINE__, expected_value, actual_value);                      \
            return 1;                                                                      \
        }                                                                                  \
    } while (0)

#define EXPECT_SIZE(actual, expected)                                                       \
    do {                                                                                   \
        size_t actual_value = (actual);                                                     \
        size_t expected_value = (expected);                                                 \
        if (actual_value != expected_value) {                                               \
            fprintf(stderr, "%s:%d: expected size %llu, got %llu\n",                       \
                    __FILE__, __LINE__,                                                     \
                    (unsigned long long)expected_value,                                     \
                    (unsigned long long)actual_value);                                      \
            return 1;                                                                      \
        }                                                                                  \
    } while (0)

static int test_dynamic_array(void) {
    dynamic_array_t array = {0};
    size_t size = 0U;
    int value = 0;
    int out_value = 0;

    EXPECT_STATUS(dynamic_array_ctor(&array, 1U, sizeof(value)), DYNAMIC_ARRAY_STATUS_OK);
    EXPECT_STATUS(dynamic_array_size(&array, &size), DYNAMIC_ARRAY_STATUS_OK);
    EXPECT_SIZE(size, 0U);

    for (value = 1; value <= 5; ++value) {
        EXPECT_STATUS(dynamic_array_push_back(&array, &value), DYNAMIC_ARRAY_STATUS_OK);
    }

    EXPECT_STATUS(dynamic_array_size(&array, &size), DYNAMIC_ARRAY_STATUS_OK);
    EXPECT_SIZE(size, 5U);
    EXPECT_TRUE(array.capacity >= 5U);

    EXPECT_STATUS(dynamic_array_back(&array, &out_value), DYNAMIC_ARRAY_STATUS_OK);
    EXPECT_TRUE(out_value == 5);
    EXPECT_STATUS(dynamic_array_pop_back(&array), DYNAMIC_ARRAY_STATUS_OK);
    EXPECT_STATUS(dynamic_array_back(&array, &out_value), DYNAMIC_ARRAY_STATUS_OK);
    EXPECT_TRUE(out_value == 4);

    EXPECT_STATUS(dynamic_array_dtor(&array), DYNAMIC_ARRAY_STATUS_OK);
    EXPECT_TRUE(array.data == NULL);
    EXPECT_SIZE(array.size, 0U);
    return 0;
}

static int test_singly_linked_list(void) {
    singly_linked_list_t list = {0};
    size_t size = 0U;
    int value = 0;
    int out_value = 0;

    EXPECT_STATUS(singly_linked_list_ctor(&list, sizeof(value)), SINGLY_LINKED_LIST_STATUS_OK);
    EXPECT_STATUS(singly_linked_list_size(&list, &size), SINGLY_LINKED_LIST_STATUS_OK);
    EXPECT_SIZE(size, 0U);

    for (value = 1; value <= 5; ++value) {
        EXPECT_STATUS(singly_linked_list_push_front(&list, &value), SINGLY_LINKED_LIST_STATUS_OK);
    }

    EXPECT_STATUS(singly_linked_list_size(&list, &size), SINGLY_LINKED_LIST_STATUS_OK);
    EXPECT_SIZE(size, 5U);
    EXPECT_STATUS(singly_linked_list_front(&list, &out_value), SINGLY_LINKED_LIST_STATUS_OK);
    EXPECT_TRUE(out_value == 5);
    EXPECT_STATUS(singly_linked_list_pop_front(&list), SINGLY_LINKED_LIST_STATUS_OK);
    EXPECT_STATUS(singly_linked_list_front(&list, &out_value), SINGLY_LINKED_LIST_STATUS_OK);
    EXPECT_TRUE(out_value == 4);

    EXPECT_STATUS(singly_linked_list_dtor(&list), SINGLY_LINKED_LIST_STATUS_OK);
    EXPECT_TRUE(list.head == NULL);
    EXPECT_SIZE(list.size, 0U);
    return 0;
}

static int test_stack_array(void) {
    stack_array *stack = NULL;
    size_t size = 0U;
    int value = 0;
    int out_value = 0;

    EXPECT_STATUS(stack_array_ctr(0U, sizeof(value), &stack), STACK_ARRAY_STATUS_OK);
    EXPECT_TRUE(stack != NULL);
    EXPECT_STATUS(stack_array_size(stack, &size), STACK_ARRAY_STATUS_OK);
    EXPECT_SIZE(size, 0U);

    for (value = 10; value <= 14; ++value) {
        EXPECT_STATUS(stack_array_push(stack, &value), STACK_ARRAY_STATUS_OK);
    }

    EXPECT_STATUS(stack_array_size(stack, &size), STACK_ARRAY_STATUS_OK);
    EXPECT_SIZE(size, 5U);

    for (value = 14; value >= 10; --value) {
        EXPECT_STATUS(stack_array_top(stack, &out_value), STACK_ARRAY_STATUS_OK);
        EXPECT_TRUE(out_value == value);
        EXPECT_STATUS(stack_array_pop(stack), STACK_ARRAY_STATUS_OK);
    }

    EXPECT_STATUS(stack_array_size(stack, &size), STACK_ARRAY_STATUS_OK);
    EXPECT_SIZE(size, 0U);
    EXPECT_STATUS(stack_array_dtr(&stack), STACK_ARRAY_STATUS_OK);
    EXPECT_TRUE(stack == NULL);
    EXPECT_STATUS(stack_array_dtr(&stack), STACK_ARRAY_STATUS_OK);
    return 0;
}

static int test_stack_list(void) {
    stack_list *stack = NULL;
    size_t size = 0U;
    int value = 0;
    int out_value = 0;

    EXPECT_STATUS(stack_list_ctor(0U, sizeof(value), &stack), STACK_LIST_STATUS_OK);
    EXPECT_TRUE(stack != NULL);
    EXPECT_STATUS(stack_list_size(stack, &size), STACK_LIST_STATUS_OK);
    EXPECT_SIZE(size, 0U);

    for (value = 10; value <= 14; ++value) {
        EXPECT_STATUS(stack_list_push(stack, &value), STACK_LIST_STATUS_OK);
    }

    EXPECT_STATUS(stack_list_size(stack, &size), STACK_LIST_STATUS_OK);
    EXPECT_SIZE(size, 5U);

    for (value = 14; value >= 10; --value) {
        EXPECT_STATUS(stack_list_top(stack, &out_value), STACK_LIST_STATUS_OK);
        EXPECT_TRUE(out_value == value);
        EXPECT_STATUS(stack_list_pop(stack), STACK_LIST_STATUS_OK);
    }

    EXPECT_STATUS(stack_list_size(stack, &size), STACK_LIST_STATUS_OK);
    EXPECT_SIZE(size, 0U);
    EXPECT_STATUS(stack_list_dtor(&stack), STACK_LIST_STATUS_OK);
    EXPECT_TRUE(stack == NULL);
    EXPECT_STATUS(stack_list_dtor(&stack), STACK_LIST_STATUS_OK);
    return 0;
}

int main(void) {
    int failed = 0;

    failed += test_dynamic_array();
    failed += test_singly_linked_list();
    failed += test_stack_array();
    failed += test_stack_list();

    if (failed != 0) {
        fprintf(stderr, "API tests failed: %d\n", failed);
        return 1;
    }

    puts("API tests passed");
    return 0;
}
