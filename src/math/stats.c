/**
 * @file stats.c
 * @brief Descriptive statistics implementation.
 */
#include "math/stats.h"

ErrorCode stats_mean(const float *data, size_t n, float *out) {
    if (!data || n == 0 || !out) return ERR_INVALID_ARG;

    /* Accumulate in double to limit rounding error over long arrays. */
    double sum = 0.0;
    for (size_t i = 0; i < n; i++) sum += (double)data[i];
    *out = (float)(sum / (double)n);
    return ERR_OK;
}

ErrorCode stats_variance(const float *data, size_t n, float *out) {
    if (!data || n < 2 || !out) return ERR_INVALID_ARG;

    /* Welford's online algorithm — numerically stable single pass. */
    double mean = 0.0;
    double m2 = 0.0;
    for (size_t i = 0; i < n; i++) {
        double x = (double)data[i];
        double delta = x - mean;
        mean += delta / (double)(i + 1);
        m2 += delta * (x - mean);
    }
    *out = (float)(m2 / (double)(n - 1));
    return ERR_OK;
}

ErrorCode stats_min_max(const float *data, size_t n, float *out_min, float *out_max) {
    if (!data || n == 0 || (!out_min && !out_max)) return ERR_INVALID_ARG;

    float lo = data[0];
    float hi = data[0];
    for (size_t i = 1; i < n; i++) {
        if (data[i] < lo) lo = data[i];
        if (data[i] > hi) hi = data[i];
    }
    if (out_min) *out_min = lo;
    if (out_max) *out_max = hi;
    return ERR_OK;
}
