/**
 * @file matx.c
 * @brief Dynamic matrix implementation: naive and cache-blocked matmul.
 */
#include "math/matx.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MATX_DEFAULT_BLOCK 32

/** Validate dimensions and compute element count, rejecting overflow. */
static ErrorCode checked_count(size_t rows, size_t cols, size_t *out_count) {
    if (rows == 0 || cols == 0) return ERR_INVALID_ARG;
    if (rows > SIZE_MAX / cols) return ERR_OVERFLOW;
    if (rows * cols > SIZE_MAX / sizeof(float)) return ERR_OVERFLOW;
    *out_count = rows * cols;
    return ERR_OK;
}

ErrorCode matx_init(MatX *m, size_t rows, size_t cols) {
    if (!m) return ERR_INVALID_ARG;

    size_t    count = 0;
    ErrorCode err = checked_count(rows, cols, &count);
    if (err) return err;

    m->data = calloc(count, sizeof(float));
    if (!m->data) return ERR_NOMEM;
    m->rows = rows;
    m->cols = cols;
    m->owns_data = 1;
    return ERR_OK;
}

ErrorCode matx_init_arena(MatX *m, Arena *arena, size_t rows, size_t cols) {
    if (!m || !arena) return ERR_INVALID_ARG;

    size_t    count = 0;
    ErrorCode err = checked_count(rows, cols, &count);
    if (err) return err;

    m->data = arena_alloc(arena, count * sizeof(float), _Alignof(float));
    if (!m->data) return ERR_NOMEM;
    memset(m->data, 0, count * sizeof(float));
    m->rows = rows;
    m->cols = cols;
    m->owns_data = 0;
    return ERR_OK;
}

void matx_destroy(MatX *m) {
    if (m) {
        if (m->owns_data) free(m->data);
        m->data = NULL;
        m->rows = 0;
        m->cols = 0;
        m->owns_data = 0;
    }
}

ErrorCode matx_fill(MatX *m, float value) {
    if (!m || !m->data) return ERR_INVALID_ARG;
    for (size_t i = 0; i < m->rows * m->cols; i++) m->data[i] = value;
    return ERR_OK;
}

ErrorCode matx_random_uniform(MatX *m, Rng *rng, float lo, float hi) {
    if (!m || !m->data || !rng) return ERR_INVALID_ARG;
    for (size_t i = 0; i < m->rows * m->cols; i++) m->data[i] = rng_range_float(rng, lo, hi);
    return ERR_OK;
}

/** Shared validation for element-wise binary operations. */
static ErrorCode check_elementwise(const MatX *a, const MatX *b, const MatX *out) {
    if (!a || !b || !out || !a->data || !b->data || !out->data) return ERR_INVALID_ARG;
    if (a->rows != b->rows || a->cols != b->cols) return ERR_INVALID_ARG;
    if (out->rows != a->rows || out->cols != a->cols) return ERR_INVALID_ARG;
    return ERR_OK;
}

ErrorCode matx_add(const MatX *a, const MatX *b, MatX *out) {
    ErrorCode err = check_elementwise(a, b, out);
    if (err) return err;
    for (size_t i = 0; i < a->rows * a->cols; i++) out->data[i] = a->data[i] + b->data[i];
    return ERR_OK;
}

ErrorCode matx_sub(const MatX *a, const MatX *b, MatX *out) {
    ErrorCode err = check_elementwise(a, b, out);
    if (err) return err;
    for (size_t i = 0; i < a->rows * a->cols; i++) out->data[i] = a->data[i] - b->data[i];
    return ERR_OK;
}

ErrorCode matx_hadamard(const MatX *a, const MatX *b, MatX *out) {
    ErrorCode err = check_elementwise(a, b, out);
    if (err) return err;
    for (size_t i = 0; i < a->rows * a->cols; i++) out->data[i] = a->data[i] * b->data[i];
    return ERR_OK;
}

ErrorCode matx_scale(const MatX *a, float s, MatX *out) {
    if (!a || !out || !a->data || !out->data) return ERR_INVALID_ARG;
    if (out->rows != a->rows || out->cols != a->cols) return ERR_INVALID_ARG;
    for (size_t i = 0; i < a->rows * a->cols; i++) out->data[i] = a->data[i] * s;
    return ERR_OK;
}

ErrorCode matx_map(const MatX *a, float (*fn)(float), MatX *out) {
    if (!a || !fn || !out || !a->data || !out->data) return ERR_INVALID_ARG;
    if (out->rows != a->rows || out->cols != a->cols) return ERR_INVALID_ARG;
    for (size_t i = 0; i < a->rows * a->cols; i++) out->data[i] = fn(a->data[i]);
    return ERR_OK;
}

ErrorCode matx_add_row(const MatX *a, const MatX *row, MatX *out) {
    if (!a || !row || !out || !a->data || !row->data || !out->data) return ERR_INVALID_ARG;
    if (row->rows != 1 || row->cols != a->cols) return ERR_INVALID_ARG;
    if (out->rows != a->rows || out->cols != a->cols) return ERR_INVALID_ARG;

    for (size_t r = 0; r < a->rows; r++)
        for (size_t c = 0; c < a->cols; c++)
            out->data[r * a->cols + c] = a->data[r * a->cols + c] + row->data[c];
    return ERR_OK;
}

ErrorCode matx_col_sums(const MatX *a, MatX *out) {
    if (!a || !out || !a->data || !out->data) return ERR_INVALID_ARG;
    if (out->rows != 1 || out->cols != a->cols || out->data == a->data) return ERR_INVALID_ARG;

    for (size_t c = 0; c < a->cols; c++) out->data[c] = 0.0f;
    for (size_t r = 0; r < a->rows; r++)
        for (size_t c = 0; c < a->cols; c++) out->data[c] += a->data[r * a->cols + c];
    return ERR_OK;
}

ErrorCode matx_row_argmax(const MatX *a, size_t r, size_t *out_col) {
    if (!a || !a->data || r >= a->rows || !out_col) return ERR_INVALID_ARG;

    size_t best = 0;
    for (size_t c = 1; c < a->cols; c++)
        if (matx_get(a, r, c) > matx_get(a, r, best)) best = c;
    *out_col = best;
    return ERR_OK;
}

/** Shared validation for the matmul variants. */
static ErrorCode check_mul(const MatX *a, const MatX *b, const MatX *out) {
    if (!a || !b || !out || !a->data || !b->data || !out->data) return ERR_INVALID_ARG;
    if (a->cols != b->rows) return ERR_INVALID_ARG;
    if (out->rows != a->rows || out->cols != b->cols) return ERR_INVALID_ARG;
    if (out->data == a->data || out->data == b->data) return ERR_INVALID_ARG;
    return ERR_OK;
}

ErrorCode matx_mul(const MatX *a, const MatX *b, MatX *out) {
    ErrorCode err = check_mul(a, b, out);
    if (err) return err;

    /* i-k-j loop order: the inner loop walks both b and out row-wise. */
    memset(out->data, 0, out->rows * out->cols * sizeof(float));
    for (size_t i = 0; i < a->rows; i++) {
        for (size_t k = 0; k < a->cols; k++) {
            float aik = matx_get(a, i, k);
            for (size_t j = 0; j < b->cols; j++) {
                out->data[i * out->cols + j] += aik * b->data[k * b->cols + j];
            }
        }
    }
    return ERR_OK;
}

ErrorCode matx_mul_blocked(const MatX *a, const MatX *b, MatX *out, size_t block) {
    ErrorCode err = check_mul(a, b, out);
    if (err) return err;
    if (block == 0) block = MATX_DEFAULT_BLOCK;

    /* Tile all three loops so working sets stay cache-resident. */
    memset(out->data, 0, out->rows * out->cols * sizeof(float));
    for (size_t ii = 0; ii < a->rows; ii += block) {
        size_t i_end = ii + block < a->rows ? ii + block : a->rows;
        for (size_t kk = 0; kk < a->cols; kk += block) {
            size_t k_end = kk + block < a->cols ? kk + block : a->cols;
            for (size_t jj = 0; jj < b->cols; jj += block) {
                size_t j_end = jj + block < b->cols ? jj + block : b->cols;
                for (size_t i = ii; i < i_end; i++) {
                    for (size_t k = kk; k < k_end; k++) {
                        float aik = matx_get(a, i, k);
                        for (size_t j = jj; j < j_end; j++) {
                            out->data[i * out->cols + j] += aik * b->data[k * b->cols + j];
                        }
                    }
                }
            }
        }
    }
    return ERR_OK;
}

ErrorCode matx_transpose(const MatX *a, MatX *out) {
    if (!a || !out || !a->data || !out->data) return ERR_INVALID_ARG;
    if (out->rows != a->cols || out->cols != a->rows) return ERR_INVALID_ARG;
    if (out->data == a->data) return ERR_INVALID_ARG;

    for (size_t r = 0; r < a->rows; r++)
        for (size_t c = 0; c < a->cols; c++) matx_set(out, c, r, matx_get(a, r, c));
    return ERR_OK;
}
