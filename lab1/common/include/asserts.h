#ifndef ASSERTS_H_INCLUDED
#define ASSERTS_H_INCLUDED

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
#define SOFT_ASSERT_FUNCTIONAL(test, message, command) SOFT_ASSERT_FUNCTIONAL((test), (message), (command))

#endif
