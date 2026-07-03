/**
 * @file matx.h
 * @brief Dynamically sized float matrices (row-major) for ML-style workloads.
 *
 * Heap-backed by default, or arena-backed via matx_init_arena. Row-major:
 * element (r, c) is data[r * cols + c]. For production ML use a BLAS; these
 * implementations are self-contained and show naive vs cache-blocked matmul.
 */
#ifndef MATH_MATX_H
#define MATH_MATX_H

#include "core/error.h"
#include "memory/arena.h"
#include "math/rng.h"
#include <stddef.h>

/** @brief Dynamically sized row-major float matrix. */
typedef struct MatX {
    float *data;      /**< rows * cols floats, row-major. */
    size_t rows;      /**< Number of rows. */
    size_t cols;      /**< Number of columns. */
    int    owns_data; /**< Nonzero if data is heap-owned (freed by matx_destroy). */
} MatX;

/**
 * @brief Allocate a zero-initialized rows x cols matrix on the heap.
 * @param m    Matrix to initialize.
 * @param rows Row count (> 0).
 * @param cols Column count (> 0).
 * @return ERR_OK, ERR_INVALID_ARG for bad inputs, ERR_OVERFLOW if rows * cols
 *         overflows, ERR_NOMEM on allocation failure. Free with matx_destroy.
 */
ErrorCode matx_init(MatX *m, size_t rows, size_t cols);

/**
 * @brief Allocate a zero-initialized matrix from an arena.
 * @param m     Matrix to initialize.
 * @param arena Arena to allocate from (memory is released with the arena —
 *              do NOT call matx_destroy).
 * @param rows  Row count (> 0).
 * @param cols  Column count (> 0).
 * @return ERR_OK, ERR_INVALID_ARG, ERR_OVERFLOW, or ERR_NOMEM if the arena is full.
 */
ErrorCode matx_init_arena(MatX *m, Arena *arena, size_t rows, size_t cols);

/** @brief Free a heap-backed matrix (no-op for arena-backed or NULL). */
void matx_destroy(MatX *m);

/** @brief Element (r, c). No bounds checking — callers validate indices. */
static inline float matx_get(const MatX *m, size_t r, size_t c) {
    return m->data[r * m->cols + c];
}

/** @brief Set element (r, c). No bounds checking — callers validate indices. */
static inline void matx_set(MatX *m, size_t r, size_t c, float v) {
    m->data[r * m->cols + c] = v;
}

/** @brief Set every element to value. */
ErrorCode matx_fill(MatX *m, float value);

/** @brief Fill with uniform random values in [lo, hi) — e.g. ML weight init. */
ErrorCode matx_random_uniform(MatX *m, Rng *rng, float lo, float hi);

/**
 * @brief Element-wise out = a + b. All three must have identical dimensions
 *        and be pre-allocated (out may alias a or b).
 */
ErrorCode matx_add(const MatX *a, const MatX *b, MatX *out);

/**
 * @brief Matrix product out = a * b (naive triple loop).
 * @param a   Left operand (n x k).
 * @param b   Right operand (k x m).
 * @param out Pre-allocated result (n x m); must not alias a or b.
 * @return ERR_OK, or ERR_INVALID_ARG on NULL/dimension mismatch/aliasing.
 */
ErrorCode matx_mul(const MatX *a, const MatX *b, MatX *out);

/**
 * @brief Matrix product out = a * b, cache-blocked (tiled) — same contract as
 *        matx_mul but with much better cache behavior on large matrices.
 * @param block Tile edge length (e.g. 32); 0 selects a sensible default.
 */
ErrorCode matx_mul_blocked(const MatX *a, const MatX *b, MatX *out, size_t block);

/** @brief out = a transposed. out must be pre-allocated cols x rows; no aliasing. */
ErrorCode matx_transpose(const MatX *a, MatX *out);

#endif /* MATH_MATX_H */
