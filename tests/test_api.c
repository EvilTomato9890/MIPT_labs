#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "benchmark.h"
#include "stack_array.h"
#include "stack_list.h"

struct pair_value {
    int first;
    int second;
};

static int test_array_stack_with_int(void) {
    stack_array *stack = NULL;
    stack_array_status_t ctor_status = stack_array_ctr(4U, sizeof(int), &stack);
    if (ctor_status != STACK_ARRAY_STATUS_OK || stack == NULL) {
        return 0;
    }

    int value = 42;
    if (stack_array_push(stack, &value) != STACK_ARRAY_STATUS_OK) {
        (void)stack_array_dtr(&stack);
        return 0;
    }

    int top_value = 0;
    if (stack_array_top(stack, &top_value) != STACK_ARRAY_STATUS_OK || top_value != 42) {
        (void)stack_array_dtr(&stack);
        return 0;
    }

    if (stack_array_pop(stack) != STACK_ARRAY_STATUS_OK) {
        (void)stack_array_dtr(&stack);
        return 0;
    }
    if (stack_array_pop(stack) != STACK_ARRAY_STATUS_EMPTY) {
        (void)stack_array_dtr(&stack);
        return 0;
    }

    return stack_array_dtr(&stack) == STACK_ARRAY_STATUS_OK && stack == NULL;
}

static int test_array_stack_with_struct(void) {
    stack_array *stack = NULL;
    stack_array_status_t ctor_status = stack_array_ctr(2U, sizeof(struct pair_value), &stack);
    if (ctor_status != STACK_ARRAY_STATUS_OK || stack == NULL) {
        return 0;
    }

    struct pair_value input = {13, 37};
    if (stack_array_push(stack, &input) != STACK_ARRAY_STATUS_OK) {
        (void)stack_array_dtr(&stack);
        return 0;
    }

    struct pair_value output = {0, 0};
    if (stack_array_top(stack, &output) != STACK_ARRAY_STATUS_OK) {
        (void)stack_array_dtr(&stack);
        return 0;
    }
    if (memcmp(&input, &output, sizeof(struct pair_value)) != 0) {
        (void)stack_array_dtr(&stack);
        return 0;
    }

    return stack_array_dtr(&stack) == STACK_ARRAY_STATUS_OK && stack == NULL;
}

static int test_array_rollback_on_invalid_push(void) {
    stack_array *stack = NULL;
    if (stack_array_ctr(2U, sizeof(int), &stack) != STACK_ARRAY_STATUS_OK || stack == NULL) {
        return 0;
    }

    int value = 10;
    if (stack_array_push(stack, &value) != STACK_ARRAY_STATUS_OK) {
        (void)stack_array_dtr(&stack);
        return 0;
    }

    size_t before_size = 0U;
    if (stack_array_size(stack, &before_size) != STACK_ARRAY_STATUS_OK) {
        (void)stack_array_dtr(&stack);
        return 0;
    }

    if (stack_array_push(stack, NULL) != STACK_ARRAY_STATUS_NULL_ARG) {
        (void)stack_array_dtr(&stack);
        return 0;
    }

    size_t after_size = 0U;
    if (stack_array_size(stack, &after_size) != STACK_ARRAY_STATUS_OK || after_size != before_size) {
        (void)stack_array_dtr(&stack);
        return 0;
    }

    return stack_array_dtr(&stack) == STACK_ARRAY_STATUS_OK && stack == NULL;
}

static int test_stack_array_ctor_does_not_override_out_on_error(void) {
    stack_array *stack = (stack_array *)(uintptr_t)0x1;
    stack_array_status_t status = stack_array_ctr(2U, 0U, &stack);
    return status == STACK_ARRAY_STATUS_INVALID_ARG && stack == (stack_array *)(uintptr_t)0x1;
}

static int test_list_stack_with_int(void) {
    stack_list *stack = NULL;
    stack_list_status_t ctor_status = stack_list_ctor(8U, sizeof(int), &stack);
    if (ctor_status != STACK_LIST_STATUS_OK || stack == NULL) {
        return 0;
    }

    int first = 10;
    int second = 20;
    if (stack_list_push(stack, &first) != STACK_LIST_STATUS_OK || stack_list_push(stack, &second) != STACK_LIST_STATUS_OK) {
        (void)stack_list_dtor(&stack);
        return 0;
    }

    int top_value = 0;
    if (stack_list_top(stack, &top_value) != STACK_LIST_STATUS_OK || top_value != 20) {
        (void)stack_list_dtor(&stack);
        return 0;
    }

    if (stack_list_pop(stack) != STACK_LIST_STATUS_OK || stack_list_pop(stack) != STACK_LIST_STATUS_OK) {
        (void)stack_list_dtor(&stack);
        return 0;
    }
    if (stack_list_pop(stack) != STACK_LIST_STATUS_EMPTY) {
        (void)stack_list_dtor(&stack);
        return 0;
    }

    return stack_list_dtor(&stack) == STACK_LIST_STATUS_OK && stack == NULL;
}

static int test_list_stack_with_struct(void) {
    stack_list *stack = NULL;
    stack_list_status_t ctor_status = stack_list_ctor(0U, sizeof(struct pair_value), &stack);
    if (ctor_status != STACK_LIST_STATUS_OK || stack == NULL) {
        return 0;
    }

    struct pair_value input = {111, 222};
    if (stack_list_push(stack, &input) != STACK_LIST_STATUS_OK) {
        (void)stack_list_dtor(&stack);
        return 0;
    }

    struct pair_value output = {0, 0};
    if (stack_list_top(stack, &output) != STACK_LIST_STATUS_OK) {
        (void)stack_list_dtor(&stack);
        return 0;
    }
    if (memcmp(&input, &output, sizeof(struct pair_value)) != 0) {
        (void)stack_list_dtor(&stack);
        return 0;
    }

    return stack_list_dtor(&stack) == STACK_LIST_STATUS_OK && stack == NULL;
}

static int test_stack_list_ctor_does_not_override_out_on_error(void) {
    stack_list *stack = (stack_list *)(uintptr_t)0x1;
    stack_list_status_t status = stack_list_ctor(2U, 0U, &stack);
    return status == STACK_LIST_STATUS_INVALID_ARG && stack == (stack_list *)(uintptr_t)0x1;
}

static int test_benchmark_run_does_not_override_elapsed_on_error(void) {
    struct benchmark_config config = {
        .implementation = BENCHMARK_IMPL_ARRAY,
        .test_id = 99,
        .test4_push_count = 0U,
        .test3_operations = NULL,
        .test3_operation_count = 0U
    };

    double elapsed_seconds = 123.456;
    benchmark_status_t status = benchmark_run(&config, &elapsed_seconds);
    return status == BENCHMARK_STATUS_INVALID_ARG && elapsed_seconds == 123.456;
}

int main(void) {
    int all_ok =
        test_array_stack_with_int() &&
        test_array_stack_with_struct() &&
        test_array_rollback_on_invalid_push() &&
        test_stack_array_ctor_does_not_override_out_on_error() &&
        test_list_stack_with_int() &&
        test_list_stack_with_struct() &&
        test_stack_list_ctor_does_not_override_out_on_error() &&
        test_benchmark_run_does_not_override_elapsed_on_error();

    if (!all_ok) {
        printf("API tests failed\n");
        return 1;
    }

    printf("API tests passed\n");
    return 0;
}
