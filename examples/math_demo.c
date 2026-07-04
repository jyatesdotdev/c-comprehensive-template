/**
 * @file math_demo.c
 * @brief Tour of the math module: transforms, quaternions, MatX, RNG, stats.
 */
#include "core/time.h"
#include "math/mat.h"
#include "math/matx.h"
#include "math/quat.h"
#include "math/rng.h"
#include "math/scalar.h"
#include "math/stats.h"
#include "math/vec.h"

#include <stdio.h>
#include <stdlib.h>

/** Model-view-projection pipeline: the core of every 3D renderer. */
static void demo_transforms(void) {
    Mat4 model = mat4_mul(mat4_translate((Vec3){0.0f, 0.0f, -5.0f}),
                          mat4_rotate_y(scalar_deg_to_rad(45.0f)));
    Mat4 view =
        mat4_look_at((Vec3){0.0f, 2.0f, 3.0f}, (Vec3){0.0f, 0.0f, -5.0f}, (Vec3){0.0f, 1.0f, 0.0f});
    Mat4 proj = mat4_perspective(scalar_deg_to_rad(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);
    Mat4 mvp = mat4_mul(proj, mat4_mul(view, model));

    Vec4 clip = mat4_mul_vec4(mvp, (Vec4){1.0f, 0.0f, 0.0f, 1.0f});
    printf("MVP: local (1,0,0) -> NDC (%.3f, %.3f, %.3f)\n", (double)(clip.x / clip.w),
           (double)(clip.y / clip.w), (double)(clip.z / clip.w));
}

/** Quaternion rotation and interpolation. */
static void demo_quaternions(void) {
    Quat start = quat_identity();
    Quat end = quat_from_axis_angle((Vec3){0.0f, 1.0f, 0.0f}, scalar_deg_to_rad(90.0f));

    printf("Slerp of +X around Y axis:\n");
    for (int i = 0; i <= 4; i++) {
        float t = (float)i / 4.0f;
        Vec3  v = quat_rotate_vec3(quat_slerp(start, end, t), (Vec3){1.0f, 0.0f, 0.0f});
        printf("  t=%.2f -> (%.3f, %.3f, %.3f)\n", (double)t, (double)v.x, (double)v.y,
               (double)v.z);
    }
}

/** Naive vs cache-blocked matmul on ML-sized matrices. */
static int demo_matx(void) {
    enum { N = 256 };
    MatX a, b, out;
    Rng  rng;
    rng_seed(&rng, 7, 1);

    if (matx_init(&a, N, N) || matx_init(&b, N, N) || matx_init(&out, N, N)) {
        fprintf(stderr, "matx_init failed\n");
        return 1;
    }
    matx_random_uniform(&a, &rng, -1.0f, 1.0f);
    matx_random_uniform(&b, &rng, -1.0f, 1.0f);

    double t0 = time_now_ms();
    matx_mul(&a, &b, &out);
    double t_naive = time_now_ms() - t0;

    t0 = time_now_ms();
    matx_mul_blocked(&a, &b, &out, 32);
    double t_blocked = time_now_ms() - t0;

    printf("%dx%d matmul: naive %.1f ms, blocked %.1f ms\n", N, N, t_naive, t_blocked);

    matx_destroy(&a);
    matx_destroy(&b);
    matx_destroy(&out);
    return 0;
}

/** Reproducible random numbers and summary statistics. */
static int demo_rng_stats(void) {
    enum { N = 10000 };
    float *samples = malloc((size_t)N * sizeof(float));
    if (!samples) return 1;

    Rng rng;
    rng_seed(&rng, 42, 0);
    for (int i = 0; i < N; i++) samples[i] = rng_normal(&rng);

    float mean = 0.0f, var = 0.0f, lo = 0.0f, hi = 0.0f;
    stats_mean(samples, N, &mean);
    stats_variance(samples, N, &var);
    stats_min_max(samples, N, &lo, &hi);
    printf("%d normal samples (seed 42): mean=%.4f var=%.4f min=%.2f max=%.2f\n", N, (double)mean,
           (double)var, (double)lo, (double)hi);

    free(samples);
    return 0;
}

int main(void) {
    demo_transforms();
    demo_quaternions();
    if (demo_matx() != 0) return 1;
    if (demo_rng_stats() != 0) return 1;
    return 0;
}
