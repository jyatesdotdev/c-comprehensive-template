/**
 * @file vec.h
 * @brief Fixed-size float vectors (Vec2/Vec3/Vec4) for graphics, physics, ML.
 *
 * All operations are infallible and pass/return small structs by value.
 * This header owns the project-wide Vec3 type (simulation/physics.h uses it).
 */
#ifndef MATH_VEC_H
#define MATH_VEC_H

#include <stdbool.h>

/** @brief 2D float vector. */
typedef struct Vec2 {
    float x, y;
} Vec2;

/** @brief 3D float vector. */
typedef struct Vec3 {
    float x, y, z;
} Vec3;

/** @brief 4D float vector (w = 1 for points, 0 for directions). */
typedef struct Vec4 {
    float x, y, z, w;
} Vec4;

/* ── Vec2 ───────────────────────────────────────────────────────────────── */

/**
 * @brief Component-wise a + b.
 * @param a Left operand.
 * @param b Right operand.
 * @return (a.x + b.x, a.y + b.y).
 */
Vec2 vec2_add(Vec2 a, Vec2 b);
/**
 * @brief Component-wise a - b.
 * @param a Left operand.
 * @param b Right operand.
 * @return (a.x - b.x, a.y - b.y).
 */
Vec2 vec2_sub(Vec2 a, Vec2 b);
/**
 * @brief Multiply every component by s.
 * @param v Vector.
 * @param s Scalar.
 * @return (v.x * s, v.y * s).
 */
Vec2 vec2_scale(Vec2 v, float s);
/**
 * @brief Dot product.
 * @param a Left operand.
 * @param b Right operand.
 * @return a.x * b.x + a.y * b.y.
 */
float vec2_dot(Vec2 a, Vec2 b);
/**
 * @brief Euclidean length.
 * @param v Vector.
 * @return sqrt(dot(v, v)).
 */
float vec2_length(Vec2 v);
/**
 * @brief Unit vector in the direction of v (zero vector stays zero).
 * @param v Vector.
 * @return v / |v|, or {0,0} if |v| == 0.
 */
Vec2 vec2_normalize(Vec2 v);
/**
 * @brief Linear interpolation from a to b by t.
 * @param a Start.
 * @param b End.
 * @param t Mix factor (0 = a, 1 = b; not clamped).
 * @return a + (b - a) * t.
 */
Vec2 vec2_lerp(Vec2 a, Vec2 b, float t);

/* ── Vec3 ───────────────────────────────────────────────────────────────── */

/**
 * @brief Component-wise a + b.
 * @param a Left operand.
 * @param b Right operand.
 * @return (a.x + b.x, a.y + b.y, a.z + b.z).
 */
Vec3 vec3_add(Vec3 a, Vec3 b);
/**
 * @brief Component-wise a - b.
 * @param a Left operand.
 * @param b Right operand.
 * @return (a.x - b.x, a.y - b.y, a.z - b.z).
 */
Vec3 vec3_sub(Vec3 a, Vec3 b);
/**
 * @brief Multiply every component by s.
 * @param v Vector.
 * @param s Scalar.
 * @return (v.x * s, v.y * s, v.z * s).
 */
Vec3 vec3_scale(Vec3 v, float s);
/**
 * @brief Dot product.
 * @param a Left operand.
 * @param b Right operand.
 * @return a.x * b.x + a.y * b.y + a.z * b.z.
 */
float vec3_dot(Vec3 a, Vec3 b);
/**
 * @brief Cross product (right-handed).
 * @param a Left operand.
 * @param b Right operand.
 * @return a × b.
 */
Vec3 vec3_cross(Vec3 a, Vec3 b);
/**
 * @brief Euclidean length.
 * @param v Vector.
 * @return sqrt(dot(v, v)).
 */
float vec3_length(Vec3 v);
/**
 * @brief Distance between points a and b.
 * @param a First point.
 * @param b Second point.
 * @return |b - a|.
 */
float vec3_distance(Vec3 a, Vec3 b);
/**
 * @brief Unit vector in the direction of v (zero vector stays zero).
 * @param v Vector.
 * @return v / |v|, or {0,0,0} if |v| == 0.
 */
Vec3 vec3_normalize(Vec3 v);
/**
 * @brief Linear interpolation from a to b by t.
 * @param a Start.
 * @param b End.
 * @param t Mix factor (0 = a, 1 = b; not clamped).
 * @return a + (b - a) * t.
 */
Vec3 vec3_lerp(Vec3 a, Vec3 b, float t);
/**
 * @brief Component-wise approximate equality within eps.
 * @param a   Left operand.
 * @param b   Right operand.
 * @param eps Maximum absolute difference per component.
 * @return true if each |a_i - b_i| <= eps.
 */
bool vec3_approx_eq(Vec3 a, Vec3 b, float eps);

/* ── Vec4 ───────────────────────────────────────────────────────────────── */

/**
 * @brief Component-wise a + b.
 * @param a Left operand.
 * @param b Right operand.
 * @return (a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w).
 */
Vec4 vec4_add(Vec4 a, Vec4 b);
/**
 * @brief Component-wise a - b.
 * @param a Left operand.
 * @param b Right operand.
 * @return (a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w).
 */
Vec4 vec4_sub(Vec4 a, Vec4 b);
/**
 * @brief Multiply every component by s.
 * @param v Vector.
 * @param s Scalar.
 * @return (v.x * s, v.y * s, v.z * s, v.w * s).
 */
Vec4 vec4_scale(Vec4 v, float s);
/**
 * @brief Dot product.
 * @param a Left operand.
 * @param b Right operand.
 * @return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w.
 */
float vec4_dot(Vec4 a, Vec4 b);
/**
 * @brief Euclidean length.
 * @param v Vector.
 * @return sqrt(dot(v, v)).
 */
float vec4_length(Vec4 v);

#endif /* MATH_VEC_H */
