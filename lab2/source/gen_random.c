#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "asserts.h"
#include "logger.h"

#define RANDOM_LOW_BITS_MASK       0x7FFFU
#define RANDOM_HIGH_BITS_MASK      0x3U
#define RANDOM_MID_BITS_SHIFT      15U
#define RANDOM_HIGH_BITS_SHIFT     30U
#define OUTPUT_BUFFER_EXTRA_CHARS  32U
#define GENERATED_CHARS_PER_NUMBER 11U
#define GEN_RANDOM_ARGC_NO_SEED    3
#define GEN_RANDOM_ARGC_WITH_SEED  4

static uint32_t random_u32(void) {
    uint32_t low  = (uint32_t)(rand() & RANDOM_LOW_BITS_MASK);
    uint32_t mid  = (uint32_t)(rand() & RANDOM_LOW_BITS_MASK);
    uint32_t high = (uint32_t)(rand() & RANDOM_HIGH_BITS_MASK);
    return low | (mid << RANDOM_MID_BITS_SHIFT) | (high << RANDOM_HIGH_BITS_SHIFT);
}

static uint32_t random_bounded(uint32_t limit) {
    uint32_t bound = limit + 1U;
    if (bound == 0U) return random_u32();

    uint32_t threshold = (uint32_t)(-bound % bound);
    while (1) {
        uint32_t value = random_u32();
        if (value >= threshold) return value % bound;
    }
}

static void write_generated_array(size_t n, uint32_t limit) {
    size_t buffer_size = OUTPUT_BUFFER_EXTRA_CHARS + n * GENERATED_CHARS_PER_NUMBER;
    char  *buffer      = (char *)calloc(buffer_size, sizeof(char));
    HARD_ASSERT(buffer != NULL, "gen_random: no memory for output buffer");

    int written = snprintf(buffer, buffer_size, "%zu\n", n);
    HARD_ASSERT(written > 0, "gen_random: failed to format size");
    size_t pos = (size_t)written;

    for (size_t i = 0U; i < n; ++i) {
        uint32_t value = random_bounded(limit);
        written = snprintf(buffer + pos, buffer_size - pos, "%u%c",
                           value, (i + 1U == n) ? '\n' : ' ');
        HARD_ASSERT(written > 0 && (size_t)written < (buffer_size - pos),
                    "gen_random: output buffer overflow");
        pos += (size_t)written;
    }

    HARD_ASSERT(fwrite(buffer, sizeof(char), pos, stdout) == pos,
                "gen_random: fwrite failed");
    free(buffer);
}

int main(int argc, char **argv) {
    HARD_ASSERT(argc == GEN_RANDOM_ARGC_NO_SEED || argc == GEN_RANDOM_ARGC_WITH_SEED,
                "usage: gen_random <size> <limit> [seed]");
    size_t n       =   (size_t)strtoull(argv[1], NULL, 10);
    uint32_t limit = (uint32_t)strtoul (argv[2], NULL, 10);
    HARD_ASSERT(limit > 0U, "gen_random: limit must be > 0");

    unsigned seed = (argc == GEN_RANDOM_ARGC_WITH_SEED) ? (unsigned)strtoul(argv[3], NULL, 10)
                                : (unsigned)time(NULL);
    srand(seed);
    LOGGER_INFO("generate n=%zu limit=%u seed=%u", n, limit, seed);
    write_generated_array(n, limit);
    return 0;
}
