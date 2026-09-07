/**
 * @file test_file_io.c
 * @brief Tests for systems/file_io: read/write, empty files, mmap.
 */
#include "check.h"
#include "systems/file_io.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    CHECK(file_read_all(NULL, NULL, NULL) == ERR_INVALID_ARG);
    CHECK(file_write_all(NULL, NULL, 0) == ERR_INVALID_ARG);

    char path[] = "test_file_io_tmp.bin";
    (void)remove(path);

    const unsigned char payload[] = "hello-file-io";
    CHECK(file_write_all(path, payload, sizeof(payload) - 1) == ERR_OK);

    unsigned char *buf = NULL;
    size_t size = 0;
    CHECK(file_read_all(path, &buf, &size) == ERR_OK);
    CHECK(size == sizeof(payload) - 1);
    CHECK(memcmp(buf, payload, size) == 0);
    free(buf);

    /* Empty file must not be reported as ERR_NOMEM. */
    CHECK(file_write_all(path, NULL, 0) == ERR_OK);
    buf = NULL;
    size = 42;
    CHECK(file_read_all(path, &buf, &size) == ERR_OK);
    CHECK(size == 0);
    CHECK(buf != NULL);
    free(buf);

#ifndef _WIN32
    MappedFile mf;
    CHECK(file_mmap_read(path, &mf) == ERR_UNSUPPORTED); /* empty → no mmap */
    CHECK(file_write_all(path, payload, sizeof(payload) - 1) == ERR_OK);
    CHECK(file_mmap_read(path, &mf) == ERR_OK);
    CHECK(mf.size == sizeof(payload) - 1);
    CHECK(memcmp(mf.data, payload, mf.size) == 0);
    CHECK(file_munmap(&mf) == ERR_OK);
#endif

    CHECK(file_read_all("test_file_io_does_not_exist.bin", &buf, &size) == ERR_IO);
    (void)remove(path);
    printf("All file_io tests passed.\n");
    return 0;
}
