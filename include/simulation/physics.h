/**
 * @file physics.h
 * @brief Physics simulation primitives: Euler, Verlet, spring forces, collisions.
 */
#ifndef SIMULATION_PHYSICS_H
#define SIMULATION_PHYSICS_H

#include <stddef.h>

/** @brief 3D vector. */
typedef struct { float x, y, z; } Vec3;

/** @brief Particle with position, velocity, and mass. */
typedef struct {
    Vec3  pos;
    Vec3  vel;
    Vec3  prev_pos; /**< For Verlet integration. */
    float mass;
} Particle;

/**
 * @brief Step particles forward by dt using Euler integration.
 * @param particles Array of particles to update.
 * @param count     Number of particles.
 * @param dt        Time step.
 * @param gravity   Gravitational acceleration vector.
 */
void physics_step_euler(Particle *particles, size_t count, float dt, Vec3 gravity);

/**
 * @brief Step particles forward using Störmer-Verlet integration (more stable).
 * @param particles Array of particles to update.
 * @param count     Number of particles.
 * @param dt        Time step.
 * @param gravity   Gravitational acceleration vector.
 */
void physics_step_verlet(Particle *particles, size_t count, float dt, Vec3 gravity);

/**
 * @brief Apply spring force between two particles (Hooke's law + damping).
 * @param a        First particle.
 * @param b        Second particle.
 * @param rest_len Rest length of the spring.
 * @param k        Spring constant.
 * @param damping  Damping coefficient.
 * @param dt       Time step.
 */
void physics_apply_spring(Particle *a, Particle *b, float rest_len, float k, float damping, float dt);

/**
 * @brief Resolve sphere-sphere collision between two particles.
 * @param a      First particle.
 * @param b      Second particle.
 * @param radius Collision radius for both particles.
 */
void physics_collide_spheres(Particle *a, Particle *b, float radius);

/**
 * @brief Confine particle within axis-aligned box [0, bounds].
 * @param p           Particle to confine.
 * @param bounds      Box dimensions (max x, y, z).
 * @param restitution Coefficient of restitution (bounciness, 0–1).
 */
void physics_confine_box(Particle *p, Vec3 bounds, float restitution);

#endif /* SIMULATION_PHYSICS_H */
