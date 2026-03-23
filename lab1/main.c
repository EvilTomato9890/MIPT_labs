#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asserts.h"
#include "benchmark.h"
#include "logger.h"

typedef enum cli_status {
    CLI_STATUS_OK = 0,
    CLI_STATUS_NULL_ARG,
    CLI_STATUS_INVALID_ARG,
    CLI_STATUS_OVERFLOW,
    CLI_STATUS_ALLOC_FAIL,
    CLI_STATUS_IO_FAIL
} cli_status_t;

#define CLI_RETURN(status_value)                                                          \
    do {                                                                                  \
        cli_status_t status_to_return = (status_value);                                   \
        if (status_to_return != CLI_STATUS_OK) {                                          \
            LOGGER_ERROR("cli return status=%d", (int)status_to_return);                \
        }                                                                                 \
        return status_to_return;                                                          \
    } while (0)

struct cli_options {
    const char *implementation_text;
    int test_id;
    size_t test4_push_count;
    const char *operations_file_path;
};

static void print_usage(const char *program_name) {
    fprintf(stderr,
            "Usage: %s --impl <array|list> --test <1|2|3|4> [--n <value>] [--ops-file <path>]\n",
            program_name);
}

static cli_status_t parse_size_t_value(const char *text, size_t *value) {
    SOFT_ASSERT_FUNCTIONAL(text != NULL && value != NULL, "Parse_size arguments must not be NULL",
                           CLI_RETURN(CLI_STATUS_NULL_ARG));

    errno = 0;
    char *end_pointer = NULL;
    unsigned long long parsed = strtoull(text, &end_pointer, 10);
    if (errno != 0 || end_pointer == text || *end_pointer != '\0') {
        CLI_RETURN(CLI_STATUS_INVALID_ARG);
    }
    if (parsed > (unsigned long long)SIZE_MAX) {
        CLI_RETURN(CLI_STATUS_OVERFLOW);
    }

    *value = (size_t)parsed;
    CLI_RETURN(CLI_STATUS_OK);
}

static cli_status_t parse_int_value(const char *text, int *value) {
    SOFT_ASSERT_FUNCTIONAL(text != NULL && value != NULL, "Parse_int arguments must not be NULL",
                           CLI_RETURN(CLI_STATUS_NULL_ARG));

    errno = 0;
    char *end_pointer = NULL;
    long parsed = strtol(text, &end_pointer, 10);
    if (errno != 0 || end_pointer == text || *end_pointer != '\0') {
        CLI_RETURN(CLI_STATUS_INVALID_ARG);
    }
    if (parsed < INT_MIN || parsed > INT_MAX) {
        CLI_RETURN(CLI_STATUS_OVERFLOW);
    }

    *value = (int)parsed;
    CLI_RETURN(CLI_STATUS_OK);
}

static cli_status_t parse_arguments(int argc, char **argv, struct cli_options *options) {
    SOFT_ASSERT_FUNCTIONAL(argv != NULL && options != NULL, "Parse_arguments pointers must not be NULL",
                           CLI_RETURN(CLI_STATUS_NULL_ARG));

    struct cli_options parsed_options = {
        .implementation_text = NULL,
        .test_id = 0,
        .test4_push_count = 0U,
        .operations_file_path = NULL
    };

    for (int index = 1; index < argc; ++index) {
        if (strcmp(argv[index], "--impl") == 0 && index + 1 < argc) {
            parsed_options.implementation_text = argv[++index];
        } else if (strcmp(argv[index], "--test") == 0 && index + 1 < argc) {
            cli_status_t parse_status = parse_int_value(argv[++index], &parsed_options.test_id);
            if (parse_status != CLI_STATUS_OK) {
                CLI_RETURN(parse_status);
            }
        } else if (strcmp(argv[index], "--n") == 0 && index + 1 < argc) {
            cli_status_t parse_status = parse_size_t_value(argv[++index], &parsed_options.test4_push_count);
            if (parse_status != CLI_STATUS_OK) {
                CLI_RETURN(parse_status);
            }
        } else if (strcmp(argv[index], "--ops-file") == 0 && index + 1 < argc) {
            parsed_options.operations_file_path = argv[++index];
        } else {
            CLI_RETURN(CLI_STATUS_INVALID_ARG);
        }
    }

    if (parsed_options.implementation_text == NULL || parsed_options.test_id < 1 || parsed_options.test_id > 4) {
        CLI_RETURN(CLI_STATUS_INVALID_ARG);
    }

    *options = parsed_options;
    CLI_RETURN(CLI_STATUS_OK);
}

static cli_status_t append_operation(uint8_t **operations, size_t *count, size_t *capacity, uint8_t value) {
    SOFT_ASSERT_FUNCTIONAL(operations != NULL && count != NULL && capacity != NULL,
                           "Append_operation arguments must not be NULL", CLI_RETURN(CLI_STATUS_NULL_ARG));

    if (*count == *capacity) {
        size_t new_capacity = (*capacity == 0U) ? 4096U : (*capacity * 2U);
        if (new_capacity < *capacity) {
            CLI_RETURN(CLI_STATUS_OVERFLOW);
        }

        uint8_t *new_buffer = (uint8_t *)realloc(*operations, new_capacity * sizeof(uint8_t));
        if (new_buffer == NULL) {
            CLI_RETURN(CLI_STATUS_ALLOC_FAIL);
        }

        *operations = new_buffer;
        *capacity = new_capacity;
    }

    (*operations)[*count] = value;
    *count += 1U;
    CLI_RETURN(CLI_STATUS_OK);
}

static cli_status_t load_operations_file(const char *path, uint8_t **operations, size_t *count) {
    SOFT_ASSERT_FUNCTIONAL(path != NULL && operations != NULL && count != NULL,
                           "Load_operations arguments must not be NULL", CLI_RETURN(CLI_STATUS_NULL_ARG));

    FILE *input = fopen(path, "r");
    if (input == NULL) {
        CLI_RETURN(CLI_STATUS_IO_FAIL);
    }

    uint8_t *buffer = NULL;
    size_t buffer_size = 0U;
    size_t buffer_capacity = 0U;
    cli_status_t status = CLI_STATUS_OK;

    for (;;) {
        int symbol = fgetc(input);
        if (symbol == EOF) {
            break;
        }
        if (isspace((unsigned char)symbol)) {
            continue;
        }

        if (symbol == '1') {
            status = append_operation(&buffer, &buffer_size, &buffer_capacity, 1U);
        } else if (symbol == '2') {
            status = append_operation(&buffer, &buffer_size, &buffer_capacity, 2U);
        } else {
            status = CLI_STATUS_INVALID_ARG;
        }

        if (status != CLI_STATUS_OK) {
            break;
        }
    }

    fclose(input);
    if (status != CLI_STATUS_OK || buffer_size == 0U) {
        free(buffer);
        CLI_RETURN((status == CLI_STATUS_OK) ? CLI_STATUS_INVALID_ARG : status);
    }

    *operations = buffer;
    *count = buffer_size;
    CLI_RETURN(CLI_STATUS_OK);
}

int main(int argc, char **argv) {
    logger_initialize_stream(NULL);

    struct cli_options options = {0};
    cli_status_t parse_status = parse_arguments(argc, argv, &options);
    if (parse_status != CLI_STATUS_OK) {
        print_usage(argv[0]);
        return 1;
    }

    struct benchmark_config config = {
        .implementation = BENCHMARK_IMPL_ARRAY,
        .test_id = options.test_id,
        .test4_push_count = options.test4_push_count,
        .test3_operations = NULL,
        .test3_operation_count = 0U
    };

    benchmark_status_t impl_status = benchmark_parse_impl(options.implementation_text, &config.implementation);
    if (impl_status != BENCHMARK_STATUS_OK) {
        LOGGER_ERROR("Implementation parse failed");
        print_usage(argv[0]);
        return 1;
    }

    uint8_t *operations = NULL;
    size_t operations_count = 0U;

    if (options.test_id == 3) {
        if (options.operations_file_path == NULL) {
            LOGGER_ERROR("Missing --ops-file for test 3");
            print_usage(argv[0]);
            return 1;
        }

        cli_status_t load_status = load_operations_file(options.operations_file_path, &operations, &operations_count);
        if (load_status != CLI_STATUS_OK) {
            LOGGER_ERROR("Failed to load operations from %s", options.operations_file_path);
            return 1;
        }

        config.test3_operations = operations;
        config.test3_operation_count = operations_count;
    }

    if (options.test_id == 4 && options.test4_push_count == 0U) {
        LOGGER_ERROR("Invalid --n for test 4");
        print_usage(argv[0]);
        free(operations);
        return 1;
    }

    double elapsed_seconds = 0.0;
    benchmark_status_t benchmark_status = benchmark_run(&config, &elapsed_seconds);
    if (benchmark_status != BENCHMARK_STATUS_OK) {
        LOGGER_ERROR("Benchmark failed with status=%d", (int)benchmark_status);
        free(operations);
        return 1;
    }

    printf("%.9f\n", elapsed_seconds);
    free(operations);
    logger_close();
    return 0;
}
