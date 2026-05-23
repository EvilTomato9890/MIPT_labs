#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asserts.h"
#include "binary_heap.h"
#include "binomial_heap.h"
#include "logger.h"
#include "return_macros.h"
#include "testing.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define TESTER_ARGC 8
#define DIJKSTRA_DEFAULT_SEED 42U

typedef enum tester_status {
    TESTER_STATUS_OK = TESTING_STATUS_OK,
    TESTER_STATUS_NULL_ARG = TESTING_STATUS_NULL_ARG,
    TESTER_STATUS_INVALID_ARG = TESTING_STATUS_INVALID_ARG,
    TESTER_STATUS_OVERFLOW = TESTING_STATUS_OVERFLOW,
    TESTER_STATUS_ALLOC_FAIL = TESTING_STATUS_ALLOC_FAIL,
    TESTER_STATUS_IO_FAIL = TESTING_STATUS_IO_FAIL,
    TESTER_STATUS_DATA_MISMATCH = TESTING_STATUS_DATA_MISMATCH,
    TESTER_STATUS_HEAP_FAIL = TESTING_STATUS_HEAP_FAIL,
    TESTER_STATUS_UNKNOWN_POINT
} tester_status_t;

typedef testing_status_t (*point_runner_fn)(const dataset_config_t *cfg);

typedef struct named_point_runner {
    const char *name;
    point_runner_fn fn;
} named_point_runner_t;

static tester_status_t parse_size_arg(const char *text, const char *name, size_t *out) {
    SOFT_ASSERT_FUNCTIONAL(text != NULL && name != NULL && out != NULL,
                           "parse_size_arg arguments must not be NULL",
                           return TESTER_STATUS_NULL_ARG);

    char *end = NULL;
    errno = 0;
    unsigned long long parsed = strtoull(text, &end, 10);
    RETURN_IF_ERROR(errno != 0 || end == text || *end != '\0',
                    TESTER_STATUS_INVALID_ARG,
                    "tester: invalid %s '%s'", name, text);
    RETURN_IF_ERROR(parsed > SIZE_MAX,
                    TESTER_STATUS_OVERFLOW,
                    "tester: %s '%s' overflows size_t", name, text);

    *out = (size_t)parsed;
    return TESTER_STATUS_OK;
}

static testing_status_t run_group(const dataset_config_t *cfg,
                                  const named_heap_builder_t *builders,
                                  size_t count) {
    SOFT_ASSERT_FUNCTIONAL(cfg != NULL && builders != NULL,
                           "run_group arguments must not be NULL",
                           return TESTING_STATUS_NULL_ARG);

    for (size_t index = 0U; index < count; ++index) {
        LOGGER_INFO("queue %s", builders[index].name);
    }
    return run_heap_group(cfg, builders, count);
}

static testing_status_t run_dijkstra_point(const dataset_config_t *cfg, graph_kind_t kind) {
    SOFT_ASSERT_FUNCTIONAL(cfg != NULL, "cfg must not be NULL", return TESTING_STATUS_NULL_ARG);

    graph_benchmark_config_t graph_cfg = {
        .result_csv = cfg->result_csv,
        .from = cfg->from,
        .to = cfg->to,
        .step = cfg->step,
        .copies = cfg->copies,
        .seed = DIJKSTRA_DEFAULT_SEED,
        .kind = kind
    };

    named_dijkstra_runner_t runners[] = {
        {"dijkstra_naive", dijkstra_run_naive},
        {"dijkstra_binary_heap", dijkstra_run_binary_heap},
        {"dijkstra_binomial_heap", dijkstra_run_binomial_heap},
        {"dijkstra_fibonacci_heap", dijkstra_run_fibonacci_heap}
    };

    for (size_t index = 0U; index < ARRAY_SIZE(runners); ++index) {
        LOGGER_INFO("queue %s", runners[index].name);
    }
    return run_dijkstra_group(&graph_cfg, runners, ARRAY_SIZE(runners));
}

static testing_status_t run_point_1(const dataset_config_t *cfg) {
    named_heap_builder_t builders[] = {
        {"binary_heap_linear", binary_heap_benchmark_linear},
        {"binary_heap_inserts", binary_heap_benchmark_inserts}
    };
    return run_group(cfg, builders, ARRAY_SIZE(builders));
}

static testing_status_t run_point_2(const dataset_config_t *cfg) {
    named_heap_builder_t builders[] = {
        {"binomial_heap_inserts", binomial_heap_benchmark_inserts}
    };
    return run_group(cfg, builders, ARRAY_SIZE(builders));
}

static testing_status_t run_point_3_sparse(const dataset_config_t *cfg) {
    return run_dijkstra_point(cfg, GRAPH_KIND_SPARSE);
}

static testing_status_t run_point_3_dense(const dataset_config_t *cfg) {
    return run_dijkstra_point(cfg, GRAPH_KIND_DENSE);
}

static point_runner_fn resolve_point_runner(const char *name) {
    SOFT_ASSERT_FUNCTIONAL(name != NULL, "name must not be NULL", return NULL);

    static const named_point_runner_t runners[] = {
        {"p1", run_point_1},
        {"p2", run_point_2},
        {"p3_sparse", run_point_3_sparse},
        {"p3_dense", run_point_3_dense}
    };

    for (size_t index = 0U; index < ARRAY_SIZE(runners); ++index) {
        if (strcmp(name, runners[index].name) == 0) {
            return runners[index].fn;
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    RETURN_IF_ERROR(argc != TESTER_ARGC,
                    TESTER_STATUS_INVALID_ARG,
                    "usage: tester <point> <tests_dir> <csv> <from> <to> <step> <copies>");

    point_runner_fn runner = resolve_point_runner(argv[1]);
    RETURN_IF_ERROR(runner == NULL,
                    TESTER_STATUS_UNKNOWN_POINT,
                    "tester: unknown point '%s'", argv[1]);

    dataset_config_t cfg = {
        .tests_dir = argv[2],
        .result_csv = argv[3],
        .from = 0U,
        .to = 0U,
        .step = 0U,
        .copies = 0U
    };

    tester_status_t status = parse_size_arg(argv[4], "from", &cfg.from);
    if (status != TESTER_STATUS_OK) {
        return (int)status;
    }
    status = parse_size_arg(argv[5], "to", &cfg.to);
    if (status != TESTER_STATUS_OK) {
        return (int)status;
    }
    status = parse_size_arg(argv[6], "step", &cfg.step);
    if (status != TESTER_STATUS_OK) {
        return (int)status;
    }
    status = parse_size_arg(argv[7], "copies", &cfg.copies);
    if (status != TESTER_STATUS_OK) {
        return (int)status;
    }

    return (int)runner(&cfg);
}
