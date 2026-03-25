#ifndef LAB2_LOGGER_H_INCLUDED
#define LAB2_LOGGER_H_INCLUDED

#include <stdio.h>

typedef enum logger_mode_type {
    LOGGER_MODE_DEBUG = 0,
    LOGGER_MODE_INFO = 1,
    LOGGER_MODE_WARNING = 2,
    LOGGER_MODE_ERROR = 3
} logger_mode_type;

void logger_initialize_stream(FILE *stream);
int logger_initialize_file(const char *path);
void logger_close(void);

void logger_log_message(logger_mode_type mode, const char *file, int line, const char *format, ...);

#ifdef LOGGER_ALL
    #define LOGGER_DEBUG(...) logger_log_message(LOGGER_MODE_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
    #define LOGGER_INFO(...) logger_log_message(LOGGER_MODE_INFO, __FILE__, __LINE__, __VA_ARGS__)
    #define LOGGER_WARNING(...) logger_log_message(LOGGER_MODE_WARNING, __FILE__, __LINE__, __VA_ARGS__)
#else
    #define LOGGER_DEBUG(...) (void)0
    #define LOGGER_INFO(...) (void)0
    #define LOGGER_WARNING(...) (void)0
#endif

#define LOGGER_ERROR(...) logger_log_message(LOGGER_MODE_ERROR, __FILE__, __LINE__, __VA_ARGS__)

#endif /* LAB2_LOGGER_H_INCLUDED */
