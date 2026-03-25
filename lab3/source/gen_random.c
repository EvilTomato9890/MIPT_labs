#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "asserts.h"
#include "logger.h"

int main(int argc, char **argv) {
    HARD_ASSERT(argc == 3, "usage: gen_random <size> <limit>");
    size_t n = (size_t)strtoull(argv[1], NULL, 10);
    uint32_t limit = (uint32_t)strtoul(argv[2], NULL, 10);
    HARD_ASSERT(limit > 0U, "gen_random: limit must be > 0");

    srand((unsigned)time(NULL));
    LOGGER_INFO("generate n=%zu limit=%u", n, limit);
    printf("%zu\n", n);
    for (size_t i = 0U; i < n; ++i) {
        uint32_t v = (uint32_t)((double)rand() / ((double)RAND_MAX + 1.0) * (double)limit);
        printf("%u%c", v, (i + 1U == n) ? '\n' : ' ');
    }
    return 0;
}
