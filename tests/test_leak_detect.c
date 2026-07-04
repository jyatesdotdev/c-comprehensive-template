/**
 * @file test_leak_detect.c
 * @brief Tests for the leak detection allocator.
 *
 * Calls the tracking functions directly (not via macros) so we can
 * verify the report counts without macro interference.
 */
#include "memory/leak_detect.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(cond)                                                                    \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

int main(void) {
    /* Test 1: no leaks when everything is freed */
    leak_detect_init();
    void *a = leak_detect_malloc(128, __FILE__, __LINE__);
    void *b = leak_detect_calloc(4, 32, __FILE__, __LINE__);
    CHECK(a != NULL && b != NULL);
    leak_detect_free(a, __FILE__, __LINE__);
    leak_detect_free(b, __FILE__, __LINE__);
    CHECK(leak_detect_report() == 0);
    leak_detect_shutdown();

    /* Test 2: detect a leak */
    leak_detect_init();
    void *c = leak_detect_malloc(64, __FILE__, __LINE__);
    CHECK(c != NULL);
    /* intentionally not freed */
    CHECK(leak_detect_report() == 1);
    /* clean up to avoid real leak */
    free(c);
    leak_detect_shutdown();

    /* Test 3: realloc tracking */
    leak_detect_init();
    void *d = leak_detect_malloc(16, __FILE__, __LINE__);
    d = leak_detect_realloc(d, 256, __FILE__, __LINE__);
    CHECK(d != NULL);
    leak_detect_free(d, __FILE__, __LINE__);
    CHECK(leak_detect_report() == 0);
    leak_detect_shutdown();

    printf("All leak_detect tests passed.\n");
    return 0;
}
