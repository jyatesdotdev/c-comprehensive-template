/**
 * @file mat.c
 * @brief 4x4 matrix implementation (column-major).
 */
#include "math/mat.h"

#include <math.h>

/* Element (row r, col c) of column-major storage. */
#define M(mat, r, c) ((mat).m[(c) * 4 + (r)])

Mat4 mat4_identity(void) {
    Mat4 out = {{0}};
    out.m[0] = out.m[5] = out.m[10] = out.m[15] = 1.0f;
    return out;
}

Mat4 mat4_mul(Mat4 a, Mat4 b) {
    Mat4 out;
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) sum += M(a, r, k) * M(b, k, c);
            M(out, r, c) = sum;
        }
    }
    return out;
}

Mat4 mat4_transpose(Mat4 a) {
    Mat4 out;
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++) M(out, r, c) = M(a, c, r);
    return out;
}

/* Cofactor-expansion inverse (the well-known gluInvertMatrix formulation). */
bool mat4_inverse(Mat4 a, Mat4 *out_inv) {
    const float *m = a.m;
    float        inv[16];

    inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] +
             m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] -
             m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
    inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] +
             m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
    inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] -
              m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
    inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] -
             m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] +
             m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] -
             m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
    inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] +
              m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
    inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] +
             m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
    inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] -
             m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
    inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] +
              m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
    inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] -
              m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
    inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] -
             m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
    inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] +
             m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] -
              m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
    inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] +
              m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

    float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
    if (fabsf(det) < 1e-12f) return false;

    if (out_inv) {
        float inv_det = 1.0f / det;
        for (int i = 0; i < 16; i++) out_inv->m[i] = inv[i] * inv_det;
    }
    return true;
}

Vec4 mat4_mul_vec4(Mat4 a, Vec4 v) {
    Vec4 out;
    out.x = M(a, 0, 0) * v.x + M(a, 0, 1) * v.y + M(a, 0, 2) * v.z + M(a, 0, 3) * v.w;
    out.y = M(a, 1, 0) * v.x + M(a, 1, 1) * v.y + M(a, 1, 2) * v.z + M(a, 1, 3) * v.w;
    out.z = M(a, 2, 0) * v.x + M(a, 2, 1) * v.y + M(a, 2, 2) * v.z + M(a, 2, 3) * v.w;
    out.w = M(a, 3, 0) * v.x + M(a, 3, 1) * v.y + M(a, 3, 2) * v.z + M(a, 3, 3) * v.w;
    return out;
}

Vec3 mat4_mul_point(Mat4 a, Vec3 p) {
    Vec4 r = mat4_mul_vec4(a, (Vec4){p.x, p.y, p.z, 1.0f});
    return (Vec3){r.x, r.y, r.z};
}

Mat4 mat4_translate(Vec3 t) {
    Mat4 out = mat4_identity();
    M(out, 0, 3) = t.x;
    M(out, 1, 3) = t.y;
    M(out, 2, 3) = t.z;
    return out;
}

Mat4 mat4_scale(Vec3 s) {
    Mat4 out = mat4_identity();
    M(out, 0, 0) = s.x;
    M(out, 1, 1) = s.y;
    M(out, 2, 2) = s.z;
    return out;
}

Mat4 mat4_rotate_x(float angle) {
    Mat4  out = mat4_identity();
    float c = cosf(angle), s = sinf(angle);
    M(out, 1, 1) = c;
    M(out, 1, 2) = -s;
    M(out, 2, 1) = s;
    M(out, 2, 2) = c;
    return out;
}

Mat4 mat4_rotate_y(float angle) {
    Mat4  out = mat4_identity();
    float c = cosf(angle), s = sinf(angle);
    M(out, 0, 0) = c;
    M(out, 0, 2) = s;
    M(out, 2, 0) = -s;
    M(out, 2, 2) = c;
    return out;
}

Mat4 mat4_rotate_z(float angle) {
    Mat4  out = mat4_identity();
    float c = cosf(angle), s = sinf(angle);
    M(out, 0, 0) = c;
    M(out, 0, 1) = -s;
    M(out, 1, 0) = s;
    M(out, 1, 1) = c;
    return out;
}

Mat4 mat4_rotate_axis(Vec3 axis, float angle) {
    Vec3  a = vec3_normalize(axis);
    float c = cosf(angle), s = sinf(angle), t = 1.0f - c;

    Mat4 out = mat4_identity();
    M(out, 0, 0) = c + a.x * a.x * t;
    M(out, 0, 1) = a.x * a.y * t - a.z * s;
    M(out, 0, 2) = a.x * a.z * t + a.y * s;
    M(out, 1, 0) = a.y * a.x * t + a.z * s;
    M(out, 1, 1) = c + a.y * a.y * t;
    M(out, 1, 2) = a.y * a.z * t - a.x * s;
    M(out, 2, 0) = a.z * a.x * t - a.y * s;
    M(out, 2, 1) = a.z * a.y * t + a.x * s;
    M(out, 2, 2) = c + a.z * a.z * t;
    return out;
}

Mat4 mat4_perspective(float fovy_rad, float aspect, float near_z, float far_z) {
    float f = 1.0f / tanf(fovy_rad * 0.5f);
    Mat4  out = {{0}};
    M(out, 0, 0) = f / aspect;
    M(out, 1, 1) = f;
    M(out, 2, 2) = (far_z + near_z) / (near_z - far_z);
    M(out, 2, 3) = (2.0f * far_z * near_z) / (near_z - far_z);
    M(out, 3, 2) = -1.0f;
    return out;
}

Mat4 mat4_ortho(float left, float right, float bottom, float top, float near_z, float far_z) {
    Mat4 out = mat4_identity();
    M(out, 0, 0) = 2.0f / (right - left);
    M(out, 1, 1) = 2.0f / (top - bottom);
    M(out, 2, 2) = -2.0f / (far_z - near_z);
    M(out, 0, 3) = -(right + left) / (right - left);
    M(out, 1, 3) = -(top + bottom) / (top - bottom);
    M(out, 2, 3) = -(far_z + near_z) / (far_z - near_z);
    return out;
}

Mat4 mat4_look_at(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = vec3_normalize(vec3_sub(center, eye));
    Vec3 s = vec3_normalize(vec3_cross(f, up));
    Vec3 u = vec3_cross(s, f);

    Mat4 out = mat4_identity();
    M(out, 0, 0) = s.x;
    M(out, 0, 1) = s.y;
    M(out, 0, 2) = s.z;
    M(out, 1, 0) = u.x;
    M(out, 1, 1) = u.y;
    M(out, 1, 2) = u.z;
    M(out, 2, 0) = -f.x;
    M(out, 2, 1) = -f.y;
    M(out, 2, 2) = -f.z;
    M(out, 0, 3) = -vec3_dot(s, eye);
    M(out, 1, 3) = -vec3_dot(u, eye);
    M(out, 2, 3) = vec3_dot(f, eye);
    return out;
}
