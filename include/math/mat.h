/**
 * @file mat.h
 * @brief 4x4 float matrices with 3D transform builders (column-major).
 *
 * Storage is column-major to match OpenGL and cglm: element (row r, col c)
 * is m[c * 4 + r]. All operations are infallible and work by value.
 */
#ifndef MATH_MAT_H
#define MATH_MAT_H

#include "math/vec.h"
#include <stdbool.h>

/** @brief 4x4 float matrix, column-major: element (r, c) = m[c * 4 + r]. */
typedef struct Mat4 {
    float m[16];
} Mat4;

/** @brief Identity matrix. */
Mat4 mat4_identity(void);

/** @brief Matrix product a * b (applies b first, then a). */
Mat4 mat4_mul(Mat4 a, Mat4 b);

/** @brief Transpose. */
Mat4 mat4_transpose(Mat4 a);

/**
 * @brief General matrix inverse.
 * @param a       Matrix to invert.
 * @param out_inv Receives the inverse when invertible (may be NULL to test only).
 * @return true if a is invertible, false if the determinant is ~0.
 */
bool mat4_inverse(Mat4 a, Mat4 *out_inv);

/** @brief Transform a Vec4 (full 4x4 multiply). */
Vec4 mat4_mul_vec4(Mat4 a, Vec4 v);

/** @brief Transform a point (w = 1), returning the xyz of the result. */
Vec3 mat4_mul_point(Mat4 a, Vec3 p);

/* ── Transform builders ─────────────────────────────────────────────────── */

/** @brief Translation by t. */
Mat4 mat4_translate(Vec3 t);

/** @brief Non-uniform scale by s. */
Mat4 mat4_scale(Vec3 s);

/** @brief Rotation around the X axis by angle radians. */
Mat4 mat4_rotate_x(float angle);

/** @brief Rotation around the Y axis by angle radians. */
Mat4 mat4_rotate_y(float angle);

/** @brief Rotation around the Z axis by angle radians. */
Mat4 mat4_rotate_z(float angle);

/** @brief Rotation around an arbitrary axis (normalized internally) by angle radians. */
Mat4 mat4_rotate_axis(Vec3 axis, float angle);

/* ── Camera / projection builders (right-handed, OpenGL clip space) ─────── */

/**
 * @brief Perspective projection.
 * @param fovy_rad Vertical field of view in radians.
 * @param aspect   Width / height ratio.
 * @param near_z   Near clip plane distance (> 0).
 * @param far_z    Far clip plane distance (> near_z).
 */
Mat4 mat4_perspective(float fovy_rad, float aspect, float near_z, float far_z);

/** @brief Orthographic projection over [left,right] x [bottom,top] x [near_z,far_z]. */
Mat4 mat4_ortho(float left, float right, float bottom, float top, float near_z, float far_z);

/**
 * @brief View matrix looking from eye toward center.
 * @param eye    Camera position.
 * @param center Point the camera looks at.
 * @param up     World up direction (typically {0, 1, 0}).
 */
Mat4 mat4_look_at(Vec3 eye, Vec3 center, Vec3 up);

#endif /* MATH_MAT_H */
