#include "logger.h"

#include <stdarg.h>
#include <time.h>

#include "asserts.h"
#include "colors.h"
#include "return_macros.h"

static FILE *output_stream = NULL;
static logger_output_type output_type = LOGGER_OUTPUT_EXTERNAL_STREAM;
static int color_enabled = 1;

static const char *logger_mode_string(logger_mode_type type);
static void logger_time_string(char *buffer, size_t size);
static const char *logger_color_on(logger_mode_type mode);

#if defined(_WIN32)
static struct tm *portable_localtime_r(const time_t *timer, struct tm *result) {
    return (localtime_s(result, timer) == 0) ? result : NULL;
}
#define localtime_r portable_localtime_r
#endif

static const char *logger_mode_string(logger_mode_type type) {
    switch (type) {
        case LOGGER_MODE_DEBUG:   return "DEBUG";
        case LOGGER_MODE_INFO:    return "INFO";
        case LOGGER_MODE_WARNING: return "WARNING";
        case LOGGER_MODE_ERROR:   return "ERROR";
        default:
            SOFT_ASSERT_FUNCTIONAL(0, "Wrong logger mode", return "?");
            return "?";
    }
}

static void logger_time_string(char *buffer, size_t size) {
    HARD_ASSERT(buffer != NULL, "Buffer pointer must not be NULL");

    time_t current_time = time(NULL);
    struct tm time_info = {0};
    if (localtime_r(&current_time, &time_info) == NULL) {
        SOFT_ASSERT(0, "Invalid time input");
        buffer[0] = '\0';
        return;
    }
    if (strftime(buffer, size, "%H:%M:%S:%Y-%m-%d", &time_info) == 0U) {
        SOFT_ASSERT(0, "Invalid time input");
        buffer[0] = '\0';
    }
}

static const char *logger_color_on(logger_mode_type mode) {
    if (!color_enabled) {
        return "";
    }
    switch (mode) {
        case LOGGER_MODE_DEBUG:   return CYAN;
        case LOGGER_MODE_INFO:    return BLUE;
        case LOGGER_MODE_WARNING: return YELLOW;
        case LOGGER_MODE_ERROR:   return RED;
        default:                  return "";
    }
}

void logger_initialize_stream(FILE *stream) {
    if (output_type == LOGGER_OUTPUT_OWNED_FILE && output_stream != NULL) {
        fclose(output_stream);
    }
    output_stream = (stream != NULL) ? stream : stderr;
    output_type = LOGGER_OUTPUT_EXTERNAL_STREAM;
    color_enabled = 1;
}

int logger_initialize_file(const char *path) {
    SOFT_ASSERT_FUNCTIONAL(path != NULL, "File path must not be NULL", return 1);

    FILE *file = fopen(path, "a");
    RETURN_IF_ERROR(file == NULL, 1, "logger: failed to open file '%s'", path);

    if (output_type == LOGGER_OUTPUT_OWNED_FILE && output_stream != NULL) {
        fclose(output_stream);
    }
    output_stream = file;
    output_type = LOGGER_OUTPUT_OWNED_FILE;
    color_enabled = 0;
    return 0;
}

void logger_close(void) {
    if (output_type == LOGGER_OUTPUT_OWNED_FILE && output_stream != NULL) {
        fclose(output_stream);
    }
    output_stream = NULL;
    output_type = LOGGER_OUTPUT_EXTERNAL_STREAM;
    color_enabled = 1;
}

void logger_log_message(logger_mode_type mode, const char *file, int line, const char *format, ...) {
    SOFT_ASSERT_FUNCTIONAL(format != NULL, "Format string must not be NULL", return);

    if (output_stream == NULL) {
        logger_initialize_stream(NULL);
    }

    char time_buffer[32] = "";
    logger_time_string(time_buffer, sizeof(time_buffer));
    if (output_type != LOGGER_OUTPUT_EXTERNAL_STREAM) {
        fprintf(output_stream, "%s. %s:%d. %s. ",
                time_buffer,
                (file != NULL) ? file : "unknown",
                line,
                logger_mode_string(mode));
    } else {
        const char *color_on = logger_color_on(mode);
        fprintf(output_stream, "[%s] %s:%d. %s%s%s. ",
                time_buffer,
                (file != NULL) ? file : "unknown",
                line,
                color_on,
                logger_mode_string(mode),
                RESET);
    }

    va_list arguments;
    va_start(arguments, format);
    vfprintf(output_stream, format, arguments);
    va_end(arguments);
    fputc('\n', output_stream);
}
