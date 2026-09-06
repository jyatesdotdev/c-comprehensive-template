/**
 * @file process.h
 * @brief Process control: shell commands, fork/exec, pipes, signals.
 */
#ifndef SYSTEMS_PROCESS_H
#define SYSTEMS_PROCESS_H

#include "core/error.h"
#include <stddef.h>

/**
 * @brief Run a shell command (`system()`) and return its wait status.
 *
 * `cmd` is passed to the shell. Never interpolate untrusted input; use
 * process_exec() for an argv exec without a shell.
 * @param cmd         Shell command string.
 * @param exit_status Receives the wait status from system() (not WEXITSTATUS).
 * @return ERR_OK on success, ERR_INVALID_ARG, or ERR_IO if system() fails.
 */
ErrorCode process_run(const char *cmd, int *exit_status);

/**
 * @brief Run a shell command (`popen()`) and capture stdout into a caller-freed buffer.
 *
 * Same injection rules as process_run: `cmd` is a shell command.
 * @param cmd     Shell command to execute.
 * @param out_buf Receives malloc'd output (null-terminated). Caller frees.
 * @param out_len Receives byte count (excluding null terminator).
 * @return ERR_OK on success, ERR_INVALID_ARG, ERR_IO, ERR_NOMEM, or ERR_OVERFLOW.
 */
ErrorCode process_capture(const char *cmd, char **out_buf, size_t *out_len);

/**
 * @brief Fork and exec a program without a shell (POSIX only).
 *
 * Parent waits for the child. execvp failure is reported as ERR_OK with
 * *exit_status == 127. Windows returns ERR_UNSUPPORTED.
 * @param prog        Path to executable.
 * @param argv        NULL-terminated argument array (argv[0] = prog name).
 * @param exit_status Receives child exit status.
 * @return ERR_OK on success (including exec-fail 127), ERR_INVALID_ARG,
 *         ERR_IO on fork/waitpid failure, ERR_UNSUPPORTED on Windows.
 */
ErrorCode process_exec(const char *prog, char *const argv[], int *exit_status);

/** @brief Signal handler function signature. */
typedef void (*SignalHandler)(int);

/**
 * @brief Install a handler for SIGINT (sigaction on POSIX, signal() on Windows).
 * @param handler Handler function, or NULL to restore default.
 * @return ERR_OK on success, ERR_IO on failure.
 */
ErrorCode process_on_sigint(SignalHandler handler);

#endif /* SYSTEMS_PROCESS_H */
