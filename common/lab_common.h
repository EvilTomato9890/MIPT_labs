#ifndef LAB_COMMON_H_INCLUDED
#define LAB_COMMON_H_INCLUDED

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../asserts.h"

#if defined(_WIN32)
#include <direct.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <time.h>
#endif

static double lab_now_seconds(void) {
#if defined(_WIN32)
    static LARGE_INTEGER frequency = {};
    static int initialized = 0;
    LARGE_INTEGER counter = {};

    if (!initialized) {
        QueryPerformanceFrequency(&frequency);
        initialized = 1;
    }

    QueryPerformanceCounter(&counter);
    return (double) counter.QuadPart / (double) frequency.QuadPart;
#else
    struct timespec ts = {};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double) ts.tv_sec + (double) ts.tv_nsec / 1000000000.0;
#endif
}

static uint64_t lab_splitmix64(uint64_t* state) {
    uint64_t z = 0;

    ASSERT(state != nullptr);

    *state += 0x9e3779b97f4a7c15ULL;
    z       = *state;
    z       = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z       = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

static void lab_fill_sequence(int* data, int n) {
    int i = 0;

    ASSERT(data != nullptr);
    ASSERT(n >= 0);

    for (i = 0; i < n; ++i) {
        data[i] = i;
    }
}

static void lab_shuffle_ints(int* data, int n, uint64_t seed) {
    uint64_t state = seed;
    int i = 0;

    ASSERT(data != nullptr);
    ASSERT(n >= 0);

    if (state == 0) {
        state = 0x123456789abcdefULL;
    }

    for (i = n - 1; i > 0; --i) {
        int j = (int) (lab_splitmix64(&state) % (uint64_t) (i + 1));
        int tmp = data[i];

        data[i] = data[j];
        data[j] = tmp;
    }
}

static int* lab_make_keys(int n, int sorted, uint64_t seed) {
    RETURN_IF(n <= 0, nullptr);

    ASSERT(sorted == 0 || sorted == 1);

    int* keys = (int*) calloc((size_t) n, sizeof(int));

    if (keys == nullptr) {
        fprintf(stderr, "allocation failed for %d keys\n", n);
        return nullptr;
    }

    lab_fill_sequence(keys, n);
    if (!sorted) {
        lab_shuffle_ints(keys, n, seed);
    }

    return keys;
}

static int lab_ensure_dir(const char* path) {
    RETURN_IF(path == nullptr, 0);
    ASSERT(path[0] != '\0');

#if defined(_WIN32)
    if (_mkdir(path) == 0 || errno == EEXIST) {
        return 1;
    }
#else
    if (mkdir(path, 0777) == 0 || errno == EEXIST) {
        return 1;
    }
#endif

    fprintf(stderr, "cannot create directory %s: %s\n", path, strerror(errno));
    return 0;
}

static int lab_is_sorted_unique(const int* data, int n) {
    int i = 0;

    RETURN_IF(data == nullptr, 0);
    RETURN_IF(n < 0, 0);

    for (i = 1; i < n; ++i) {
        if (data[i - 1] >= data[i]) {
            return 0;
        }
    }

    return 1;
}

#endif /* LAB_COMMON_H_INCLUDED */
