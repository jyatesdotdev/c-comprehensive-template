/**
 * @file test_pool.c
 * @brief Tests for the fixed-size pool allocator.
 */
#include "memory/pool.h"

#include <stdint.h>
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
    Pool p;
    CHECK(pool_init(NULL, 32, 4) == ERR_INVALID_ARG);
    CHECK(pool_init(&p, 32, 0) == ERR_INVALID_ARG);
    CHECK(pool_init(&p, SIZE_MAX / 2, 4) == ERR_OVERFLOW);
    CHECK(pool_init(&p, 32, 4) == ERR_OK);

    void *a = pool_alloc(&p);
    void *b = pool_alloc(&p);
    CHECK(a != NULL && b != NULL && a != b);

    /* Freed blocks are reused (LIFO free list) */
    pool_free(&p, a);
    void *c = pool_alloc(&p);
    CHECK(c == a);

    /* Exhaustion returns NULL rather than overrunning */
    void *d = pool_alloc(&p);
    void *e = pool_alloc(&p);
    CHECK(d != NULL && e != NULL);
    CHECK(pool_alloc(&p) == NULL);

    pool_free(&p, NULL); /* safe no-op */
    pool_destroy(&p);
    pool_destroy(&p); /* double-destroy is safe */
    pool_destroy(NULL);
    printf("All pool tests passed.\n");
    return 0;
}
