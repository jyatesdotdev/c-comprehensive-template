/** @file test_simulation.c — Tests for physics and numerical modules. */
#include "simulation/physics.h"
#include "simulation/numerical.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>

#define ASSERT_NEAR(a, b, tol) \
    do { if (fabs((double)(a) - (double)(b)) > (tol)) { \
        fprintf(stderr, "FAIL %s:%d: %.10f != %.10f (tol %.1e)\n", __FILE__, __LINE__, (double)(a), (double)(b), (tol)); \
        return 1; } } while(0)

static double f_x2(double x) { return x * x; }
static double f_poly(double x) { return x * x * x - x - 2.0; }
static double df_poly(double x) { return 3.0 * x * x - 1.0; }

/* Simple harmonic oscillator: y''=-y => y[0]=pos, y[1]=vel */
static void sho_ode(double t, const double *y, double *dydt, void *ctx) {
    (void)t; (void)ctx;
    dydt[0] = y[1];
    dydt[1] = -y[0];
}

int main(void) {
    int pass = 0;

    /* Simpson: integral(x^2, 0, 1) = 1/3 */
    ASSERT_NEAR(numerical_integrate_simpson(f_x2, 0.0, 1.0, 100), 1.0/3.0, 1e-10);
    printf("PASS: Simpson integration\n"); pass++;

    /* Bisection */
    int iters;
    double root = numerical_bisect(f_poly, 1.0, 2.0, 1e-12, &iters);
    ASSERT_NEAR(f_poly(root), 0.0, 1e-6);
    printf("PASS: Bisection root finding (%d iters)\n", iters); pass++;

    /* Newton-Raphson */
    root = numerical_newton(f_poly, df_poly, 1.5, 1e-12, 50, &iters);
    ASSERT_NEAR(f_poly(root), 0.0, 1e-10);
    assert(iters < 15); /* Newton should converge fast */
    printf("PASS: Newton-Raphson (%d iters)\n", iters); pass++;

    /* RK4: SHO with y(0)=1, y'(0)=0 => y(t)=cos(t). Check at t=2*pi. */
    double y[2] = {1.0, 0.0};
    numerical_rk4_integrate(sho_ode, 0.0, 2.0 * 3.14159265358979, y, 2, 0.001, NULL);
    ASSERT_NEAR(y[0], 1.0, 1e-3);
    ASSERT_NEAR(y[1], 0.0, 1e-3);
    printf("PASS: RK4 SHO full period\n"); pass++;

    /* Euler physics step */
    Particle p = { .pos={0,0,0}, .vel={1,0,0}, .mass=1.0f };
    Vec3 g = {0, -10, 0};
    physics_step_euler(&p, 1, 1.0f, g);
    ASSERT_NEAR(p.pos.x, 1.0f, 1e-5);
    ASSERT_NEAR(p.vel.y, -10.0f, 1e-5);
    printf("PASS: Euler integration\n"); pass++;

    /* Verlet physics step */
    Particle v = { .pos={0,0,0}, .vel={0,0,0}, .prev_pos={0,0,0}, .mass=1.0f };
    Vec3 g2 = {0, -10, 0};
    physics_step_verlet(&v, 1, 1.0f, g2);
    ASSERT_NEAR(v.pos.y, -10.0f, 1e-3);
    printf("PASS: Verlet integration\n"); pass++;

    /* Box confinement */
    Particle bp = { .pos={-1,5,0}, .vel={-5,0,0}, .mass=1.0f };
    Vec3 bounds = {10, 10, 10};
    physics_confine_box(&bp, bounds, 0.5f);
    assert(bp.pos.x >= 0.0f);
    assert(bp.vel.x > 0.0f); /* reflected */
    printf("PASS: Box confinement\n"); pass++;

    /* Sphere collision */
    Particle a = { .pos={0,0,0}, .vel={1,0,0}, .mass=1.0f };
    Particle b = { .pos={0.5f,0,0}, .vel={-1,0,0}, .mass=1.0f };
    physics_collide_spheres(&a, &b, 0.5f);
    assert(a.vel.x < 0.0f); /* bounced back */
    assert(b.vel.x > 0.0f);
    printf("PASS: Sphere collision\n"); pass++;

    printf("\nAll %d simulation tests passed.\n", pass);
    return 0;
}
