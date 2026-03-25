#ifndef LAB2_RETURN_MACROS_H_INCLUDED
#define LAB2_RETURN_MACROS_H_INCLUDED

#include "logger.h"

#define RETURN_IF_FAIL(test, message)            \
    do {                                         \
        if (!(test)) {                           \
            LOGGER_ERROR("%s", (message));      \
            return;                              \
        }                                        \
    } while (0)

#define RETURN_VAL_IF_FAIL(test, value, message) \
    do {                                         \
        if (!(test)) {                           \
            LOGGER_ERROR("%s", (message));      \
            return (value);                      \
        }                                        \
    } while (0)

#endif
