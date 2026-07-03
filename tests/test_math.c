/**
 * @file test_math.c
 * @brief Tests for the math module: scalar, vec, mat, quat, matx, rng, stats.
 *
 * Uses CHECK instead of assert() so tests still run under NDEBUG (Release).
 */
#include "math/mat.h"
#include "math/matx.h"
#include "math/quat.h"
#include "math/rng.h"
#include "math/scalar.h"
#include "math/stats.h"
#include "math/vec.h"
#include "memory/arena.h"

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

#define EPS 1e-5f

static void test_scalar(void) {
    CHECK(scalar_clamp(5.0f, 0.0f, 1.0f) == 1.0f);
    CHECK(scalar_clamp(-5.0f, 0.0f, 1.0f) == 0.0f);
    CHECK(scalar_clamp(0.5f, 0.0f, 1.0f) == 0.5f);
    CHECK(scalar_approx_eq(scalar_lerp(0.0f, 10.0f, 0.5f), 5.0f, EPS));
    CHECK(scalar_approx_eq(scalar_deg_to_rad(180.0f), SCALAR_PI, EPS));
    CHECK(scalar_approx_eq(scalar_rad_to_deg(SCALAR_PI), 180.0f, 1e-3f));
    CHECK(scalar_approx_eq(1.0f, 1.0f + 1e-6f, EPS));
    CHECK(!scalar_approx_eq(1.0f, 1.1f, EPS));
}

static void test_vec(void) {
    Vec2 a2 = {1.0f, 2.0f}, b2 = {3.0f, 4.0f};
    CHECK(scalar_approx_eq(vec2_add(a2, b2).x, 4.0f, EPS));
    CHECK(scalar_approx_eq(vec2_sub(b2, a2).y, 2.0f, EPS));
    CHECK(scalar_approx_eq(vec2_dot(a2, b2), 11.0f, EPS));
    CHECK(scalar_approx_eq(vec2_length((Vec2){3.0f, 4.0f}), 5.0f, EPS));
    CHECK(scalar_approx_eq(vec2_length(vec2_normalize(b2)), 1.0f, EPS));
    CHECK(scalar_approx_eq(vec2_lerp(a2, b2, 0.5f).x, 2.0f, EPS));
    CHECK(scalar_approx_eq(vec2_scale(a2, 2.0f).y, 4.0f, EPS));

    Vec3 x = {1.0f, 0.0f, 0.0f}, y = {0.0f, 1.0f, 0.0f}, z = {0.0f, 0.0f, 1.0f};
    CHECK(vec3_approx_eq(vec3_cross(x, y), z, EPS)); /* right-handed */
    CHECK(scalar_approx_eq(vec3_dot(x, y), 0.0f, EPS));
    CHECK(scalar_approx_eq(vec3_length((Vec3){2.0f, 3.0f, 6.0f}), 7.0f, EPS));
    CHECK(scalar_approx_eq(vec3_distance(x, y), sqrtf(2.0f), EPS));
    CHECK(scalar_approx_eq(vec3_length(vec3_normalize((Vec3){5.0f, 5.0f, 5.0f})), 1.0f, EPS));
    Vec3 zero = {0.0f, 0.0f, 0.0f};
    CHECK(vec3_approx_eq(vec3_normalize(zero), zero, EPS)); /* zero stays zero */
    CHECK(vec3_approx_eq(vec3_lerp(x, y, 0.0f), x, EPS));
    CHECK(vec3_approx_eq(vec3_lerp(x, y, 1.0f), y, EPS));
    CHECK(vec3_approx_eq(vec3_add(vec3_scale(x, 2.0f), vec3_sub(y, y)), (Vec3){2.0f, 0.0f, 0.0f},
                         EPS));

    Vec4 a4 = {1.0f, 2.0f, 3.0f, 4.0f}, b4 = {4.0f, 3.0f, 2.0f, 1.0f};
    CHECK(scalar_approx_eq(vec4_dot(a4, b4), 20.0f, EPS));
    CHECK(scalar_approx_eq(vec4_add(a4, b4).w, 5.0f, EPS));
    CHECK(scalar_approx_eq(vec4_sub(a4, b4).x, -3.0f, EPS));
    CHECK(scalar_approx_eq(vec4_scale(a4, 0.5f).z, 1.5f, EPS));
    CHECK(scalar_approx_eq(vec4_length((Vec4){2.0f, 0.0f, 0.0f, 0.0f}), 2.0f, EPS));
}

static void test_mat4(void) {
    /* identity is multiplicative neutral */
    Mat4 t = mat4_translate((Vec3){1.0f, 2.0f, 3.0f});
    Mat4 ti = mat4_mul(t, mat4_identity());
    for (int i = 0; i < 16; i++) CHECK(scalar_approx_eq(ti.m[i], t.m[i], EPS));

    /* translate moves a point */
    Vec3 p = mat4_mul_point(t, (Vec3){0.0f, 0.0f, 0.0f});
    CHECK(vec3_approx_eq(p, (Vec3){1.0f, 2.0f, 3.0f}, EPS));

    /* scale */
    Vec3 s = mat4_mul_point(mat4_scale((Vec3){2.0f, 3.0f, 4.0f}), (Vec3){1.0f, 1.0f, 1.0f});
    CHECK(vec3_approx_eq(s, (Vec3){2.0f, 3.0f, 4.0f}, EPS));

    /* rotating +X by 90° around Z gives +Y; same for the axis-angle builder */
    Vec3 rx = mat4_mul_point(mat4_rotate_z(scalar_deg_to_rad(90.0f)), (Vec3){1.0f, 0.0f, 0.0f});
    CHECK(vec3_approx_eq(rx, (Vec3){0.0f, 1.0f, 0.0f}, EPS));
    Vec3 ra = mat4_mul_point(mat4_rotate_axis((Vec3){0.0f, 0.0f, 1.0f}, scalar_deg_to_rad(90.0f)),
                             (Vec3){1.0f, 0.0f, 0.0f});
    CHECK(vec3_approx_eq(ra, (Vec3){0.0f, 1.0f, 0.0f}, EPS));
    Vec3 ry = mat4_mul_point(mat4_rotate_y(scalar_deg_to_rad(90.0f)), (Vec3){0.0f, 0.0f, 1.0f});
    CHECK(vec3_approx_eq(ry, (Vec3){1.0f, 0.0f, 0.0f}, EPS));
    Vec3 rxx = mat4_mul_point(mat4_rotate_x(scalar_deg_to_rad(90.0f)), (Vec3){0.0f, 1.0f, 0.0f});
    CHECK(vec3_approx_eq(rxx, (Vec3){0.0f, 0.0f, 1.0f}, EPS));

    /* transpose of transpose is the original */
    Mat4 tt = mat4_transpose(mat4_transpose(t));
    for (int i = 0; i < 16; i++) CHECK(scalar_approx_eq(tt.m[i], t.m[i], EPS));

    /* inverse: M * M^-1 == I for an invertible transform */
    Mat4 m = mat4_mul(t, mat4_rotate_y(0.7f));
    Mat4 inv;
    CHECK(mat4_inverse(m, &inv));
    Mat4 id = mat4_mul(m, inv);
    Mat4 expect = mat4_identity();
    for (int i = 0; i < 16; i++) CHECK(scalar_approx_eq(id.m[i], expect.m[i], 1e-4f));

    /* singular matrix (zero scale) is rejected */
    Mat4 sing = mat4_scale((Vec3){0.0f, 1.0f, 1.0f});
    CHECK(!mat4_inverse(sing, NULL));

    /* look_at from +Z toward origin maps the origin to -eye distance on Z */
    Mat4 view =
        mat4_look_at((Vec3){0.0f, 0.0f, 5.0f}, (Vec3){0.0f, 0.0f, 0.0f}, (Vec3){0.0f, 1.0f, 0.0f});
    Vec3 origin_view = mat4_mul_point(view, (Vec3){0.0f, 0.0f, 0.0f});
    CHECK(vec3_approx_eq(origin_view, (Vec3){0.0f, 0.0f, -5.0f}, EPS));

    /* perspective: a point on the near plane center maps to clip z = -w */
    Mat4 proj = mat4_perspective(scalar_deg_to_rad(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);
    Vec4 clip = mat4_mul_vec4(proj, (Vec4){0.0f, 0.0f, -0.1f, 1.0f});
    CHECK(scalar_approx_eq(clip.z / clip.w, -1.0f, 1e-3f));

    /* ortho maps its box corners to [-1, 1] */
    Mat4 ortho = mat4_ortho(-2.0f, 2.0f, -1.0f, 1.0f, 0.0f, 10.0f);
    Vec3 corner = mat4_mul_point(ortho, (Vec3){2.0f, 1.0f, -10.0f});
    CHECK(vec3_approx_eq(corner, (Vec3){1.0f, 1.0f, 1.0f}, EPS));
}

static void test_quat(void) {
    /* identity rotates nothing */
    Vec3 v = {1.0f, 2.0f, 3.0f};
    CHECK(vec3_approx_eq(quat_rotate_vec3(quat_identity(), v), v, EPS));

    /* 90° around Z: +X → +Y, and matches the equivalent matrix */
    Quat qz = quat_from_axis_angle((Vec3){0.0f, 0.0f, 1.0f}, scalar_deg_to_rad(90.0f));
    Vec3 rq = quat_rotate_vec3(qz, (Vec3){1.0f, 0.0f, 0.0f});
    CHECK(vec3_approx_eq(rq, (Vec3){0.0f, 1.0f, 0.0f}, EPS));
    Vec3 rm = mat4_mul_point(quat_to_mat4(qz), (Vec3){1.0f, 0.0f, 0.0f});
    CHECK(vec3_approx_eq(rm, rq, EPS));

    /* composing two 45° rotations equals one 90° rotation */
    Quat q45 = quat_from_axis_angle((Vec3){0.0f, 0.0f, 1.0f}, scalar_deg_to_rad(45.0f));
    Vec3 rc = quat_rotate_vec3(quat_mul(q45, q45), (Vec3){1.0f, 0.0f, 0.0f});
    CHECK(vec3_approx_eq(rc, (Vec3){0.0f, 1.0f, 0.0f}, EPS));

    /* conjugate undoes the rotation */
    Vec3 back = quat_rotate_vec3(quat_conjugate(qz), rq);
    CHECK(vec3_approx_eq(back, (Vec3){1.0f, 0.0f, 0.0f}, EPS));

    /* slerp endpoints and midpoint */
    Quat a = quat_identity();
    CHECK(vec3_approx_eq(quat_rotate_vec3(quat_slerp(a, qz, 0.0f), (Vec3){1.0f, 0.0f, 0.0f}),
                         (Vec3){1.0f, 0.0f, 0.0f}, EPS));
    CHECK(vec3_approx_eq(quat_rotate_vec3(quat_slerp(a, qz, 1.0f), (Vec3){1.0f, 0.0f, 0.0f}),
                         (Vec3){0.0f, 1.0f, 0.0f}, EPS));
    Vec3  mid = quat_rotate_vec3(quat_slerp(a, qz, 0.5f), (Vec3){1.0f, 0.0f, 0.0f});
    float inv_sqrt2 = 1.0f / sqrtf(2.0f);
    CHECK(vec3_approx_eq(mid, (Vec3){inv_sqrt2, inv_sqrt2, 0.0f}, 1e-4f));

    /* normalize recovers unit length; zero falls back to identity */
    Quat big = {0.0f, 0.0f, 3.0f, 4.0f};
    Quat n = quat_normalize(big);
    CHECK(scalar_approx_eq(n.z * n.z + n.w * n.w, 1.0f, EPS));
    Quat zeroq = {0.0f, 0.0f, 0.0f, 0.0f};
    Quat nz = quat_normalize(zeroq);
    CHECK(scalar_approx_eq(nz.w, 1.0f, EPS));
}

static void test_matx(void) {
    MatX a, b, out, t;

    /* invalid args */
    CHECK(matx_init(NULL, 2, 2) == ERR_INVALID_ARG);
    CHECK(matx_init(&a, 0, 2) == ERR_INVALID_ARG);
    CHECK(matx_init(&a, (size_t)1 << 40, (size_t)1 << 40) == ERR_OVERFLOW);

    /* known 2x3 * 3x2 product */
    CHECK(matx_init(&a, 2, 3) == ERR_OK);
    CHECK(matx_init(&b, 3, 2) == ERR_OK);
    CHECK(matx_init(&out, 2, 2) == ERR_OK);
    float av[] = {1, 2, 3, 4, 5, 6};
    float bv[] = {7, 8, 9, 10, 11, 12};
    for (size_t i = 0; i < 6; i++) {
        a.data[i] = av[i];
        b.data[i] = bv[i];
    }
    CHECK(matx_mul(&a, &b, &out) == ERR_OK);
    CHECK(scalar_approx_eq(matx_get(&out, 0, 0), 58.0f, EPS));
    CHECK(scalar_approx_eq(matx_get(&out, 0, 1), 64.0f, EPS));
    CHECK(scalar_approx_eq(matx_get(&out, 1, 0), 139.0f, EPS));
    CHECK(scalar_approx_eq(matx_get(&out, 1, 1), 154.0f, EPS));

    /* dimension mismatch and aliasing rejected */
    CHECK(matx_mul(&a, &a, &out) == ERR_INVALID_ARG);
    CHECK(matx_mul(&a, &b, &a) == ERR_INVALID_ARG);

    /* transpose */
    CHECK(matx_init(&t, 3, 2) == ERR_OK);
    CHECK(matx_transpose(&a, &t) == ERR_OK);
    CHECK(scalar_approx_eq(matx_get(&t, 2, 1), matx_get(&a, 1, 2), EPS));
    CHECK(matx_transpose(&a, &out) == ERR_INVALID_ARG); /* wrong shape */

    /* add + fill */
    CHECK(matx_fill(&a, 1.5f) == ERR_OK);
    CHECK(matx_add(&a, &a, &a) == ERR_OK); /* aliasing allowed for add */
    CHECK(scalar_approx_eq(matx_get(&a, 1, 2), 3.0f, EPS));
    CHECK(matx_add(&a, &b, &out) == ERR_INVALID_ARG); /* shape mismatch */

    matx_destroy(&a);
    matx_destroy(&b);
    matx_destroy(&out);
    matx_destroy(&t);
    matx_destroy(NULL); /* safe */

    /* blocked matmul agrees with naive on random matrices */
    Rng rng;
    rng_seed(&rng, 42, 1);
    MatX ra, rb, naive, blocked;
    CHECK(matx_init(&ra, 33, 47) == ERR_OK); /* odd sizes exercise tile edges */
    CHECK(matx_init(&rb, 47, 21) == ERR_OK);
    CHECK(matx_init(&naive, 33, 21) == ERR_OK);
    CHECK(matx_init(&blocked, 33, 21) == ERR_OK);
    CHECK(matx_random_uniform(&ra, &rng, -1.0f, 1.0f) == ERR_OK);
    CHECK(matx_random_uniform(&rb, &rng, -1.0f, 1.0f) == ERR_OK);
    CHECK(matx_mul(&ra, &rb, &naive) == ERR_OK);
    CHECK(matx_mul_blocked(&ra, &rb, &blocked, 8) == ERR_OK);
    for (size_t i = 0; i < naive.rows * naive.cols; i++)
        CHECK(scalar_approx_eq(naive.data[i], blocked.data[i], 1e-3f));
    matx_destroy(&ra);
    matx_destroy(&rb);
    matx_destroy(&naive);
    matx_destroy(&blocked);

    /* arena-backed matrices: no matx_destroy, freed with the arena */
    Arena arena;
    CHECK(arena_init(&arena, 64 * 1024) == ERR_OK);
    MatX am;
    CHECK(matx_init_arena(&am, &arena, 8, 8) == ERR_OK);
    CHECK(am.owns_data == 0);
    CHECK(matx_fill(&am, 2.0f) == ERR_OK);
    CHECK(scalar_approx_eq(matx_get(&am, 7, 7), 2.0f, EPS));
    MatX toobig;
    CHECK(matx_init_arena(&toobig, &arena, 1024, 1024) == ERR_NOMEM); /* arena full */
    arena_destroy(&arena);
}

static void test_rng(void) {
    Rng a, b;

    /* determinism: same seed → same sequence; different stream → different */
    rng_seed(&a, 123, 7);
    rng_seed(&b, 123, 7);
    for (int i = 0; i < 10; i++) CHECK(rng_next_u32(&a) == rng_next_u32(&b));
    rng_seed(&b, 123, 8);
    int same = 1;
    for (int i = 0; i < 10; i++)
        if (rng_next_u32(&a) != rng_next_u32(&b)) same = 0;
    CHECK(!same);

    /* bounds */
    rng_seed(&a, 99, 1);
    for (int i = 0; i < 1000; i++) {
        CHECK(rng_range_u32(&a, 10) < 10);
        float f = rng_next_float(&a);
        CHECK(f >= 0.0f && f < 1.0f);
        float rf = rng_range_float(&a, -2.0f, 3.0f);
        CHECK(rf >= -2.0f && rf < 3.0f);
    }
    CHECK(rng_range_u32(&a, 0) == 0);
    CHECK(rng_range_u32(&a, 1) == 0);

    /* normal samples: mean ~0, variance ~1 over many draws */
    enum { N = 20000 };
    float *samples = malloc(N * sizeof(float));
    CHECK(samples != NULL);
    rng_seed(&a, 2024, 3);
    for (int i = 0; i < N; i++) samples[i] = rng_normal(&a);
    float mean = 0.0f, var = 0.0f;
    CHECK(stats_mean(samples, N, &mean) == ERR_OK);
    CHECK(stats_variance(samples, N, &var) == ERR_OK);
    CHECK(fabsf(mean) < 0.05f);
    CHECK(fabsf(var - 1.0f) < 0.05f);
    free(samples);
}

static void test_stats(void) {
    float data[] = {2.0f, 4.0f, 4.0f, 4.0f, 5.0f, 5.0f, 7.0f, 9.0f};
    float mean = 0.0f, var = 0.0f, lo = 0.0f, hi = 0.0f;

    CHECK(stats_mean(data, 8, &mean) == ERR_OK);
    CHECK(scalar_approx_eq(mean, 5.0f, EPS));
    CHECK(stats_variance(data, 8, &var) == ERR_OK);
    CHECK(scalar_approx_eq(var, 32.0f / 7.0f, 1e-4f)); /* sample variance */
    CHECK(stats_min_max(data, 8, &lo, &hi) == ERR_OK);
    CHECK(lo == 2.0f && hi == 9.0f);
    CHECK(stats_min_max(data, 8, NULL, &hi) == ERR_OK); /* min optional */

    CHECK(stats_mean(NULL, 8, &mean) == ERR_INVALID_ARG);
    CHECK(stats_mean(data, 0, &mean) == ERR_INVALID_ARG);
    CHECK(stats_mean(data, 8, NULL) == ERR_INVALID_ARG);
    CHECK(stats_variance(data, 1, &var) == ERR_INVALID_ARG);
    CHECK(stats_min_max(data, 8, NULL, NULL) == ERR_INVALID_ARG);
}

int main(void) {
    test_scalar();
    test_vec();
    test_mat4();
    test_quat();
    test_matx();
    test_rng();
    test_stats();
    printf("All math tests passed.\n");
    return 0;
}
