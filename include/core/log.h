/**
 * @file log.h
 * @brief Leveled logging: timestamps, runtime level filter, optional color.
 *
 * Each log call produces one atomic line on the configured stream (stderr
 * by default), so concurrent threads never interleave partial messages.
 * Color is applied when the stream is a TTY and NO_COLOR is unset.
 *
 * For logging an ErrorCode with source location, see LOG_ERROR_CODE in
 * core/error.h.
 */
#ifndef CORE_LOG_H
#define CORE_LOG_H

#include <stdio.h>

/** @brief Log severity, in increasing order. */
typedef enum {
    LOG_LEVEL_DEBUG = 0, /**< Verbose diagnostics, compiled out of hot paths. */
    LOG_LEVEL_INFO,      /**< Normal operational messages. */
    LOG_LEVEL_WARN,      /**< Something unexpected but recoverable. */
    LOG_LEVEL_ERROR,     /**< Operation failed. */
} LogLevel;

/** Marks a function as printf-style so compilers type-check its format arguments. */
#if defined(__GNUC__) || defined(__clang__)
#define LOG_PRINTF_FORMAT(fmt_idx, arg_idx) __attribute__((format(printf, fmt_idx, arg_idx)))
#else
#define LOG_PRINTF_FORMAT(fmt_idx, arg_idx)
#endif

/**
 * @brief Set the minimum level that gets emitted (default LOG_LEVEL_INFO).
 * @param min_level Messages below this level are dropped.
 */
void log_set_level(LogLevel min_level);

/** @brief Current minimum level. */
LogLevel log_get_level(void);

/**
 * @brief Redirect log output (default stderr). Pass NULL to reset to stderr.
 * @param stream Destination stream; caller keeps ownership.
 */
void log_set_stream(FILE *stream);

/**
 * @brief Emit one log line: "HH:MM:SS.mmm LEVEL message\n".
 *        Prefer the LOG_* macros below.
 * @param level Severity.
 * @param fmt   printf-style format string.
 * @param ...   Format arguments.
 */
void log_msg(LogLevel level, const char *fmt, ...) LOG_PRINTF_FORMAT(2, 3);

/** Log at DEBUG level. */
#define LOG_DEBUG(...) log_msg(LOG_LEVEL_DEBUG, __VA_ARGS__)
/** Log at INFO level. */
#define LOG_INFO(...) log_msg(LOG_LEVEL_INFO, __VA_ARGS__)
/** Log at WARN level. */
#define LOG_WARN(...) log_msg(LOG_LEVEL_WARN, __VA_ARGS__)
/** Log at ERROR level. */
#define LOG_ERROR(...) log_msg(LOG_LEVEL_ERROR, __VA_ARGS__)

#endif /* CORE_LOG_H */
