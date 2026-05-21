/**
 * @file logger.c
 * @brief Implementation of the minimal stderr logger.
 */

#include "logger.h"

#include <stdarg.h>
#include <time.h>

#include "colors.h"

static const char *logger_mode_string(logger_mode_type type) {
    switch (type) {
        case LOGGER_MODE_DEBUG:   return "DEBUG";
        case LOGGER_MODE_INFO:    return "INFO";
        case LOGGER_MODE_WARNING: return "WARNING";
        case LOGGER_MODE_ERROR:   return "ERROR";
        default: return "UNKNOWN";
    }
}

static const char *logger_mode_color(logger_mode_type type) {
    switch (type) {
        case LOGGER_MODE_DEBUG:   return CYAN;
        case LOGGER_MODE_INFO:    return BLUE;
        case LOGGER_MODE_WARNING: return YELLOW;
        case LOGGER_MODE_ERROR:   return RED;
        default: return "";
    }
}

static void logger_time_string(char *buffer, size_t size) {
    if (buffer == NULL || size == 0U) {
        return;
    }

    time_t current_time = time(NULL);
    struct tm time_info;
#if defined(_WIN32)
    localtime_s(&time_info, &current_time);
#else
    localtime_r(&current_time, &time_info);
#endif
    (void)strftime(buffer, size, "%H:%M:%S:%Y-%m-%d", &time_info);
}

void logger_initialize_stream(FILE *stream) {
    (void)stream;
}

int logger_initialize_file(const char *path) {
    (void)path;
    return 0;
}

void logger_close(void) {
}

void logger_log_message(logger_mode_type mode, const char *file, int line, const char *format, ...) {
    char time_buffer[32] = "";
    logger_time_string(time_buffer, sizeof(time_buffer));

    fprintf(stderr, "[%s] %s:%d. %s%s%s. ",
            time_buffer,
            (file == NULL) ? "unknown" : file,
            line,
            logger_mode_color(mode),
            logger_mode_string(mode),
            RESET);

    va_list arguments;
    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    va_end(arguments);
    fputc('\n', stderr);
}
