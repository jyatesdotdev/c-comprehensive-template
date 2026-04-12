/**
 * @file process.h
 * @brief Process control: shell commands, fork/exec, pipes, signals.
 */
#ifndef SYSTEMS_PROCESS_H
#define SYSTEMS_PROCESS_H

#include "core/error.h"
#include <stddef.h>

/**
 * @brief Run a shell command and return its exit status.
 * @param cmd         Shell command string.
 * @param exit_status Receives the command's exit status.
 * @return ERR_OK on success, ERR_IO on failure.
 */
ErrorCode process_run(const char *cmd, int *exit_status);

/**
 * @brief Run a command and capture its stdout into a caller-freed buffer.
 * @param cmd     Shell command to execute.
 * @param out_buf Receives malloc'd output (null-terminated).
 * @param out_len Receives byte count (excluding null terminator).
 * @return ERR_OK on success, ERR_IO on failure, ERR_NOMEM on allocation failure.
 */
ErrorCode process_capture(const char *cmd, char **out_buf, size_t *out_len);

/**
 * @brief Fork and exec a program (POSIX only).
 *
 * Parent waits for child and returns exit status.
 * @param prog        Path to executable.
 * @param argv        NULL-terminated argument array (argv[0] = prog name).
 * @param exit_status Receives child exit status.
 * @return ERR_OK on success, ERR_IO on fork/exec failure.
 */
ErrorCode process_exec(const char *prog, char *const argv[], int *exit_status);

/** @brief Signal handler function signature. */
typedef void (*SignalHandler)(int);

/**
 * @brief Install a handler for SIGINT.
 * @param handler Handler function, or NULL to restore default.
 * @return ERR_OK on success, ERR_UNSUPPORTED on failure.
 */
ErrorCode process_on_sigint(SignalHandler handler);

#endif /* SYSTEMS_PROCESS_H */
