/**
 * @file systems_demo.c
 * @brief Demonstrates systems programming: file I/O, mmap, directory walking,
 *        process capture, fork/exec, and signal handling.
 */
#include "systems/file_io.h"
#include "systems/process.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Directory walk callback ────────────────────────────────────────────── */
static bool print_entry(const char *name, bool is_dir, void *user_data) {
    (void)user_data;
    printf("  %s %s\n", is_dir ? "[DIR]" : "     ", name);
    return true; /* continue */
}

/* ── Signal handler ─────────────────────────────────────────────────────── */
static volatile int g_interrupted = 0;
static void on_sigint(int sig) {
    (void)sig;
    g_interrupted = 1;
}

int main(void) {
    ErrorCode err;
    const char *tmp_path = "/tmp/systems_demo.txt";
    const char *msg = "Hello from systems_demo!\n";

    /* 1. Write a file */
    err = file_write_all(tmp_path, (const unsigned char *)msg, strlen(msg));
    printf("file_write_all: %s\n", error_str(err));

    /* 2. Read it back */
    unsigned char *buf = NULL;
    size_t sz = 0;
    err = file_read_all(tmp_path, &buf, &sz);
    printf("file_read_all : %s (%zu bytes) -> %.*s", error_str(err), sz, (int)sz, buf);
    free(buf);

    /* 3. Memory-map the file read-only */
    MappedFile mf = {0};
    err = file_mmap_read(tmp_path, &mf);
    if (err == ERR_OK) {
        printf("mmap read     : %zu bytes -> %.*s", mf.size, (int)mf.size, (char *)mf.data);
        file_munmap(&mf);
    } else {
        printf("mmap read     : %s\n", error_str(err));
    }

    /* 4. Walk current directory */
    printf("\nDirectory listing of '.':\n");
    dir_walk(".", print_entry, NULL);

    /* 5. Capture command output */
    char *out = NULL;
    size_t out_len = 0;
    err = process_capture("echo 'captured output'", &out, &out_len);
    printf("\nprocess_capture: %s -> %s", error_str(err), out ? out : "(null)\n");
    free(out);

    /* 6. Fork/exec */
    char *argv[] = {"echo", "fork/exec works", NULL};
    int status = 0;
    err = process_exec("echo", argv, &status);
    printf("process_exec  : %s (exit %d)\n", error_str(err), status);

    /* 7. Signal handling */
    process_on_sigint(on_sigint);
    printf("\nSIGINT handler installed (Ctrl-C sets flag). Flag = %d\n", g_interrupted);
    process_on_sigint(NULL); /* restore default */

    /* Cleanup */
    remove(tmp_path);
    return 0;
}
