/**
 * @file benchmark_demo.c
 * @brief Performance benchmarking and optimization demonstration.
 *
 * Demonstrates measurable performance differences from:
 * - Cache-friendly vs cache-unfriendly memory access
 * - SIMD vs scalar computation
 * - Arena/pool allocator vs malloc
 * - Branch prediction effects
 */
#include "testing/perf_test.h"
#include "memory/arena.h"
#include "memory/pool.h"
#include "hpc/simd_ops.h"

#include <stdlib.h>
#include <string.h>

/* Prevent compiler from optimizing away results. */
static volatile float sink_f;
static volatile int   sink_i;

#define N     (1 << 20) /* ~1M elements */
#define ITERS 100

/* ── Cache: Row-major vs Column-major ─────────────────────────────────── */

#define ROWS 1024
#define COLS 1024
static float matrix[ROWS][COLS];

static void bench_row_major(void) {
    float sum = 0;
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++) sum += matrix[r][c];
    sink_f = sum;
}

static void bench_col_major(void) {
    float sum = 0;
    for (int c = 0; c < COLS; c++)
        for (int r = 0; r < ROWS; r++) sum += matrix[r][c];
    sink_f = sum;
}

/* ── SIMD vs Scalar ───────────────────────────────────────────────────── */

static float a_arr[N], b_arr[N], dst[N];

static void scalar_add(float *d, const float *a, const float *b, size_t n) {
    for (size_t i = 0; i < n; i++) d[i] = a[i] + b[i];
}

/* ── Allocator Comparison ─────────────────────────────────────────────── */

#define ALLOC_COUNT 10000
#define ALLOC_SIZE  64

static void bench_malloc_free(void) {
    void *ptrs[ALLOC_COUNT];
    for (int i = 0; i < ALLOC_COUNT; i++) ptrs[i] = malloc(ALLOC_SIZE);
    for (int i = 0; i < ALLOC_COUNT; i++) free(ptrs[i]);
}

static void bench_arena_alloc(Arena *a) {
    for (int i = 0; i < ALLOC_COUNT; i++) arena_alloc(a, ALLOC_SIZE, 8);
    arena_reset(a);
}

static void bench_pool_alloc(Pool *p) {
    void *ptrs[ALLOC_COUNT];
    for (int i = 0; i < ALLOC_COUNT; i++) ptrs[i] = pool_alloc(p);
    for (int i = 0; i < ALLOC_COUNT; i++) pool_free(p, ptrs[i]);
}

/* ── Branch Prediction ────────────────────────────────────────────────── */

static int sorted_data[N];
static int unsorted_data[N];

static int cmp_int(const void *a, const void *b) {
    return *(const int *)a - *(const int *)b;
}

static void bench_branch(const int *data, size_t n) {
    int sum = 0;
    for (size_t i = 0; i < n; i++)
        if (data[i] >= 128) sum += data[i];
    sink_i = sum;
}

/* ── Main ─────────────────────────────────────────────────────────────── */

int main(void) {
    srand(42);
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++) matrix[r][c] = (float)(r * COLS + c);

    for (int i = 0; i < N; i++) {
        a_arr[i] = (float)i * 0.5f;
        b_arr[i] = (float)i * 0.3f;
        sorted_data[i] = unsorted_data[i] = rand() % 256;
    }
    qsort(sorted_data, (size_t)N, sizeof(int), cmp_int);

    printf("=== Performance Benchmarks ===\n\n");

    /* 1. Cache access patterns */
    printf("--- Cache Access Patterns (1024x1024 matrix) ---\n");
    PERF_BENCH("row-major (cache-friendly)", ITERS, { bench_row_major(); });
    PERF_BENCH("col-major (cache-hostile)", ITERS, { bench_col_major(); });
    printf("\n");

    /* 2. SIMD vs scalar */
    printf("--- SIMD vs Scalar (%d floats) ---\n", N);
    PERF_BENCH("scalar add", ITERS, { scalar_add(dst, a_arr, b_arr, N); });
    PERF_BENCH("simd_add_f32", ITERS, { simd_add_f32(dst, a_arr, b_arr, N); });
    printf("\n");

    /* 3. Allocator throughput */
    printf("--- Allocator Throughput (%d x %d bytes) ---\n", ALLOC_COUNT, ALLOC_SIZE);
    Arena arena;
    arena_init(&arena, (size_t)ALLOC_COUNT * ALLOC_SIZE * 2);
    Pool pool;
    pool_init(&pool, ALLOC_SIZE, ALLOC_COUNT);

    PERF_BENCH("malloc/free", ITERS, { bench_malloc_free(); });
    PERF_BENCH("arena alloc+reset", ITERS, { bench_arena_alloc(&arena); });
    PERF_BENCH("pool alloc+free", ITERS, { bench_pool_alloc(&pool); });

    arena_destroy(&arena);
    pool_destroy(&pool);
    printf("\n");

    /* 4. Branch prediction */
    printf("--- Branch Prediction (%d elements, threshold=128) ---\n", N);
    PERF_BENCH("sorted (predictable)", ITERS, { bench_branch(sorted_data, (size_t)N); });
    PERF_BENCH("unsorted (unpredictable)", ITERS, { bench_branch(unsorted_data, (size_t)N); });
    printf("\n");

    printf("=== Done ===\n");
    return 0;
}
