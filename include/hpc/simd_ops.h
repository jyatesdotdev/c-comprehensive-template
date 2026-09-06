/**
 * @file simd_ops.h
 * @brief SIMD-accelerated operations (SSE4.2 / NEON when available, else scalar).
 *
 * HAS_AVX2 only pulls in <immintrin.h>; the kernels themselves are 128-bit SSE
 * or NEON plus a scalar tail. Pointers must be non-NULL when n > 0.
 */
#ifndef HPC_SIMD_OPS_H
#define HPC_SIMD_OPS_H

#include <stddef.h>

/**
 * @brief Element-wise add: dst[i] = a[i] + b[i].
 * @param dst Output array.
 * @param a   First input array.
 * @param b   Second input array.
 * @param n   Number of elements.
 */
void simd_add_f32(float *dst, const float *a, const float *b, size_t n);

/**
 * @brief Element-wise multiply: dst[i] = a[i] * b[i].
 * @param dst Output array.
 * @param a   First input array.
 * @param b   Second input array.
 * @param n   Number of elements.
 */
void simd_mul_f32(float *dst, const float *a, const float *b, size_t n);

/**
 * @brief Scale: dst[i] = a[i] * scalar.
 * @param dst    Output array.
 * @param a      Input array.
 * @param scalar Scale factor.
 * @param n      Number of elements.
 */
void simd_scale_f32(float *dst, const float *a, float scalar, size_t n);

/**
 * @brief Dot product of two float arrays.
 * @param a First input array.
 * @param b Second input array.
 * @param n Number of elements.
 * @return Dot product result.
 */
float simd_dot_f32(const float *a, const float *b, size_t n);

/**
 * @brief Sum all elements of a float array.
 * @param a Input array.
 * @param n Number of elements.
 * @return Sum of all elements.
 */
float simd_sum_f32(const float *a, size_t n);

#endif /* HPC_SIMD_OPS_H */
