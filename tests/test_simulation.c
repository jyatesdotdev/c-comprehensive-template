/**
 * @file test_simulation.c
 * @brief Tests for physics and numerical modules.
 */
#include "simulation/numerical.h"
#include "simulation/physics.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(cond)                                                                    \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

#define CHECK_NEAR(a, b, tol) CHECK(fabs((double)(a) - (double)(b)) <= (tol))

static double f_x2(double x) {
    return x * x;
}
static double f_poly(double x) {
    return x * x * x - x - 2.0;
}
static double df_poly(double x) {
    return 3.0 * x * x - 1.0;
}

/* Simple harmonic oscillator: y''=-y => y[0]=pos, y[1]=vel */
static void sho_ode(double t, const double *y, double *dydt, void *ctx) {
    (void)t;
    (void)ctx;
    dydt[0] = y[1];
    dydt[1] = -y[0];
}

int main(void) {
    /* Simpson: integral(x^2, 0, 1) = 1/3 */
    CHECK_NEAR(numerical_integrate_simpson(f_x2, 0.0, 1.0, 100), 1.0 / 3.0, 1e-10);

    /* Bisection */
    int    iters = 0;
    double root = numerical_bisect(f_poly, 1.0, 2.0, 1e-12, &iters);
    CHECK_NEAR(f_poly(root), 0.0, 1e-6);
    CHECK(iters > 0);

    /* Newton-Raphson */
    root = numerical_newton(f_poly, df_poly, 1.5, 1e-12, 50, &iters);
    CHECK_NEAR(f_poly(root), 0.0, 1e-10);
    CHECK(iters < 15); /* Newton should converge fast */

    /* RK4: SHO with y(0)=1, y'(0)=0 => y(t)=cos(t). Check at t=2*pi. */
    double y[2] = {1.0, 0.0};
    CHECK(numerical_rk4_integrate(sho_ode, 0.0, 2.0 * 3.14159265358979, y, 2, 0.001, NULL) ==
          ERR_OK);
    CHECK_NEAR(y[0], 1.0, 1e-3);
    CHECK_NEAR(y[1], 0.0, 1e-3);

    /* RK4 rejects bad arguments */
    CHECK(numerical_rk4_step(NULL, 0.0, y, 2, 0.1, NULL) == ERR_INVALID_ARG);
    CHECK(numerical_rk4_integrate(sho_ode, 0.0, 1.0, y, 0, 0.1, NULL) == ERR_INVALID_ARG);
    CHECK(numerical_rk4_integrate(sho_ode, 0.0, 1.0, y, 2, 0.0, NULL) == ERR_INVALID_ARG);

    /* Euler physics step */
    Particle p = {.pos = {0, 0, 0}, .vel = {1, 0, 0}, .mass = 1.0f};
    Vec3     g = {0, -10, 0};
    physics_step_euler(&p, 1, 1.0f, g);
    CHECK_NEAR(p.pos.x, 1.0f, 1e-5);
    CHECK_NEAR(p.vel.y, -10.0f, 1e-5);
    physics_step_euler(NULL, 1, 1.0f, g); /* NULL is a safe no-op */

    /* Verlet physics step */
    Particle v = {.pos = {0, 0, 0}, .vel = {0, 0, 0}, .prev_pos = {0, 0, 0}, .mass = 1.0f};
    Vec3     g2 = {0, -10, 0};
    physics_step_verlet(&v, 1, 1.0f, g2);
    CHECK_NEAR(v.pos.y, -10.0f, 1e-3);

    /* Spring pulls two separated particles toward each other */
    Particle sa = {.pos = {0, 0, 0}, .vel = {0, 0, 0}, .mass = 1.0f};
    Particle sb = {.pos = {2, 0, 0}, .vel = {0, 0, 0}, .mass = 1.0f};
    physics_apply_spring(&sa, &sb, 1.0f, 10.0f, 0.0f, 0.1f);
    CHECK(sa.vel.x > 0.0f);
    CHECK(sb.vel.x < 0.0f);

    /* Box confinement */
    Particle bp = {.pos = {-1, 5, 0}, .vel = {-5, 0, 0}, .mass = 1.0f};
    Vec3     bounds = {10, 10, 10};
    physics_confine_box(&bp, bounds, 0.5f);
    CHECK(bp.pos.x >= 0.0f);
    CHECK(bp.vel.x > 0.0f); /* reflected */

    /* Sphere collision */
    Particle a = {.pos = {0, 0, 0}, .vel = {1, 0, 0}, .mass = 1.0f};
    Particle b = {.pos = {0.5f, 0, 0}, .vel = {-1, 0, 0}, .mass = 1.0f};
    physics_collide_spheres(&a, &b, 0.5f);
    CHECK(a.vel.x < 0.0f); /* bounced back */
    CHECK(b.vel.x > 0.0f);

    printf("All simulation tests passed.\n");
    return 0;
}
