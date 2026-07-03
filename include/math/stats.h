/**
 * @file stats.h
 * @brief Basic descriptive statistics over float arrays.
 */
#ifndef MATH_STATS_H
#define MATH_STATS_H

#include "core/error.h"
#include <stddef.h>

/**
 * @brief Arithmetic mean of data[0..n).
 * @param data Input values (must not be NULL).
 * @param n    Element count (must be > 0).
 * @param out  Receives the mean.
 * @return ERR_OK, or ERR_INVALID_ARG for bad inputs.
 */
ErrorCode stats_mean(const float *data, size_t n, float *out);

/**
 * @brief Sample variance (n - 1 denominator) via Welford's algorithm.
 * @param data Input values (must not be NULL).
 * @param n    Element count (must be >= 2).
 * @param out  Receives the variance.
 * @return ERR_OK, or ERR_INVALID_ARG for bad inputs.
 */
ErrorCode stats_variance(const float *data, size_t n, float *out);

/**
 * @brief Minimum and maximum of data[0..n).
 * @param data    Input values (must not be NULL).
 * @param n       Element count (must be > 0).
 * @param out_min Receives the minimum (may be NULL if not wanted).
 * @param out_max Receives the maximum (may be NULL if not wanted).
 * @return ERR_OK, or ERR_INVALID_ARG for bad inputs.
 */
ErrorCode stats_min_max(const float *data, size_t n, float *out_min, float *out_max);

#endif /* MATH_STATS_H */
