#include <stdbool.h>

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "asserts.h"
#include "logger.h"
#include "return_macros.h"

#define RANDOM_LOW_BITS_MASK       0x7FFFU
#define RANDOM_HIGH_BITS_MASK      0x3U
#define RANDOM_MID_BITS_SHIFT      15U
#define RANDOM_HIGH_BITS_SHIFT     30U
#define OUTPUT_BUFFER_EXTRA_CHARS  32U
#define GENERATED_CHARS_PER_NUMBER 11U
#define GEN_RANDOM_ARGC_NO_SEED    3
#define GEN_RANDOM_ARGC_WITH_SEED  4

typedef enum gen_random_status {
    GEN_RANDOM_STATUS_OK = 0,
    GEN_RANDOM_STATUS_NULL_ARG,
    GEN_RANDOM_STATUS_INVALID_ARG,
    GEN_RANDOM_STATUS_OVERFLOW,
    GEN_RANDOM_STATUS_ALLOC_FAIL,
    GEN_RANDOM_STATUS_IO_FAIL
} gen_random_status_t;

static gen_random_status_t parse_size_arg(const char *text, const char *name, size_t *out) {
    SOFT_ASSERT_FUNCTIONAL(text != NULL && name != NULL && out != NULL,
                           "parse_size_arg arguments must not be NULL",
                           return GEN_RANDOM_STATUS_NULL_ARG);

    char *end = NULL;
    errno = 0;
    unsigned long long parsed = strtoull(text, &end, 10);
    RETURN_IF_ERROR(errno != 0 || end == text || *end != '\0',
                    GEN_RANDOM_STATUS_INVALID_ARG,
                    "gen_random: invalid %s '%s'", name, text);
    RETURN_IF_ERROR(parsed > SIZE_MAX,
                    GEN_RANDOM_STATUS_OVERFLOW,
                    "gen_random: %s '%s' overflows size_t", name, text);

    *out = (size_t)parsed;
    return GEN_RANDOM_STATUS_OK;
}

static gen_random_status_t parse_limit_arg(const char *text, int *out) {
    SOFT_ASSERT_FUNCTIONAL(text != NULL && out != NULL,
                           "parse_limit_arg arguments must not be NULL",
                           return GEN_RANDOM_STATUS_NULL_ARG);

    char *end = NULL;
    errno = 0;
    unsigned long long parsed = strtoull(text, &end, 10);
    RETURN_IF_ERROR(errno != 0 || end == text || *end != '\0',
                    GEN_RANDOM_STATUS_INVALID_ARG,
                    "gen_random: invalid limit '%s'", text);
    RETURN_IF_ERROR(parsed == 0ULL,
                    GEN_RANDOM_STATUS_INVALID_ARG,
                    "gen_random: limit must be greater than zero");
    RETURN_IF_ERROR(parsed > (unsigned long long)INT_MAX,
                    GEN_RANDOM_STATUS_OVERFLOW,
                    "gen_random: limit must be in [1, %d]", INT_MAX);

    *out = (int)parsed;
    return GEN_RANDOM_STATUS_OK;
}

static gen_random_status_t parse_seed_arg(const char *text, unsigned *out) {
    SOFT_ASSERT_FUNCTIONAL(text != NULL && out != NULL,
                           "parse_seed_arg arguments must not be NULL",
                           return GEN_RANDOM_STATUS_NULL_ARG);

    char *end = NULL;
    errno = 0;
    unsigned long long parsed = strtoull(text, &end, 10);
    RETURN_IF_ERROR(errno != 0 || end == text || *end != '\0',
                    GEN_RANDOM_STATUS_INVALID_ARG,
                    "gen_random: invalid seed '%s'", text);
    RETURN_IF_ERROR(parsed > (unsigned long long)UINT_MAX,
                    GEN_RANDOM_STATUS_OVERFLOW,
                    "gen_random: seed '%s' overflows unsigned", text);

    *out = (unsigned)parsed;
    return GEN_RANDOM_STATUS_OK;
}

static uint32_t random_u32(void) {
    uint32_t low = (uint32_t)(rand() & RANDOM_LOW_BITS_MASK);
    uint32_t mid = (uint32_t)(rand() & RANDOM_LOW_BITS_MASK);
    uint32_t high = (uint32_t)(rand() & RANDOM_HIGH_BITS_MASK);
    return low | (mid << RANDOM_MID_BITS_SHIFT) | (high << RANDOM_HIGH_BITS_SHIFT);
}

static uint32_t random_bounded(uint32_t limit) {
    uint32_t bound = limit + 1U;
    if (bound == 0U) {
        return random_u32();
    }

    uint32_t threshold = (uint32_t)(-bound % bound);
    while (true) {
        uint32_t value = random_u32();
        if (value >= threshold) {
            return value % bound;
        }
    }
}

static gen_random_status_t write_generated_array(size_t n, int limit) {
    RETURN_IF_ERROR(n > (SIZE_MAX - OUTPUT_BUFFER_EXTRA_CHARS) / GENERATED_CHARS_PER_NUMBER,
                    GEN_RANDOM_STATUS_OVERFLOW,
                    "gen_random: requested array size %zu is too large", n);

    size_t buffer_size = OUTPUT_BUFFER_EXTRA_CHARS + n * GENERATED_CHARS_PER_NUMBER;
    char *buffer = calloc(buffer_size, sizeof(buffer[0]));
    RETURN_IF_ERROR(buffer == NULL,
                    GEN_RANDOM_STATUS_ALLOC_FAIL,
                    "gen_random: no memory for output buffer");

    int written = snprintf(buffer, buffer_size, "%zu\n", n);
    RETURN_IF_ERROR_CLEANUP(written <= 0 || (size_t)written >= buffer_size,
                            GEN_RANDOM_STATUS_IO_FAIL,
                            free(buffer),
                            "gen_random: failed to format size");
    size_t position = (size_t)written;

    for (size_t index = 0U; index < n; ++index) {
        int value = (int)random_bounded((uint32_t)limit);
        written = snprintf(buffer + position, buffer_size - position, "%d%c",
                           value, (index + 1U == n) ? '\n' : ' ');
        RETURN_IF_ERROR_CLEANUP(written <= 0 || (size_t)written >= (buffer_size - position),
                                GEN_RANDOM_STATUS_IO_FAIL,
                                free(buffer),
                                "gen_random: output buffer overflow while writing item %zu", index);
        position += (size_t)written;
    }

    RETURN_IF_ERROR_CLEANUP(fwrite(buffer, sizeof(buffer[0]), position, stdout) != position,
                            GEN_RANDOM_STATUS_IO_FAIL,
                            free(buffer),
                            "gen_random: fwrite failed");
    free(buffer);
    return GEN_RANDOM_STATUS_OK;
}

int main(int argc, char **argv) {
    RETURN_IF_ERROR(argc != GEN_RANDOM_ARGC_NO_SEED && argc != GEN_RANDOM_ARGC_WITH_SEED,
                    GEN_RANDOM_STATUS_INVALID_ARG,
                    "usage: gen_random <size> <limit> [seed]");

    size_t n = 0U;
    int limit = 0;
    gen_random_status_t status = parse_size_arg(argv[1], "size", &n);
    RETURN_IF_ERROR(status != GEN_RANDOM_STATUS_OK, status,
                    "gen_random: failed to parse size '%s'", argv[1]);
    status = parse_limit_arg(argv[2], &limit);
    RETURN_IF_ERROR(status != GEN_RANDOM_STATUS_OK, status,
                    "gen_random: failed to parse limit '%s'", argv[2]);

    unsigned seed = (unsigned)time(NULL);
    if (argc == GEN_RANDOM_ARGC_WITH_SEED) {
        status = parse_seed_arg(argv[3], &seed);
        RETURN_IF_ERROR(status != GEN_RANDOM_STATUS_OK, status,
                        "gen_random: failed to parse seed '%s'", argv[3]);
    }

    srand(seed);
    LOGGER_INFO("generate n=%zu limit=%d seed=%u", n, limit, seed);
    return (int)write_generated_array(n, limit);
}
