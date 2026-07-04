/**
 * @file test_arena.c
 * @brief Tests for the arena (bump) allocator.
 */
#include "memory/arena.h"

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
    CHECK(arena_init(NULL, 1024) == ERR_INVALID_ARG);
    Arena a;
    CHECK(arena_init(&a, 0) == ERR_INVALID_ARG);
    CHECK(arena_init(&a, 1024) == ERR_OK);

    void *p = arena_alloc(&a, 64, 8);
    CHECK(p != NULL);
    CHECK(((size_t)(unsigned char *)p % 8) == 0); /* honors alignment */

    /* Allocate until full, then reset makes room again */
    while (arena_alloc(&a, 64, 8)) {}
    CHECK(arena_alloc(&a, 64, 8) == NULL);
    arena_reset(&a);
    p = arena_alloc(&a, 64, 8);
    CHECK(p != NULL);

    /* Oversized request fails cleanly */
    CHECK(arena_alloc(&a, 4096, 8) == NULL);

    arena_destroy(&a);
    arena_destroy(&a); /* double-destroy is safe */
    arena_destroy(NULL);
    printf("All arena tests passed.\n");
    return 0;
}
