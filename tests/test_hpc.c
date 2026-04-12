/**
 * @file test_hpc.c
 * @brief Tests for SIMD ops, parallel_for, and parallel_reduce.
 */
#include "hpc/simd_ops.h"
#include "hpc/parallel.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define N 1024
#define ASSERT(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); return 1; } \
} while(0)

static int test_simd_ops(void) {
    float a[N], b[N], dst[N];
    for (int i = 0; i < N; i++) { a[i] = (float)i; b[i] = (float)(N - i); }

    simd_add_f32(dst, a, b, N);
    for (int i = 0; i < N; i++)
        ASSERT(fabsf(dst[i] - (float)N) < 1e-3f, "simd_add_f32");

    simd_mul_f32(dst, a, b, N);
    for (int i = 0; i < N; i++)
        ASSERT(fabsf(dst[i] - (float)i * (float)(N - i)) < 1.0f, "simd_mul_f32");

    simd_scale_f32(dst, a, 2.0f, N);
    for (int i = 0; i < N; i++)
        ASSERT(fabsf(dst[i] - (float)i * 2.0f) < 1e-3f, "simd_scale_f32");

    float dot = simd_dot_f32(a, b, N);
    float expected_dot = 0.0f;
    for (int i = 0; i < N; i++) expected_dot += a[i] * b[i];
    /* Large sums lose float precision; use relative tolerance */
    ASSERT(fabsf(dot - expected_dot) / fabsf(expected_dot) < 1e-5f, "simd_dot_f32");

    float sum = simd_sum_f32(a, N);
    float expected_sum = (float)(N - 1) * (float)N / 2.0f;
    ASSERT(fabsf(sum - expected_sum) / fabsf(expected_sum) < 1e-5f, "simd_sum_f32");

    return 0;
}

/* parallel_for: each thread writes its index range */
typedef struct { int *buf; } PForCtx;

static void fill_chunk(size_t start, size_t end, void *ctx) {
    int *buf = ((PForCtx *)ctx)->buf;
    for (size_t i = start; i < end; i++) buf[i] = (int)i;
}

static int test_parallel_for(void) {
    int buf[N];
    PForCtx ctx = {buf};
    parallel_for(N, 4, fill_chunk, &ctx);
    for (int i = 0; i < N; i++)
        ASSERT(buf[i] == i, "parallel_for fill");
    return 0;
}

/* parallel_reduce: sum of 1..N */
static double sum_range(size_t start, size_t end, void *ctx) {
    (void)ctx;
    double s = 0;
    for (size_t i = start; i < end; i++) s += (double)(i + 1);
    return s;
}

static double add_d(double a, double b) { return a + b; }

static int test_parallel_reduce(void) {
    double result = parallel_reduce(N, 4, sum_range, NULL, add_d);
    double expected = (double)N * (double)(N + 1) / 2.0;
    ASSERT(fabs(result - expected) < 1e-6, "parallel_reduce sum");
    return 0;
}

int main(void) {
    int fail = 0;
    fail |= test_simd_ops();
    fail |= test_parallel_for();
    fail |= test_parallel_reduce();
    printf(fail ? "HPC tests: FAILED\n" : "HPC tests: ALL PASSED\n");
    return fail;
}
