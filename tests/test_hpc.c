/**
 * @file test_hpc.c
 * @brief Tests for SIMD ops, parallel_for, and parallel_reduce.
 */
#include "hpc/parallel.h"
#include "hpc/simd_ops.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(cond)                                                                    \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

#define N 1024

static void test_simd_ops(void) {
    static float a[N], b[N], dst[N];
    for (int i = 0; i < N; i++) {
        a[i] = (float)i;
        b[i] = (float)(N - i);
    }

    simd_add_f32(dst, a, b, N);
    for (int i = 0; i < N; i++) CHECK(fabsf(dst[i] - (float)N) < 1e-3f);

    simd_mul_f32(dst, a, b, N);
    for (int i = 0; i < N; i++) CHECK(fabsf(dst[i] - (float)i * (float)(N - i)) < 1.0f);

    simd_scale_f32(dst, a, 2.0f, N);
    for (int i = 0; i < N; i++) CHECK(fabsf(dst[i] - (float)i * 2.0f) < 1e-3f);

    float dot = simd_dot_f32(a, b, N);
    float expected_dot = 0.0f;
    for (int i = 0; i < N; i++) expected_dot += a[i] * b[i];
    /* Large sums lose float precision; use relative tolerance */
    CHECK(fabsf(dot - expected_dot) / fabsf(expected_dot) < 1e-5f);

    float sum = simd_sum_f32(a, N);
    float expected_sum = (float)(N - 1) * (float)N / 2.0f;
    CHECK(fabsf(sum - expected_sum) / fabsf(expected_sum) < 1e-5f);

    /* Odd length exercises the scalar tail after the SIMD lanes */
    simd_add_f32(dst, a, b, N - 3);
    for (int i = 0; i < N - 3; i++) CHECK(fabsf(dst[i] - (float)N) < 1e-3f);
}

/* parallel_for: each thread writes its index range */
typedef struct {
    int *buf;
} PForCtx;

static void fill_chunk(size_t start, size_t end, void *ctx) {
    int *buf = ((PForCtx *)ctx)->buf;
    for (size_t i = start; i < end; i++) buf[i] = (int)i;
}

static void test_parallel_for(void) {
    static int buf[N];
    PForCtx    ctx = {buf};
    parallel_for(N, 4, fill_chunk, &ctx);
    for (int i = 0; i < N; i++) CHECK(buf[i] == i);

    /* More threads than work items still covers the full range */
    parallel_for(3, 16, fill_chunk, &ctx);
    for (int i = 0; i < 3; i++) CHECK(buf[i] == i);
}

/* parallel_reduce: sum of 1..N */
static double sum_range(size_t start, size_t end, void *ctx) {
    (void)ctx;
    double s = 0;
    for (size_t i = start; i < end; i++) s += (double)(i + 1);
    return s;
}

static double add_d(double a, double b) {
    return a + b;
}

static void test_parallel_reduce(void) {
    double result = parallel_reduce(N, 4, sum_range, NULL, add_d);
    double expected = (double)N * (double)(N + 1) / 2.0;
    CHECK(fabs(result - expected) < 1e-6);

    /* Single-threaded path */
    CHECK(fabs(parallel_reduce(N, 1, sum_range, NULL, add_d) - expected) < 1e-6);
    CHECK(parallel_reduce(0, 4, sum_range, NULL, add_d) == 0.0);
}

int main(void) {
    test_simd_ops();
    test_parallel_for();
    test_parallel_reduce();
    printf("All hpc tests passed.\n");
    return 0;
}
