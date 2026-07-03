/**
 * @file simd_ops.c
 * @brief SIMD-accelerated float vector operations (NEON / SSE4.2 / scalar fallback).
 */
#include "hpc/simd_ops.h"

#if defined(HAS_SSE42) || defined(HAS_AVX2)
#include <immintrin.h>
#elif defined(HAS_NEON)
#include <arm_neon.h>
#endif

/* ── Element-wise add ─────────────────────────────────────────────────── */

void simd_add_f32(float *dst, const float *a, const float *b, size_t n) {
    size_t i = 0;
#if defined(HAS_NEON)
    for (; i + 4 <= n; i += 4) vst1q_f32(dst + i, vaddq_f32(vld1q_f32(a + i), vld1q_f32(b + i)));
#elif defined(HAS_SSE42)
    for (; i + 4 <= n; i += 4)
        _mm_storeu_ps(dst + i, _mm_add_ps(_mm_loadu_ps(a + i), _mm_loadu_ps(b + i)));
#endif
    for (; i < n; i++) dst[i] = a[i] + b[i];
}

/* ── Element-wise multiply ────────────────────────────────────────────── */

void simd_mul_f32(float *dst, const float *a, const float *b, size_t n) {
    size_t i = 0;
#if defined(HAS_NEON)
    for (; i + 4 <= n; i += 4) vst1q_f32(dst + i, vmulq_f32(vld1q_f32(a + i), vld1q_f32(b + i)));
#elif defined(HAS_SSE42)
    for (; i + 4 <= n; i += 4)
        _mm_storeu_ps(dst + i, _mm_mul_ps(_mm_loadu_ps(a + i), _mm_loadu_ps(b + i)));
#endif
    for (; i < n; i++) dst[i] = a[i] * b[i];
}

/* ── Scale ────────────────────────────────────────────────────────────── */

void simd_scale_f32(float *dst, const float *a, float scalar, size_t n) {
    size_t i = 0;
#if defined(HAS_NEON)
    float32x4_t vs = vdupq_n_f32(scalar);
    for (; i + 4 <= n; i += 4) vst1q_f32(dst + i, vmulq_f32(vld1q_f32(a + i), vs));
#elif defined(HAS_SSE42)
    __m128 vs = _mm_set1_ps(scalar);
    for (; i + 4 <= n; i += 4) _mm_storeu_ps(dst + i, _mm_mul_ps(_mm_loadu_ps(a + i), vs));
#endif
    for (; i < n; i++) dst[i] = a[i] * scalar;
}

/* ── Dot product ──────────────────────────────────────────────────────── */

float simd_dot_f32(const float *a, const float *b, size_t n) {
    float  sum = 0.0f;
    size_t i = 0;
#if defined(HAS_NEON)
    float32x4_t vsum = vdupq_n_f32(0.0f);
    for (; i + 4 <= n; i += 4) vsum = vmlaq_f32(vsum, vld1q_f32(a + i), vld1q_f32(b + i));
    sum = vaddvq_f32(vsum);
#elif defined(HAS_SSE42)
    __m128 vsum = _mm_setzero_ps();
    for (; i + 4 <= n; i += 4)
        vsum = _mm_add_ps(vsum, _mm_mul_ps(_mm_loadu_ps(a + i), _mm_loadu_ps(b + i)));
    float tmp[4];
    _mm_storeu_ps(tmp, vsum);
    sum = tmp[0] + tmp[1] + tmp[2] + tmp[3];
#endif
    for (; i < n; i++) sum += a[i] * b[i];
    return sum;
}

/* ── Sum reduction ────────────────────────────────────────────────────── */

float simd_sum_f32(const float *a, size_t n) {
    float  sum = 0.0f;
    size_t i = 0;
#if defined(HAS_NEON)
    float32x4_t vsum = vdupq_n_f32(0.0f);
    for (; i + 4 <= n; i += 4) vsum = vaddq_f32(vsum, vld1q_f32(a + i));
    sum = vaddvq_f32(vsum);
#elif defined(HAS_SSE42)
    __m128 vsum = _mm_setzero_ps();
    for (; i + 4 <= n; i += 4) vsum = _mm_add_ps(vsum, _mm_loadu_ps(a + i));
    float tmp[4];
    _mm_storeu_ps(tmp, vsum);
    sum = tmp[0] + tmp[1] + tmp[2] + tmp[3];
#endif
    for (; i < n; i++) sum += a[i];
    return sum;
}
