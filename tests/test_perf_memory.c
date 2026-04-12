/**
 * @file test_perf_memory.c
 * @brief Performance tests for memory allocators using perf_test.h.
 *
 * Compares arena vs pool vs malloc throughput and reports timing stats.
 */
#include "testing/perf_test.h"
#include "memory/arena.h"
#include "memory/pool.h"
#include <stdlib.h>
#include <string.h>

#define ALLOC_SIZE 64
#define ITERS      10000

static void bench_arena(void) {
    Arena a;
    arena_init(&a, (size_t)ALLOC_SIZE * ITERS);
    PERF_BENCH("arena_alloc", ITERS, {
        void *p = arena_alloc(&a, ALLOC_SIZE, 8);
        (void)p;
    });
    arena_destroy(&a);
}

static void bench_pool(void) {
    Pool p;
    pool_init(&p, ALLOC_SIZE, 2); /* tiny pool: alloc+free cycle */
    PERF_BENCH("pool_alloc+free", ITERS, {
        void *ptr = pool_alloc(&p);
        pool_free(&p, ptr);
    });
    pool_destroy(&p);
}

static void bench_malloc(void) {
    PERF_BENCH("malloc+free", ITERS, {
        void *p = malloc(ALLOC_SIZE);
        free(p);
    });
}

int main(void) {
    printf("=== Memory Allocator Performance ===\n\n");
    bench_arena();
    bench_pool();
    bench_malloc();
    printf("\nDone.\n");
    return 0;
}
