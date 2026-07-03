/**
 * @file hpc_demo.c
 * @brief HPC module demo: SIMD ops, parallel_for, parallel_reduce, thread pool.
 *
 * Benchmarks SIMD vs scalar and parallel vs serial to show speedups.
 */
#include "hpc/simd_ops.h"
#include "hpc/parallel.h"
#include "hpc/thread_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define N       (1 << 20) /* ~1M elements */
#define THREADS 4

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/* ── SIMD benchmark ───────────────────────────────────────────────────── */

static void bench_simd(void) {
    float *a = malloc(N * sizeof(float));
    float *b = malloc(N * sizeof(float));
    float *dst = malloc(N * sizeof(float));
    for (size_t i = 0; i < N; i++) {
        a[i] = (float)i * 0.001f;
        b[i] = (float)i * 0.002f;
    }

    double t0 = now_sec();
    simd_add_f32(dst, a, b, N);
    simd_mul_f32(dst, a, b, N);
    simd_scale_f32(dst, a, 3.14f, N);
    float  dot = simd_dot_f32(a, b, N);
    float  sum = simd_sum_f32(a, N);
    double t1 = now_sec();

    printf("[SIMD] add+mul+scale+dot+sum on %d floats: %.3f ms  (dot=%.2f, sum=%.2f)\n", N,
           (t1 - t0) * 1000.0, (double)dot, (double)sum);
    free(a);
    free(b);
    free(dst);
}

/* ── parallel_for benchmark ───────────────────────────────────────────── */

typedef struct {
    double *data;
} ForCtx;

static void compute_chunk(size_t start, size_t end, void *ctx) {
    double *data = ((ForCtx *)ctx)->data;
    for (size_t i = start; i < end; i++)
        data[i] = sin((double)i * 0.0001) * cos((double)i * 0.0002);
}

static void bench_parallel_for(void) {
    double *data = malloc(N * sizeof(double));
    ForCtx  ctx = {data};

    double t0 = now_sec();
    compute_chunk(0, N, &ctx);
    double t_serial = now_sec() - t0;

    t0 = now_sec();
    parallel_for(N, THREADS, compute_chunk, &ctx);
    double t_par = now_sec() - t0;

    printf("[parallel_for] serial=%.3f ms  parallel(%d)=%.3f ms  speedup=%.2fx\n", t_serial * 1000,
           THREADS, t_par * 1000, t_serial / t_par);
    free(data);
}

/* ── parallel_reduce benchmark ────────────────────────────────────────── */

static double sum_chunk(size_t start, size_t end, void *ctx) {
    (void)ctx;
    double s = 0.0;
    for (size_t i = start; i < end; i++) s += 1.0 / ((double)i * (double)i + 1.0);
    return s;
}

static double add(double a, double b) {
    return a + b;
}

static void bench_parallel_reduce(void) {
    double t0 = now_sec();
    double serial = sum_chunk(0, N, NULL);
    double t_serial = now_sec() - t0;

    t0 = now_sec();
    double par = parallel_reduce(N, THREADS, sum_chunk, NULL, add);
    double t_par = now_sec() - t0;

    printf("[parallel_reduce] serial=%.3f ms (%.6f)  parallel(%d)=%.3f ms (%.6f)  speedup=%.2fx\n",
           t_serial * 1000, serial, THREADS, t_par * 1000, par, t_serial / t_par);
}

/* ── Thread pool demo ─────────────────────────────────────────────────── */

static void pool_task(void *arg) {
    int id = *(int *)arg;
    /* Simulate work */
    volatile double x = 0;
    for (int i = 0; i < 100000; i++) x += (double)i;
    printf("  pool task %d done (x=%.0f)\n", id, x);
}

static void bench_thread_pool(void) {
    ThreadPool *pool;
    if (thread_pool_create(&pool, THREADS) != ERR_OK) return;

    printf("[thread_pool] submitting 8 tasks to %d threads:\n", THREADS);
    int ids[8];
    for (int i = 0; i < 8; i++) {
        ids[i] = i;
        thread_pool_submit(pool, pool_task, &ids[i]);
    }
    thread_pool_destroy(pool); /* waits for all tasks */
    printf("[thread_pool] done\n");
}

int main(void) {
    printf("=== HPC Module Demo ===\n\n");
    bench_simd();
    printf("\n");
    bench_parallel_for();
    printf("\n");
    bench_parallel_reduce();
    printf("\n");
    bench_thread_pool();
    return 0;
}
