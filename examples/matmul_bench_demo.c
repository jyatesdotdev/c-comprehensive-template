/**
 * @file matmul_bench_demo.c
 * @brief Benchmark our matmul implementations against an optimized BLAS.
 *
 * Build with -DUSE_OPENBLAS=ON (requires OpenBLAS on Linux or uses the
 * Accelerate framework on macOS) to add the cblas_sgemm column. Without it,
 * only the template's naive and cache-blocked implementations run.
 *
 * Expect BLAS to win by 10-100x at these sizes — that gap (vectorization,
 * micro-kernels, packing, threading) is why production ML uses a BLAS.
 */
#include "core/time.h"
#include "math/matx.h"
#include "math/rng.h"
#include "math/scalar.h"

#include <stdio.h>

#ifdef HAVE_CBLAS
#ifdef __APPLE__
#define ACCELERATE_NEW_LAPACK /* opt into the non-deprecated CBLAS interface */
#include <Accelerate/Accelerate.h>
#else
#include <cblas.h>
#endif
#endif

/** Max absolute difference vs a reference result — sanity check, not a benchmark. */
static float max_abs_diff(const MatX *a, const MatX *b) {
    float worst = 0.0f;
    for (size_t i = 0; i < a->rows * a->cols; i++) {
        float d = a->data[i] - b->data[i];
        if (d < 0.0f) d = -d;
        if (d > worst) worst = d;
    }
    return worst;
}

static int bench_size(size_t n, Rng *rng) {
    MatX a, b, ref, out;
    if (matx_init(&a, n, n) || matx_init(&b, n, n) || matx_init(&ref, n, n) ||
        matx_init(&out, n, n)) {
        fprintf(stderr, "allocation failed at n=%zu\n", n);
        return 1;
    }
    matx_random_uniform(&a, rng, -1.0f, 1.0f);
    matx_random_uniform(&b, rng, -1.0f, 1.0f);

    double gflop = 2.0 * (double)n * (double)n * (double)n / 1.0e9;

    double t0 = time_now_ms();
    matx_mul(&a, &b, &ref);
    double t_naive = time_now_ms() - t0;

    t0 = time_now_ms();
    matx_mul_blocked(&a, &b, &out, 64);
    double t_blocked = time_now_ms() - t0;
    float  blocked_err = max_abs_diff(&ref, &out);

    printf("%4zu | naive %8.1f ms (%5.2f GFLOP/s) | blocked %8.1f ms (%5.2f GFLOP/s)", n, t_naive,
           gflop / (t_naive / 1000.0), t_blocked, gflop / (t_blocked / 1000.0));

#ifdef HAVE_CBLAS
    t0 = time_now_ms();
    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, (int)n, (int)n, (int)n, 1.0f, a.data,
                (int)n, b.data, (int)n, 0.0f, out.data, (int)n);
    double t_blas = time_now_ms() - t0;
    float  blas_err = max_abs_diff(&ref, &out);
    printf(" | BLAS %7.1f ms (%6.2f GFLOP/s)", t_blas, gflop / (t_blas / 1000.0));
    if (blas_err > 1e-2f) printf("  [MISMATCH %g]", (double)blas_err);
#endif
    if (blocked_err > 1e-2f) printf("  [BLOCKED MISMATCH %g]", (double)blocked_err);
    printf("\n");

    matx_destroy(&a);
    matx_destroy(&b);
    matx_destroy(&ref);
    matx_destroy(&out);
    return 0;
}

int main(void) {
#ifdef HAVE_CBLAS
    printf("Matmul benchmark: template implementations vs BLAS\n");
#else
    printf("Matmul benchmark (rebuild with -DUSE_OPENBLAS=ON for the BLAS column)\n");
#endif

    Rng rng;
    rng_seed(&rng, 99, 0);
    const size_t sizes[] = {128, 256, 512};
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        if (bench_size(sizes[i], &rng) != 0) return 1;
    }
    return 0;
}
