/**
 * @file error.c
 * @brief Error code to string mapping and logging implementation.
 */
#include "core/error.h"
#include <stdio.h>

/** @brief Human-readable strings indexed by ErrorCode. */
static const char *error_strings[] = {
    [ERR_OK]          = "success",
    [ERR_NOMEM]       = "out of memory",
    [ERR_IO]          = "I/O error",
    [ERR_INVALID_ARG] = "invalid argument",
    [ERR_OVERFLOW]    = "overflow",
    [ERR_NOT_FOUND]   = "not found",
    [ERR_UNSUPPORTED] = "unsupported operation",
};

const char *error_str(ErrorCode code) {
    if (code < 0 || (size_t)code >= sizeof(error_strings) / sizeof(error_strings[0]))
        return "unknown error";
    return error_strings[code];
}

void error_log_impl(ErrorCode code, const char *file, int line) {
    fprintf(stderr, "[ERROR] %s:%d: %s\n", file, line, error_str(code));
}
