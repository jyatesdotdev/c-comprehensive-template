/**
 * @file numerical.c
 * @brief Numerical methods: integration, root-finding, and ODE solvers.
 */
#include "simulation/numerical.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

double numerical_integrate_simpson(double (*f)(double), double a, double b, int n) {
    if (n <= 0 || n % 2 != 0) return 0.0;
    double h = (b - a) / n;
    double sum = f(a) + f(b);
    for (int i = 1; i < n; i++) sum += f(a + i * h) * (i % 2 == 0 ? 2.0 : 4.0);
    return sum * h / 3.0;
}

double numerical_bisect(double (*f)(double), double a, double b, double tol, int *iters) {
    int    count = 0;
    double mid;
    while ((b - a) > tol) {
        mid = (a + b) / 2.0;
        if (f(mid) * f(a) < 0.0) b = mid;
        else a = mid;
        count++;
    }
    if (iters) *iters = count;
    return (a + b) / 2.0;
}

double numerical_newton(double (*f)(double), double (*df)(double), double x0, double tol,
                        int max_iter, int *iters) {
    double x = x0;
    int    i;
    for (i = 0; i < max_iter; i++) {
        double fx = f(x);
        double dfx = df(x);
        if (fabs(dfx) < 1e-15) break;
        double x_new = x - fx / dfx;
        if (fabs(x_new - x) < tol) {
            x = x_new;
            i++;
            break;
        }
        x = x_new;
    }
    if (iters) *iters = i;
    return x;
}

ErrorCode numerical_rk4_step(OdeFunc f, double t, double *y, int n, double dt, void *ctx) {
    if (!f || !y || n <= 0) return ERR_INVALID_ARG;

    double *k1 = malloc(4 * (size_t)n * sizeof(double));
    if (!k1) return ERR_NOMEM;
    double *k2 = k1 + n, *k3 = k2 + n, *k4 = k3 + n;
    double *tmp = malloc((size_t)n * sizeof(double));
    if (!tmp) {
        free(k1);
        return ERR_NOMEM;
    }

    f(t, y, k1, ctx);
    for (int i = 0; i < n; i++) tmp[i] = y[i] + 0.5 * dt * k1[i];
    f(t + 0.5 * dt, tmp, k2, ctx);
    for (int i = 0; i < n; i++) tmp[i] = y[i] + 0.5 * dt * k2[i];
    f(t + 0.5 * dt, tmp, k3, ctx);
    for (int i = 0; i < n; i++) tmp[i] = y[i] + dt * k3[i];
    f(t + dt, tmp, k4, ctx);

    for (int i = 0; i < n; i++) y[i] += dt / 6.0 * (k1[i] + 2.0 * k2[i] + 2.0 * k3[i] + k4[i]);

    free(tmp);
    free(k1);
    return ERR_OK;
}

ErrorCode numerical_rk4_integrate(OdeFunc f, double t0, double t1, double *y, int n, double dt,
                                  void *ctx) {
    if (!f || !y || n <= 0 || dt <= 0.0) return ERR_INVALID_ARG;

    double t = t0;
    while (t < t1 - dt * 0.5) {
        double    step = (t + dt > t1) ? t1 - t : dt;
        ErrorCode err = numerical_rk4_step(f, t, y, n, step, ctx);
        if (err) return err; /* stop at the failing step; y holds the last good state */
        t += step;
    }
    return ERR_OK;
}
