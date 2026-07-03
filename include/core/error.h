/**
 * @file error.h
 * @brief Unified error handling for the project.
 */
#ifndef CORE_ERROR_H
#define CORE_ERROR_H

#include <stdbool.h>

/**
 * @brief Error codes returned by library functions.
 */
typedef enum {
    ERR_OK = 0,      /**< Success (no error). */
    ERR_NOMEM,       /**< Memory allocation failed. */
    ERR_IO,          /**< I/O operation failed. */
    ERR_INVALID_ARG, /**< Invalid argument passed. */
    ERR_OVERFLOW,    /**< Numeric or buffer overflow. */
    ERR_NOT_FOUND,   /**< Requested item not found. */
    ERR_UNSUPPORTED, /**< Operation not supported. */
} ErrorCode;

/**
 * @brief Return human-readable string for an error code.
 * @param code The error code to describe.
 * @return Static string describing the error.
 */
const char *error_str(ErrorCode code);

/** Log an error with file/line context. */
#define LOG_ERROR(code) error_log_impl((code), __FILE__, __LINE__)

/**
 * @brief Log an error with source location (used by LOG_ERROR macro).
 * @param code The error code to log.
 * @param file Source file name (typically __FILE__).
 * @param line Source line number (typically __LINE__).
 */
void error_log_impl(ErrorCode code, const char *file, int line);

#endif /* CORE_ERROR_H */
