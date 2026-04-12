/**
 * @file simulation_demo.c
 * @brief Demonstrates physics simulation and numerical methods.
 *
 * Shows: Verlet particle sim with collisions, RK4 ODE solving (pendulum),
 * Simpson integration, Newton-Raphson root finding.
 */
#include "simulation/physics.h"
#include "simulation/numerical.h"
#include <stdio.h>
#include <math.h>

/* --- Numerical demos --- */

static double f_sin(double x) { return sin(x); }
static double f_poly(double x) { return x * x * x - x - 2.0; }
static double df_poly(double x) { return 3.0 * x * x - 1.0; }

/* Pendulum ODE: y[0]=theta, y[1]=omega; dtheta/dt=omega, domega/dt=-(g/L)*sin(theta) */
static void pendulum_ode(double t, const double *y, double *dydt, void *ctx) {
    (void)t; (void)ctx;
    double g_over_L = 9.81 / 1.0; /* g=9.81, L=1m */
    dydt[0] = y[1];
    dydt[1] = -g_over_L * sin(y[0]);
}

int main(void) {
    /* 1. Simpson integration: integral of sin(x) from 0 to pi = 2.0 */
    double result = numerical_integrate_simpson(f_sin, 0.0, 3.14159265358979, 100);
    printf("=== Simpson Integration ===\n");
    printf("  integral(sin, 0, pi) = %.10f  (exact: 2.0)\n\n", result);

    /* 2. Root finding: x^3 - x - 2 = 0 */
    int iters;
    double root_b = numerical_bisect(f_poly, 1.0, 2.0, 1e-10, &iters);
    printf("=== Root Finding ===\n");
    printf("  Bisection:       x = %.10f  (%d iters)\n", root_b, iters);
    double root_n = numerical_newton(f_poly, df_poly, 1.5, 1e-10, 50, &iters);
    printf("  Newton-Raphson:  x = %.10f  (%d iters)\n\n", root_n, iters);

    /* 3. RK4 pendulum: theta0=0.5 rad, omega0=0, integrate 2 seconds */
    printf("=== RK4 Pendulum ===\n");
    double y[2] = {0.5, 0.0}; /* theta, omega */
    double t = 0.0, dt = 0.01;
    for (int step = 0; step <= 200; step++) {
        if (step % 50 == 0)
            printf("  t=%.2f  theta=%.6f  omega=%.6f\n", t, y[0], y[1]);
        numerical_rk4_step(pendulum_ode, t, y, 2, dt, NULL);
        t += dt;
    }

    /* 4. Verlet particle simulation with box confinement */
    printf("\n=== Verlet Particle Sim ===\n");
    Particle parts[2] = {
        { .pos={1,5,0}, .vel={2,0,0}, .prev_pos={1,5,0}, .mass=1.0f },
        { .pos={4,5,0}, .vel={-1,0,0}, .prev_pos={4,5,0}, .mass=1.0f },
    };
    Vec3 gravity = {0, -9.81f, 0};
    Vec3 bounds = {10, 10, 10};
    float pdt = 0.016f;

    for (int i = 0; i <= 60; i++) {
        if (i % 20 == 0) {
            printf("  step %3d: p0=(%.2f,%.2f) p1=(%.2f,%.2f)\n", i,
                   (double)parts[0].pos.x, (double)parts[0].pos.y,
                   (double)parts[1].pos.x, (double)parts[1].pos.y);
        }
        physics_step_verlet(parts, 2, pdt, gravity);
        physics_collide_spheres(&parts[0], &parts[1], 0.5f);
        for (int j = 0; j < 2; j++)
            physics_confine_box(&parts[j], bounds, 0.8f);
    }
    printf("  step  60: p0=(%.2f,%.2f) p1=(%.2f,%.2f)\n",
           (double)parts[0].pos.x, (double)parts[0].pos.y,
           (double)parts[1].pos.x, (double)parts[1].pos.y);

    return 0;
}
