#include <stdio.h>
#include <stdlib.h>
#include "memory/pool.h"

#define ASSERT(cond)                                                         \
    do {                                                                     \
        if (!(cond)) {                                                       \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            exit(1);                                                         \
        }                                                                    \
    } while (0)

int main(void) {
    Pool p;
    ASSERT(pool_init(&p, 32, 4) == ERR_OK);
    void *a = pool_alloc(&p);
    void *b = pool_alloc(&p);
    ASSERT(a != NULL && b != NULL);
    pool_free(&p, a);
    void *c = pool_alloc(&p);
    ASSERT(c == a); /* reuses freed block */
    pool_destroy(&p);
    printf("test_pool: PASS\n");
    return 0;
}
