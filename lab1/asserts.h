/**
 * @file asserts.h
 * @brief Assertion helpers used by the data structures and benchmarks.
 */

#ifndef ASSERTS_H_INCLUDED
#define ASSERTS_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>

#include "colors.h"

/**
 * @brief Converts a macro argument to a string literal.
 */
#define stringify(a) #a

#ifdef NDEBUG
    /**
     * @brief Disabled hard assertion for release builds.
     */
    #define HARD_ASSERT(...) (void)0
#else
    /**
     * @brief Prints a diagnostic message and aborts if the condition is false.
     *
     * @param test Condition that must be true.
     * @param message Message printed before aborting.
     */
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
    /**
     * @brief Executes a custom command if the condition is false.
     *
     * @param test Condition that must be true.
     * @param message Message used in debug builds.
     * @param command Command executed when the condition is false.
     */
    #define SOFT_ASSERT_FUNCTIONAL(test, message, command)                                 \
        do {                                                                                \
            if (!(test)) {                                                                  \
                command;                                                                    \
            }                                                                               \
        } while (0)
#else
    /**
     * @brief Prints a diagnostic message and executes a custom command if the condition is false.
     *
     * @param test Condition that must be true.
     * @param message Message printed when the condition is false.
     * @param command Command executed when the condition is false.
     */
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

/**
 * @brief Soft assertion that only reports a failed condition in debug builds.
 */
#define SOFT_ASSERT(test, message) SOFT_ASSERT_FUNCTIONAL((test), (message), (void)0)

/**
 * @brief Backward-compatible misspelled alias for SOFT_ASSERT_FUNCTIONAL.
 */
#define SOFT_ASSERT_FUCTIONAL(test, message, command) SOFT_ASSERT_FUNCTIONAL((test), (message), (command))

#endif
