/**
 * @file benchmark.c
 * @brief Implementation of stack benchmark scenarios.
 */

#include "benchmark.h"

#include <string.h>
#include <time.h>

#include "asserts.h"
#include "logger.h"
#include "stack_array.h"
#include "stack_list.h"

enum {
    /** Initial capacity hint used for array-backed stacks in benchmarks. */
    BENCHMARK_INITIAL_CAPACITY = 1024,
    /** Initial stack size for tests 1-3. */
    TEST_BASE_SIZE             = 1000000,
    /** Stop threshold for the first test scenario. */
    TEST_STOP_SIZE             = 100000,
    /** Divisor used to calculate how many elements are removed in test 1. */
    TEST1_SIZE_DECREASE_FACTOR = 2,
    /** Divisor used to calculate how many elements are added back in test 1. */
    TEST1_SIZE_INCREASE_FACTOR = 4,
    /** Number of pop/push stress blocks in test 2. */
    TEST2_BLOCK_ITERATIONS     = 100,
    /** Number of operations inside one stress block in test 2. */
    TEST2_BLOCK_SIZE           = 10000,
    /** Required number of generated operations for test 3. */
    TEST3_OPERATIONS_REQUIRED  = 1000000
};

#define BM_RETURN(status_value)                                                          \
    do {                                                                                 \
        benchmark_status_t status_to_return = (status_value);                            \
        if (status_to_return != BENCHMARK_STATUS_OK) {                                   \
            LOGGER_ERROR("benchmark return status=%d", (int)status_to_return);         \
        }                                                                                \
        return status_to_return;                                                         \
    } while (0)

/**
 * @brief Type-erased stack wrapper used by benchmark scenarios.
 */
struct stack_context {
    /** Selected stack implementation. */
    enum benchmark_impl_type implementation;
    /** Pointer to stack_array or stack_list. */
    void *stack_object;
};

static double benchmark_now_seconds(void) {
    struct timespec timestamp = {0};
    (void)timespec_get(&timestamp, TIME_UTC);
    return (double)timestamp.tv_sec + (double)timestamp.tv_nsec / 1000000000.0;
}

static benchmark_status_t stack_context_init(struct stack_context *context, enum benchmark_impl_type implementation,
                                             size_t initial_size, size_t element_size) {
    SOFT_ASSERT_FUNCTIONAL(context != NULL, "Stack context pointer must not be NULL",
                           BM_RETURN(BENCHMARK_STATUS_NULL_ARG));

    struct stack_context created = {
        .implementation = implementation,
        .stack_object = NULL
    };

    if (implementation == BENCHMARK_IMPL_ARRAY) {
        stack_array *created_stack = NULL;
        stack_array_status_t status = stack_array_ctr(initial_size, element_size, &created_stack);
        if (status != STACK_ARRAY_STATUS_OK) {
            BM_RETURN((status == STACK_ARRAY_STATUS_ALLOC_FAIL)
                ? BENCHMARK_STATUS_ALLOC_FAIL
                : BENCHMARK_STATUS_INTERNAL_ERROR);
        }
        created.stack_object = created_stack;
    } else if (implementation == BENCHMARK_IMPL_LIST) {
        stack_list *created_stack = NULL;
        stack_list_status_t status = stack_list_ctor(initial_size, element_size, &created_stack);
        if (status != STACK_LIST_STATUS_OK) {
            BM_RETURN((status == STACK_LIST_STATUS_ALLOC_FAIL)
                ? BENCHMARK_STATUS_ALLOC_FAIL
                : BENCHMARK_STATUS_INTERNAL_ERROR);
        }
        created.stack_object = created_stack;
    } else {
        BM_RETURN(BENCHMARK_STATUS_INVALID_ARG);
    }

    *context = created;
    BM_RETURN(BENCHMARK_STATUS_OK);
}

static benchmark_status_t stack_context_destroy(struct stack_context *context) {
    SOFT_ASSERT_FUNCTIONAL(context != NULL, "Stack context pointer must not be NULL",
                           BM_RETURN(BENCHMARK_STATUS_NULL_ARG));
    if (context->stack_object == NULL) {
        BM_RETURN(BENCHMARK_STATUS_OK);
    }

    benchmark_status_t status = BENCHMARK_STATUS_OK;
    if (context->implementation == BENCHMARK_IMPL_ARRAY) {
        stack_array *stack = (stack_array *)context->stack_object;
        stack_array_status_t destroy_status = stack_array_dtr(&stack);
        if (destroy_status != STACK_ARRAY_STATUS_OK) {
            status = BENCHMARK_STATUS_INTERNAL_ERROR;
        }
    } else if (context->implementation == BENCHMARK_IMPL_LIST) {
        stack_list *stack = (stack_list *)context->stack_object;
        stack_list_status_t destroy_status = stack_list_dtor(&stack);
        if (destroy_status != STACK_LIST_STATUS_OK) {
            status = BENCHMARK_STATUS_INTERNAL_ERROR;
        }
    } else {
        status = BENCHMARK_STATUS_INVALID_ARG;
    }

    context->stack_object = NULL;
    BM_RETURN(status);
}

static benchmark_status_t stack_context_push(struct stack_context *context, int value) {
    SOFT_ASSERT_FUNCTIONAL(context != NULL && context->stack_object != NULL, "Push context must be initialized",
                           BM_RETURN(BENCHMARK_STATUS_BAD_STATE));

    if (context->implementation == BENCHMARK_IMPL_ARRAY) {
        stack_array_status_t status = stack_array_push((stack_array *)context->stack_object, &value);
        BM_RETURN((status == STACK_ARRAY_STATUS_OK) ? BENCHMARK_STATUS_OK : BENCHMARK_STATUS_INTERNAL_ERROR);
    }
    if (context->implementation == BENCHMARK_IMPL_LIST) {
        stack_list_status_t status = stack_list_push((stack_list *)context->stack_object, &value);
        BM_RETURN((status == STACK_LIST_STATUS_OK) ? BENCHMARK_STATUS_OK : BENCHMARK_STATUS_INTERNAL_ERROR);
    }

    BM_RETURN(BENCHMARK_STATUS_INVALID_ARG);
}

static benchmark_status_t stack_context_pop_strict(struct stack_context *context) {
    SOFT_ASSERT_FUNCTIONAL(context != NULL && context->stack_object != NULL, "Pop context must be initialized",
                           BM_RETURN(BENCHMARK_STATUS_BAD_STATE));

    if (context->implementation == BENCHMARK_IMPL_ARRAY) {
        stack_array_status_t status = stack_array_pop((stack_array *)context->stack_object);
        BM_RETURN((status == STACK_ARRAY_STATUS_OK) ? BENCHMARK_STATUS_OK : BENCHMARK_STATUS_INTERNAL_ERROR);
    }
    if (context->implementation == BENCHMARK_IMPL_LIST) {
        stack_list_status_t status = stack_list_pop((stack_list *)context->stack_object);
        BM_RETURN((status == STACK_LIST_STATUS_OK) ? BENCHMARK_STATUS_OK : BENCHMARK_STATUS_INTERNAL_ERROR);
    }

    BM_RETURN(BENCHMARK_STATUS_INVALID_ARG);
}

static benchmark_status_t push_many(struct stack_context *context, size_t count, int *next_value) {
    SOFT_ASSERT_FUNCTIONAL(context != NULL && next_value != NULL, "Push_many arguments must not be NULL",
                           BM_RETURN(BENCHMARK_STATUS_NULL_ARG));

    for (size_t index = 0U; index < count; ++index) {
        benchmark_status_t status = stack_context_push(context, *next_value);
        if (status != BENCHMARK_STATUS_OK) {
            BM_RETURN(status);
        }
        *next_value += 1;
    }

    BM_RETURN(BENCHMARK_STATUS_OK);
}

static benchmark_status_t pop_many_strict(struct stack_context *context, size_t count) {
    SOFT_ASSERT_FUNCTIONAL(context != NULL, "Context must not be NULL",
                           BM_RETURN(BENCHMARK_STATUS_NULL_ARG));

    for (size_t index = 0U; index < count; ++index) {
        benchmark_status_t status = stack_context_pop_strict(context);
        if (status != BENCHMARK_STATUS_OK) {
            BM_RETURN(status);
        }
    }

    BM_RETURN(BENCHMARK_STATUS_OK);
}

static benchmark_status_t run_test1_half_pop_quarter_push(struct stack_context *context,
                                                           size_t *current_size, int *next_value) {
    SOFT_ASSERT_FUNCTIONAL(context != NULL && current_size != NULL && next_value != NULL,
                           "Test1 arguments must not be NULL", BM_RETURN(BENCHMARK_STATUS_NULL_ARG));

    while (*current_size >= TEST_STOP_SIZE) {
        size_t previous_size = *current_size;
        size_t pop_count = previous_size / TEST1_SIZE_DECREASE_FACTOR;
        size_t push_count = previous_size / TEST1_SIZE_INCREASE_FACTOR;

        benchmark_status_t pop_status = pop_many_strict(context, pop_count);
        if (pop_status != BENCHMARK_STATUS_OK) {
            BM_RETURN(pop_status);
        }
        *current_size -= pop_count;

        benchmark_status_t push_status = push_many(context, push_count, next_value);
        if (push_status != BENCHMARK_STATUS_OK) {
            BM_RETURN(push_status);
        }
        *current_size += push_count;
    }

    BM_RETURN(BENCHMARK_STATUS_OK);
}

static benchmark_status_t run_test2_stress_block(struct stack_context *context, size_t *current_size, int *next_value) {
    SOFT_ASSERT_FUNCTIONAL(context != NULL && current_size != NULL && next_value != NULL,
                           "Test2 arguments must not be NULL", BM_RETURN(BENCHMARK_STATUS_NULL_ARG));

    for (size_t iteration = 0U; iteration < TEST2_BLOCK_ITERATIONS; ++iteration) {
        if (*current_size < TEST2_BLOCK_SIZE) {
            BM_RETURN(BENCHMARK_STATUS_INVALID_ARG);
        }

        benchmark_status_t pop_status = pop_many_strict(context, TEST2_BLOCK_SIZE);
        if (pop_status != BENCHMARK_STATUS_OK) {
            BM_RETURN(pop_status);
        }
        *current_size -= TEST2_BLOCK_SIZE;

        benchmark_status_t push_status = push_many(context, TEST2_BLOCK_SIZE, next_value);
        if (push_status != BENCHMARK_STATUS_OK) {
            BM_RETURN(push_status);
        }
        *current_size += TEST2_BLOCK_SIZE;
    }

    BM_RETURN(BENCHMARK_STATUS_OK);
}

static benchmark_status_t run_test_1(struct stack_context *context, double *elapsed_seconds) {
    SOFT_ASSERT_FUNCTIONAL(context != NULL && elapsed_seconds != NULL, "Run_test_1 arguments must not be NULL",
                           BM_RETURN(BENCHMARK_STATUS_NULL_ARG));

    int next_value = 0;
    size_t current_size = TEST_BASE_SIZE;

    double start_time = benchmark_now_seconds();
    benchmark_status_t fill_status = push_many(context, TEST_BASE_SIZE, &next_value);
    if (fill_status != BENCHMARK_STATUS_OK) {
        BM_RETURN(fill_status);
    }

    benchmark_status_t test_status = run_test1_half_pop_quarter_push(context, &current_size, &next_value);
    if (test_status != BENCHMARK_STATUS_OK) {
        BM_RETURN(test_status);
    }

    *elapsed_seconds = benchmark_now_seconds() - start_time;
    BM_RETURN(BENCHMARK_STATUS_OK);
}

static benchmark_status_t run_test_2(struct stack_context *context, double *elapsed_seconds) {
    SOFT_ASSERT_FUNCTIONAL(context != NULL && elapsed_seconds != NULL, "Run_test_2 arguments must not be NULL",
                           BM_RETURN(BENCHMARK_STATUS_NULL_ARG));

    int next_value = 0;
    size_t current_size = TEST_BASE_SIZE;

    double start_time = benchmark_now_seconds();
    benchmark_status_t fill_status = push_many(context, TEST_BASE_SIZE, &next_value);
    if (fill_status != BENCHMARK_STATUS_OK) {
        BM_RETURN(fill_status);
    }

    benchmark_status_t pre_status = run_test2_stress_block(context, &current_size, &next_value);
    if (pre_status != BENCHMARK_STATUS_OK) {
        BM_RETURN(pre_status);
    }

    benchmark_status_t mid_status = run_test1_half_pop_quarter_push(context, &current_size, &next_value);
    if (mid_status != BENCHMARK_STATUS_OK) {
        BM_RETURN(mid_status);
    }

    benchmark_status_t post_status = run_test2_stress_block(context, &current_size, &next_value);
    if (post_status != BENCHMARK_STATUS_OK) {
        BM_RETURN(post_status);
    }

    *elapsed_seconds = benchmark_now_seconds() - start_time;
    BM_RETURN(BENCHMARK_STATUS_OK);
}

static benchmark_status_t run_test_3(struct stack_context *context,
                                     const struct benchmark_config *config, double *elapsed_seconds) {
    SOFT_ASSERT_FUNCTIONAL(context != NULL && config != NULL && elapsed_seconds != NULL,
                           "Run_test_3 arguments must not be NULL", BM_RETURN(BENCHMARK_STATUS_NULL_ARG));
    if (config->test3_operations == NULL || config->test3_operation_count != TEST3_OPERATIONS_REQUIRED) {
        BM_RETURN(BENCHMARK_STATUS_INVALID_ARG);
    }

    int next_value = 0;
    benchmark_status_t fill_status = push_many(context, TEST_BASE_SIZE, &next_value);
    if (fill_status != BENCHMARK_STATUS_OK) {
        BM_RETURN(fill_status);
    }

    double start_time = benchmark_now_seconds();
    for (size_t index = 0U; index < config->test3_operation_count; ++index) {
        uint8_t operation = config->test3_operations[index];
        if (operation == 1U) {
            benchmark_status_t push_status = stack_context_push(context, next_value);
            if (push_status != BENCHMARK_STATUS_OK) {
                BM_RETURN(push_status);
            }
            next_value += 1;
        } else if (operation == 2U) {
            benchmark_status_t pop_status = stack_context_pop_strict(context);
            if (pop_status != BENCHMARK_STATUS_OK) {
                BM_RETURN(pop_status);
            }
        } else {
            BM_RETURN(BENCHMARK_STATUS_INVALID_ARG);
        }
    }

    *elapsed_seconds = benchmark_now_seconds() - start_time;
    BM_RETURN(BENCHMARK_STATUS_OK);
}

static benchmark_status_t run_test_4(struct stack_context *context, size_t push_count, double *elapsed_seconds) {
    SOFT_ASSERT_FUNCTIONAL(context != NULL && elapsed_seconds != NULL, "Run_test_4 arguments must not be NULL",
                           BM_RETURN(BENCHMARK_STATUS_NULL_ARG));
    if (push_count == 0U) {
        BM_RETURN(BENCHMARK_STATUS_INVALID_ARG);
    }

    int next_value = 0;
    double start_time = benchmark_now_seconds();
    benchmark_status_t fill_status = push_many(context, push_count, &next_value);
    if (fill_status != BENCHMARK_STATUS_OK) {
        BM_RETURN(fill_status);
    }

    *elapsed_seconds = benchmark_now_seconds() - start_time;
    BM_RETURN(BENCHMARK_STATUS_OK);
}

benchmark_status_t benchmark_parse_impl(const char *text, enum benchmark_impl_type *implementation) {
    SOFT_ASSERT_FUNCTIONAL(text != NULL && implementation != NULL, "Parse_impl arguments must not be NULL",
                           BM_RETURN(BENCHMARK_STATUS_NULL_ARG));

    if (strcmp(text, "array") == 0) {
        *implementation = BENCHMARK_IMPL_ARRAY;
        BM_RETURN(BENCHMARK_STATUS_OK);
    }
    if (strcmp(text, "list") == 0) {
        *implementation = BENCHMARK_IMPL_LIST;
        BM_RETURN(BENCHMARK_STATUS_OK);
    }

    BM_RETURN(BENCHMARK_STATUS_INVALID_ARG);
}

benchmark_status_t benchmark_run(const struct benchmark_config *config, double *elapsed_seconds) {
    SOFT_ASSERT_FUNCTIONAL(config != NULL && elapsed_seconds != NULL, "Benchmark_run arguments must not be NULL",
                           BM_RETURN(BENCHMARK_STATUS_NULL_ARG));

    struct stack_context context = {0};
    benchmark_status_t init_status = stack_context_init(&context, config->implementation,
                                                        BENCHMARK_INITIAL_CAPACITY, sizeof(int));
    if (init_status != BENCHMARK_STATUS_OK) {
        BM_RETURN(init_status);
    }

    double measured_seconds = 0.0;
    benchmark_status_t run_status = BENCHMARK_STATUS_INVALID_ARG;

    if (config->test_id == 1) {
        run_status = run_test_1(&context, &measured_seconds);
    } else if (config->test_id == 2) {
        run_status = run_test_2(&context, &measured_seconds);
    } else if (config->test_id == 3) {
        run_status = run_test_3(&context, config, &measured_seconds);
    } else if (config->test_id == 4) {
        run_status = run_test_4(&context, config->test4_push_count, &measured_seconds);
    }

    benchmark_status_t destroy_status = stack_context_destroy(&context);
    if (run_status != BENCHMARK_STATUS_OK) {
        BM_RETURN(run_status);
    }
    if (destroy_status != BENCHMARK_STATUS_OK) {
        BM_RETURN(destroy_status);
    }

    *elapsed_seconds = measured_seconds;
    BM_RETURN(BENCHMARK_STATUS_OK);
}
