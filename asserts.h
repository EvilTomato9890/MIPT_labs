#ifndef COMMON_ASSERTS_H_INCLUDED
#define COMMON_ASSERTS_H_INCLUDED

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "colors.h"

#if defined(__GNUC__) || defined(__clang__)
    #define ASSERTS_FUNCTION_NAME __PRETTY_FUNCTION__
#else
    #define ASSERTS_FUNCTION_NAME __func__
#endif

#define ASSERTS_STRINGIFY_IMPL(expr) #expr
#define ASSERTS_STRINGIFY(expr) ASSERTS_STRINGIFY_IMPL(expr)

//REVIEW - Как назвать и стоит ли делат ьмакрос, которые еще и в логи херачит (в частности ошибки)
#define RETURN_IF(condition, return_value)                                     \
    do {                                                                       \
        if (condition) {                                                       \
            return (return_value);                                             \
        }                                                                      \
    } while (0)

#define CLEANUP_AND_RETURN_IF(condition, cleanup, return_value)                    \
    do {                                                                       \
        if (condition) {                                                       \
            cleanup                                                            \
            return (return_value);                                             \
        }                                                                      \
    } while (0)

static inline void asserts_vprint_message(const char* format, va_list args) {
    vfprintf(stderr, format, args);
}

static inline void asserts_report_ice(const char* file, int line, 
                                      const char* function_name, const char* format, ...) {
    va_list args = {};

    fprintf(stderr,
            RED_CONSOLE BOLD_CONSOLE
            "internal compiler error: "
            RESET_CONSOLE);

    va_start(args, format);
    asserts_vprint_message(format, args);
    va_end(args);

    fprintf(stderr, "\n");
    fprintf(stderr,
            RED_CONSOLE
            "  at %s:%d in %s\n"
            RESET_CONSOLE,
            file, line, function_name);

    abort();
}

static inline void asserts_report_assertion(const char* expression, 
                                            const char* file, int line, const char* function_name) {
    fprintf(stderr,
            RED_CONSOLE BOLD_CONSOLE
            "assertion failed: %s\n"
            RESET_CONSOLE,
            expression);
    fprintf(stderr,
            RED_CONSOLE
            "  at %s:%d in %s\n"
            RESET_CONSOLE,
            file, line, function_name);

    abort();
}

static inline void asserts_report_assertion_message(const char* expression,
                                                    const char* file, int line, const char* function_name,
                                                    const char* format, ...) {
    va_list args = {};

    fprintf(stderr,
            RED_CONSOLE BOLD_CONSOLE
            "assertion failed: %s\n"
            RESET_CONSOLE,
            expression);
    fprintf(stderr, RED_CONSOLE "message: " RESET_CONSOLE);

    va_start(args, format);
    asserts_vprint_message(format, args);
    va_end(args);

    fprintf(stderr, "\n");
    fprintf(stderr,
            RED_CONSOLE
            "  at %s:%d in %s\n"
            RESET_CONSOLE,
            file, line, function_name);

    abort();
}

#define ICE(...)                                                               \
    do {                                                                       \
        asserts_report_ice(__FILE__, __LINE__, ASSERTS_FUNCTION_NAME,          \
                           __VA_ARGS__);                                       \
    } while (0)

#define ICE_IF(test, ...)                                                      \
    do {                                                                       \
        if (test) {                                                            \
            asserts_report_ice(__FILE__, __LINE__, ASSERTS_FUNCTION_NAME,      \
                               __VA_ARGS__);                                   \
        }                                                                      \
    } while (0)

#define ICE_REQUIRE(test, ...)                                                 \
    do {                                                                       \
        if (!(test)) {                                                         \
            asserts_report_ice(__FILE__, __LINE__, ASSERTS_FUNCTION_NAME,      \
                               __VA_ARGS__);                                   \
        }                                                                      \
    } while (0)

#ifdef NDEBUG

#define ASSERT(test) ((void)0)

#define ASSERT_MSG(test, ...) ((void)0)

#else

#define ASSERT(test)                                                           \
    do {                                                                       \
        if (!(test)) {                                                         \
            asserts_report_assertion(ASSERTS_STRINGIFY(test), __FILE__,        \
                                     __LINE__, ASSERTS_FUNCTION_NAME);         \
        }                                                                      \
    } while (0)

#define ASSERT_MSG(test, ...)                                                  \
    do {                                                                       \
        if (!(test)) {                                                         \
            asserts_report_assertion_message(ASSERTS_STRINGIFY(test),          \
                                             __FILE__, __LINE__,               \
                                             ASSERTS_FUNCTION_NAME,            \
                                             __VA_ARGS__);                     \
        }                                                                      \
    } while (0)

#endif


#endif /* COMMON_ASSERTS_H_INCLUDED */