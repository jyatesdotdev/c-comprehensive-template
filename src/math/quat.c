/**
 * @file quat.c
 * @brief Quaternion implementation.
 */
#include "math/quat.h"

#include <math.h>

Quat quat_identity(void) {
    return (Quat){0.0f, 0.0f, 0.0f, 1.0f};
}

Quat quat_from_axis_angle(Vec3 axis, float angle_rad) {
    Vec3  a = vec3_normalize(axis);
    float half = angle_rad * 0.5f;
    float s = sinf(half);
    return (Quat){a.x * s, a.y * s, a.z * s, cosf(half)};
}

Quat quat_mul(Quat a, Quat b) {
    return (Quat){
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
    };
}

Quat quat_normalize(Quat q) {
    float len = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (len <= 0.0f) return quat_identity();
    float inv = 1.0f / len;
    return (Quat){q.x * inv, q.y * inv, q.z * inv, q.w * inv};
}

Quat quat_conjugate(Quat q) {
    return (Quat){-q.x, -q.y, -q.z, q.w};
}

Vec3 quat_rotate_vec3(Quat q, Vec3 v) {
    /* v' = q * (v, 0) * q^-1, expanded to avoid constructing temporaries:
       t = 2 * (qv x v); v' = v + w * t + qv x t */
    Vec3 qv = {q.x, q.y, q.z};
    Vec3 t = vec3_scale(vec3_cross(qv, v), 2.0f);
    return vec3_add(vec3_add(v, vec3_scale(t, q.w)), vec3_cross(qv, t));
}

Quat quat_slerp(Quat a, Quat b, float t) {
    float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;

    /* Take the shorter arc: q and -q are the same rotation. */
    if (dot < 0.0f) {
        b = (Quat){-b.x, -b.y, -b.z, -b.w};
        dot = -dot;
    }

    /* Nearly parallel — fall back to normalized lerp to avoid division by ~0. */
    if (dot > 0.9995f) {
        Quat out = {
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t,
            a.w + (b.w - a.w) * t,
        };
        return quat_normalize(out);
    }

    float theta = acosf(dot);
    float sin_theta = sinf(theta);
    float wa = sinf((1.0f - t) * theta) / sin_theta;
    float wb = sinf(t * theta) / sin_theta;
    return (Quat){
        a.x * wa + b.x * wb,
        a.y * wa + b.y * wb,
        a.z * wa + b.z * wb,
        a.w * wa + b.w * wb,
    };
}

Mat4 quat_to_mat4(Quat q) {
    float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;

    Mat4 out = mat4_identity();
    /* Column-major: element (r, c) = m[c * 4 + r]. */
    out.m[0] = 1.0f - 2.0f * (yy + zz);
    out.m[1] = 2.0f * (xy + wz);
    out.m[2] = 2.0f * (xz - wy);
    out.m[4] = 2.0f * (xy - wz);
    out.m[5] = 1.0f - 2.0f * (xx + zz);
    out.m[6] = 2.0f * (yz + wx);
    out.m[8] = 2.0f * (xz + wy);
    out.m[9] = 2.0f * (yz - wx);
    out.m[10] = 1.0f - 2.0f * (xx + yy);
    return out;
}
