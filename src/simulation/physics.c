/**
 * @file physics.c
 * @brief Particle physics simulation: integrators, springs, collisions, confinement.
 *
 * Built on math/vec.h — the open-coded component arithmetic this file once
 * carried now lives behind vec3_* operations.
 */
#include "simulation/physics.h"
#include "math/vec.h"

#include <math.h>

void physics_step_euler(Particle *particles, size_t count, float dt, Vec3 gravity) {
    if (!particles) return;
    for (size_t i = 0; i < count; i++) {
        Particle *p = &particles[i];
        p->vel = vec3_add(p->vel, vec3_scale(gravity, dt));
        p->pos = vec3_add(p->pos, vec3_scale(p->vel, dt));
    }
}

void physics_step_verlet(Particle *particles, size_t count, float dt, Vec3 gravity) {
    if (!particles || dt == 0.0f) return;
    for (size_t i = 0; i < count; i++) {
        Particle *p = &particles[i];
        /* x' = 2x - x_prev + g * dt^2 */
        Vec3 next =
            vec3_add(vec3_sub(vec3_scale(p->pos, 2.0f), p->prev_pos), vec3_scale(gravity, dt * dt));
        p->prev_pos = p->pos;
        p->pos = next;
        /* Derive velocity for external use */
        p->vel = vec3_scale(vec3_sub(p->pos, p->prev_pos), 1.0f / dt);
    }
}

void physics_apply_spring(Particle *a, Particle *b, float rest_len, float k, float damping,
                          float dt) {
    if (!a || !b || a->mass <= 0.0f || b->mass <= 0.0f) return;

    Vec3  delta = vec3_sub(b->pos, a->pos);
    float dist = vec3_length(delta);
    if (dist < 1e-8f) return;

    /* Hooke's law plus damping along the spring axis. */
    float force = k * (dist - rest_len);
    float rel_v = vec3_dot(vec3_sub(b->vel, a->vel), delta) / dist;
    force += damping * rel_v;

    Vec3 impulse = vec3_scale(delta, force / dist);
    a->vel = vec3_add(a->vel, vec3_scale(impulse, dt / a->mass));
    b->vel = vec3_sub(b->vel, vec3_scale(impulse, dt / b->mass));
}

void physics_collide_spheres(Particle *a, Particle *b, float radius) {
    if (!a || !b) return;

    Vec3  delta = vec3_sub(b->pos, a->pos);
    float dist2 = vec3_dot(delta, delta);
    float min_dist = 2.0f * radius;
    if (dist2 >= min_dist * min_dist || dist2 < 1e-12f) return;

    float dist = sqrtf(dist2);
    Vec3  normal = vec3_scale(delta, 1.0f / dist);

    /* Push apart equally */
    Vec3 half_push = vec3_scale(normal, (min_dist - dist) * 0.5f);
    a->pos = vec3_sub(a->pos, half_push);
    b->pos = vec3_add(b->pos, half_push);

    /* Elastic velocity exchange along the collision normal */
    float va = vec3_dot(a->vel, normal);
    float vb = vec3_dot(b->vel, normal);
    a->vel = vec3_add(a->vel, vec3_scale(normal, vb - va));
    b->vel = vec3_add(b->vel, vec3_scale(normal, va - vb));
}

void physics_confine_box(Particle *p, Vec3 bounds, float restitution) {
    if (!p) return;
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
