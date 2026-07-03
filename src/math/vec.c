/**
 * @file vec.c
 * @brief Fixed-size float vector implementations.
 */
#include "math/vec.h"
#include "math/scalar.h"

#include <math.h>

/* ── Vec2 ───────────────────────────────────────────────────────────────── */

Vec2 vec2_add(Vec2 a, Vec2 b) {
    return (Vec2){a.x + b.x, a.y + b.y};
}

Vec2 vec2_sub(Vec2 a, Vec2 b) {
    return (Vec2){a.x - b.x, a.y - b.y};
}

Vec2 vec2_scale(Vec2 v, float s) {
    return (Vec2){v.x * s, v.y * s};
}

float vec2_dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

float vec2_length(Vec2 v) {
    return sqrtf(vec2_dot(v, v));
}

Vec2 vec2_normalize(Vec2 v) {
    float len = vec2_length(v);
    return len > 0.0f ? vec2_scale(v, 1.0f / len) : v;
}

Vec2 vec2_lerp(Vec2 a, Vec2 b, float t) {
    return (Vec2){scalar_lerp(a.x, b.x, t), scalar_lerp(a.y, b.y, t)};
}

/* ── Vec3 ───────────────────────────────────────────────────────────────── */

Vec3 vec3_add(Vec3 a, Vec3 b) {
    return (Vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

Vec3 vec3_sub(Vec3 a, Vec3 b) {
    return (Vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 vec3_scale(Vec3 v, float s) {
    return (Vec3){v.x * s, v.y * s, v.z * s};
}

float vec3_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return (Vec3){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

float vec3_length(Vec3 v) {
    return sqrtf(vec3_dot(v, v));
}

float vec3_distance(Vec3 a, Vec3 b) {
    return vec3_length(vec3_sub(a, b));
}

Vec3 vec3_normalize(Vec3 v) {
    float len = vec3_length(v);
    return len > 0.0f ? vec3_scale(v, 1.0f / len) : v;
}

Vec3 vec3_lerp(Vec3 a, Vec3 b, float t) {
    return (Vec3){scalar_lerp(a.x, b.x, t), scalar_lerp(a.y, b.y, t), scalar_lerp(a.z, b.z, t)};
}

bool vec3_approx_eq(Vec3 a, Vec3 b, float eps) {
    return scalar_approx_eq(a.x, b.x, eps) && scalar_approx_eq(a.y, b.y, eps) &&
           scalar_approx_eq(a.z, b.z, eps);
}

/* ── Vec4 ───────────────────────────────────────────────────────────────── */

Vec4 vec4_add(Vec4 a, Vec4 b) {
    return (Vec4){a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}

Vec4 vec4_sub(Vec4 a, Vec4 b) {
    return (Vec4){a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}

Vec4 vec4_scale(Vec4 v, float s) {
    return (Vec4){v.x * s, v.y * s, v.z * s, v.w * s};
}

float vec4_dot(Vec4 a, Vec4 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

float vec4_length(Vec4 v) {
    return sqrtf(vec4_dot(v, v));
}
