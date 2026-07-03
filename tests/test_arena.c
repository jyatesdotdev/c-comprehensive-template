/* Minimal test runner macro */
#include <stdio.h>
#include <stdlib.h>
#include "memory/arena.h"

#define ASSERT(cond)                                                         \
    do {                                                                     \
        if (!(cond)) {                                                       \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            exit(1);                                                         \
        }                                                                    \
    } while (0)

int main(void) {
    Arena a;
    ASSERT(arena_init(&a, 1024) == ERR_OK);
    void *p = arena_alloc(&a, 64, 8);
    ASSERT(p != NULL);
    /* Allocate until full */
    while (arena_alloc(&a, 64, 8)) {}
    arena_reset(&a);
    p = arena_alloc(&a, 64, 8);
    ASSERT(p != NULL);
    arena_destroy(&a);
    printf("test_arena: PASS\n");
    return 0;
}
