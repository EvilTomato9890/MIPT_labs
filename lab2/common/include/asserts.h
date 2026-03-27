#ifndef LAB2_COMMON_INCLUDE_ASSERTS_H_NCLUDED
#define LAB2_COMMON_INCLUDE_ASSERTS_H_NCLUDED

#include <stdio.h>
#include <stdlib.h>

#include "colors.h"

#define stringify(a) #a

#ifdef NDEBUG
    #define HARD_ASSERT(...) (void)0
#else
    #define HARD_ASSERT(test, message)                                                   \
        do {                                                                             \
            if (!(test)) {                                                               \
                fprintf(stderr,                                                          \
                        RED "%s\nERROR OCCURRED IN %s:%d (%s) BY %s\n" RESET,           \
                        (message),                                                       \
                        __FILE__,                                                        \
                        __LINE__,                                                        \
                        __func__,                                                        \
                        stringify(test));                                                \
                abort();                                                                 \
            }                                                                            \
        } while (0)
#endif

#ifdef NDEBUG
    #define SOFT_ASSERT_FUNCTIONAL(test, message, command)                                 \
        do {                                                                                \
            if (!(test)) {                                                                  \
                command;                                                                    \
            }                                                                               \
        } while (0)
#else
    #define SOFT_ASSERT_FUNCTIONAL(test, message, command)                               \
        do {                                                                             \
            if (!(test)) {                                                               \
                fprintf(stderr,                                                          \
                        RED "%s\nERROR OCCURRED IN %s:%d (%s) BY %s\n" RESET,           \
                        (message),                                                       \
                        __FILE__,                                                        \
                        __LINE__,                                                        \
                        __func__,                                                        \
                        stringify(test));                                                \
                command;                                                                 \
            }                                                                            \
        } while (0)
#endif

#define SOFT_ASSERT(test, message) SOFT_ASSERT_FUNCTIONAL((test), (message), (void)0)

#endif /* LAB2_COMMON_INCLUDE_ASSERTS_H_NCLUDED */
