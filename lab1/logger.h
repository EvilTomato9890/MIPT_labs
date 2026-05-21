/**
 * @file logger.h
 * @brief Minimal logging API used by the project.
 */

#ifndef LAB1_LOGGER_H_NCLUDED
#define LAB1_LOGGER_H_NCLUDED

#include <stdio.h>

/**
 * @brief Severity level of a log message.
 */
typedef enum logger_mode_type {
    /** Debug-level diagnostic message. */
    LOGGER_MODE_DEBUG = 0,
    /** Informational message. */
    LOGGER_MODE_INFO = 1,
    /** Warning message. */
    LOGGER_MODE_WARNING = 2,
    /** Error message. */
    LOGGER_MODE_ERROR = 3
} logger_mode_type;

/**
 * @brief Initializes logger output stream.
 *
 * @param stream Output stream. The current implementation ignores this parameter.
 */
void logger_initialize_stream(FILE *stream);

/**
 * @brief Initializes logger output file.
 *
 * @param path Path to a log file. The current implementation ignores this parameter.
 * @return 0 on success.
 */
int logger_initialize_file(const char *path);

/**
 * @brief Closes logger resources.
 */
void logger_close(void);

/**
 * @brief Writes a formatted log message to stderr.
 *
 * @param mode Log severity.
 * @param file Source file name.
 * @param line Source line number.
 * @param format printf-compatible format string.
 */
void logger_log_message(logger_mode_type mode, const char *file, int line, const char *format, ...);

#ifdef LOGGER_ALL
    /**
     * @brief Logs a debug message when LOGGER_ALL is enabled.
     */
    #define LOGGER_DEBUG(...) logger_log_message(LOGGER_MODE_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
    /**
     * @brief Logs an info message when LOGGER_ALL is enabled.
     */
    #define LOGGER_INFO(...) logger_log_message(LOGGER_MODE_INFO, __FILE__, __LINE__, __VA_ARGS__)
    /**
     * @brief Logs a warning message when LOGGER_ALL is enabled.
     */
    #define LOGGER_WARNING(...) logger_log_message(LOGGER_MODE_WARNING, __FILE__, __LINE__, __VA_ARGS__)
#else
    /**
     * @brief Disabled debug logger.
     */
    #define LOGGER_DEBUG(...) (void)0
    /**
     * @brief Disabled info logger.
     */
    #define LOGGER_INFO(...) (void)0
    /**
     * @brief Disabled warning logger.
     */
    #define LOGGER_WARNING(...) (void)0
#endif

/**
 * @brief Logs an error message.
 */
#define LOGGER_ERROR(...) logger_log_message(LOGGER_MODE_ERROR, __FILE__, __LINE__, __VA_ARGS__)

#endif /* LAB1_LOGGER_H_NCLUDED */
