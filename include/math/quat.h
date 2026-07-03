/**
 * @file quat.h
 * @brief Quaternions for 3D rotation (no gimbal lock, cheap interpolation).
 *
 * Convention: (x, y, z) is the vector part, w the scalar part. Rotation
 * quaternions must be unit length — normalize after accumulating products.
 */
#ifndef MATH_QUAT_H
#define MATH_QUAT_H

#include "math/mat.h"
#include "math/vec.h"

/** @brief Quaternion: xyz vector part, w scalar part. */
typedef struct Quat {
    float x, y, z, w;
} Quat;

/** @brief Identity rotation (no rotation). */
Quat quat_identity(void);

/**
 * @brief Rotation of angle radians around an axis (normalized internally).
 * @param axis      Rotation axis.
 * @param angle_rad Rotation angle in radians (right-hand rule).
 */
Quat quat_from_axis_angle(Vec3 axis, float angle_rad);

/** @brief Hamilton product a * b (applies b's rotation first, then a's). */
Quat quat_mul(Quat a, Quat b);

/** @brief Unit quaternion in the direction of q (identity if q is ~zero). */
Quat quat_normalize(Quat q);

/** @brief Conjugate (inverse rotation for unit quaternions). */
Quat quat_conjugate(Quat q);

/** @brief Rotate a vector by a unit quaternion. */
Vec3 quat_rotate_vec3(Quat q, Vec3 v);

/**
 * @brief Spherical linear interpolation between unit quaternions.
 * @param a Start rotation (t = 0).
 * @param b End rotation (t = 1).
 * @param t Interpolation parameter in [0, 1].
 */
Quat quat_slerp(Quat a, Quat b, float t);

/** @brief Convert a unit quaternion to a rotation matrix. */
Mat4 quat_to_mat4(Quat q);

#endif /* MATH_QUAT_H */
