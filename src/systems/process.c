/**
 * @file process.c
 * @brief Process execution, output capture, and signal handling.
 */
#include "systems/process.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifndef _WIN32
#include <unistd.h>
#include <sys/wait.h>
#endif

ErrorCode process_run(const char *cmd, int *exit_status) {
    if (!cmd || !exit_status) return ERR_INVALID_ARG;
    /* NOLINTNEXTLINE(cert-env33-c,concurrency-mt-unsafe) — wrapping command execution is this module's purpose; callers own cmd sanitization */
    int ret = system(cmd); /* Flawfinder: ignore — cmd from caller */
    if (ret == -1) return ERR_IO;
    *exit_status = ret;
    return ERR_OK;
}

ErrorCode process_capture(const char *cmd, char **out_buf, size_t *out_len) {
    if (!cmd || !out_buf || !out_len) return ERR_INVALID_ARG;

    /* NOLINTNEXTLINE(cert-env33-c) — wrapping command execution is this module's purpose; callers own cmd sanitization */
    FILE *fp = popen(cmd, "r"); /* Flawfinder: ignore — cmd from caller */
    if (!fp) return ERR_IO;

    size_t cap = 1024, len = 0;
    char  *buf = malloc(cap);
    if (!buf) {
        pclose(fp);
        return ERR_NOMEM;
    }

    size_t n;
    while ((n = fread(buf + len, 1, cap - len - 1, fp)) > 0) {
        len += n;
        if (len + 1 >= cap) {
            cap *= 2;
            char *tmp = realloc(buf, cap);
            if (!tmp) {
                free(buf);
                pclose(fp);
                return ERR_NOMEM;
            }
            buf = tmp;
        }
    }
    buf[len] = '\0';
    pclose(fp);

    *out_buf = buf;
    *out_len = len;
    return ERR_OK;
}

#ifndef _WIN32
ErrorCode process_exec(const char *prog, char *const argv[], int *exit_status) {
    if (!prog || !argv || !exit_status) return ERR_INVALID_ARG;

    pid_t pid = fork();
    if (pid < 0) return ERR_IO;

    if (pid == 0) {
        /* Child */
        execvp(prog, argv); /* Flawfinder: ignore — prog/argv from caller */
        _exit(127);         /* exec failed */
    }

    /* Parent: wait for child */
    int wstatus;
    if (waitpid(pid, &wstatus, 0) < 0) return ERR_IO;
    *exit_status = WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : -1;
    return ERR_OK;
}
#else
ErrorCode process_exec(const char *prog, char *const argv[], int *exit_status) {
    (void)prog;
    (void)argv;
    (void)exit_status;
    return ERR_UNSUPPORTED;
}
#endif

ErrorCode process_on_sigint(SignalHandler handler) {
    if (signal(SIGINT, handler ? handler : SIG_DFL) == SIG_ERR) return ERR_IO;
    return ERR_OK;
}
