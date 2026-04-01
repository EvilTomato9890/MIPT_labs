#ifndef LAB3_COMMON_INCLUDE_RETURN_MACROS_H_NCLUDED
#define LAB3_COMMON_INCLUDE_RETURN_MACROS_H_NCLUDED

#include "logger.h"

#define RETURN_IF_ERROR(test, status, ...)                  \
    do {                                                    \
        if (test) {                                         \
            LOGGER_ERROR(__VA_ARGS__);                      \
            return (status);                                \
        }                                                   \
    } while (0)

#define RETURN_IF_ERROR_CLEANUP(test, status, cleanup, ...) \
    do {                                                    \
        if (test) {                                         \
            LOGGER_ERROR(__VA_ARGS__);                      \
            do { cleanup; } while (0);                      \
            return (status);                                \
        }                                                   \
    } while (0)

#endif /* LAB3_COMMON_INCLUDE_RETURN_MACROS_H_NCLUDED */
