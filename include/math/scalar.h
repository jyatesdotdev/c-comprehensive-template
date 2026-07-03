/**
 * @file scalar.h
 * @brief Scalar math helpers: clamp, lerp, angle conversion, float comparison.
 *
 * Small, hot, infallible functions — implemented as static inline per
 * docs/BEST_PRACTICES.md (Performance Considerations).
 */
#ifndef MATH_SCALAR_H
#define MATH_SCALAR_H

#include <math.h>
#include <stdbool.h>

/** @brief Pi as float. */
#define SCALAR_PI 3.14159265358979323846f

/** @brief Clamp x into [lo, hi]. */
static inline float scalar_clamp(float x, float lo, float hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}

/** @brief Linear interpolation from a to b by t (t=0 → a, t=1 → b). */
static inline float scalar_lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

/** @brief Convert degrees to radians. */
static inline float scalar_deg_to_rad(float deg) {
    return deg * (SCALAR_PI / 180.0f);
}

/** @brief Convert radians to degrees. */
static inline float scalar_rad_to_deg(float rad) {
    return rad * (180.0f / SCALAR_PI);
}

/**
 * @brief Approximate float equality within an absolute tolerance.
 * @param a   First value.
 * @param b   Second value.
 * @param eps Maximum allowed absolute difference (e.g. 1e-5f).
 * @return true if |a - b| <= eps.
 */
static inline bool scalar_approx_eq(float a, float b, float eps) {
    return fabsf(a - b) <= eps;
}

#endif /* MATH_SCALAR_H */
