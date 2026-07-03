/**
 * @file physics.c
 * @brief Particle physics simulation: integrators, springs, collisions, confinement.
 */
#include "simulation/physics.h"
#include <math.h>

void physics_step_euler(Particle *particles, size_t count, float dt, Vec3 gravity) {
    for (size_t i = 0; i < count; i++) {
        Particle *p = &particles[i];
        p->vel.x += gravity.x * dt;
        p->vel.y += gravity.y * dt;
        p->vel.z += gravity.z * dt;
        p->pos.x += p->vel.x * dt;
        p->pos.y += p->vel.y * dt;
        p->pos.z += p->vel.z * dt;
    }
}

void physics_step_verlet(Particle *particles, size_t count, float dt, Vec3 gravity) {
    for (size_t i = 0; i < count; i++) {
        Particle *p = &particles[i];
        float     nx = 2.0f * p->pos.x - p->prev_pos.x + gravity.x * dt * dt;
        float     ny = 2.0f * p->pos.y - p->prev_pos.y + gravity.y * dt * dt;
        float     nz = 2.0f * p->pos.z - p->prev_pos.z + gravity.z * dt * dt;
        p->prev_pos = p->pos;
        p->pos.x = nx;
        p->pos.y = ny;
        p->pos.z = nz;
        /* Derive velocity for external use */
        p->vel.x = (p->pos.x - p->prev_pos.x) / dt;
        p->vel.y = (p->pos.y - p->prev_pos.y) / dt;
        p->vel.z = (p->pos.z - p->prev_pos.z) / dt;
    }
}

void physics_apply_spring(Particle *a, Particle *b, float rest_len, float k, float damping,
                          float dt) {
    float dx = b->pos.x - a->pos.x;
    float dy = b->pos.y - a->pos.y;
    float dz = b->pos.z - a->pos.z;
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);
    if (dist < 1e-8f) return;
    float stretch = dist - rest_len;
    float force = k * stretch;
    /* Relative velocity along spring axis for damping */
    float rvx = b->vel.x - a->vel.x;
    float rvy = b->vel.y - a->vel.y;
    float rvz = b->vel.z - a->vel.z;
    float rel_v = (rvx * dx + rvy * dy + rvz * dz) / dist;
    force += damping * rel_v;
    float fx = (force / dist) * dx;
    float fy = (force / dist) * dy;
    float fz = (force / dist) * dz;
    float inv_a = dt / a->mass, inv_b = dt / b->mass;
    a->vel.x += fx * inv_a;
    a->vel.y += fy * inv_a;
    a->vel.z += fz * inv_a;
    b->vel.x -= fx * inv_b;
    b->vel.y -= fy * inv_b;
    b->vel.z -= fz * inv_b;
}

void physics_collide_spheres(Particle *a, Particle *b, float radius) {
    float dx = b->pos.x - a->pos.x;
    float dy = b->pos.y - a->pos.y;
    float dz = b->pos.z - a->pos.z;
    float dist2 = dx * dx + dy * dy + dz * dz;
    float min_dist = 2.0f * radius;
    if (dist2 >= min_dist * min_dist || dist2 < 1e-12f) return;
    float dist = sqrtf(dist2);
    float overlap = min_dist - dist;
    float nx = dx / dist, ny = dy / dist, nz = dz / dist;
    /* Push apart equally */
    float half = overlap * 0.5f;
    a->pos.x -= nx * half;
    a->pos.y -= ny * half;
    a->pos.z -= nz * half;
    b->pos.x += nx * half;
    b->pos.y += ny * half;
    b->pos.z += nz * half;
    /* Elastic velocity exchange along normal */
    float va = a->vel.x * nx + a->vel.y * ny + a->vel.z * nz;
    float vb = b->vel.x * nx + b->vel.y * ny + b->vel.z * nz;
    a->vel.x += (vb - va) * nx;
    a->vel.y += (vb - va) * ny;
    a->vel.z += (vb - va) * nz;
    b->vel.x += (va - vb) * nx;
    b->vel.y += (va - vb) * ny;
    b->vel.z += (va - vb) * nz;
}

void physics_confine_box(Particle *p, Vec3 bounds, float restitution) {
    for (int axis = 0; axis < 3; axis++) {
        float *pos = &p->pos.x + axis;
        float *vel = &p->vel.x + axis;
        float  limit = *(&bounds.x + axis);
        if (*pos < 0.0f) {
            *pos = 0.0f;
            *vel = -*vel * restitution;
        }
        if (*pos > limit) {
            *pos = limit;
            *vel = -*vel * restitution;
        }
    }
}
