/**
 * @file numerical.h
 * @brief Numerical methods: integration, root finding, ODE solving.
 */
#ifndef SIMULATION_NUMERICAL_H
#define SIMULATION_NUMERICAL_H

#include <stddef.h>

/**
 * @brief Integrate f(x) over [a,b] using Simpson's rule.
 * @param f Function to integrate.
 * @param a Lower bound of integration.
 * @param b Upper bound of integration.
 * @param n Number of intervals (must be even).
 * @return Approximate integral value.
 */
double numerical_integrate_simpson(double (*f)(double), double a, double b, int n);

/**
 * @brief Find root of f(x)=0 in [a,b] via bisection.
 * @param f     Function whose root is sought.
 * @param a     Lower bound of the bracket.
 * @param b     Upper bound of the bracket.
 * @param tol   Convergence tolerance.
 * @param iters Receives the number of iterations performed.
 * @return Approximate root.
 */
double numerical_bisect(double (*f)(double), double a, double b, double tol, int *iters);

/**
 * @brief Find root of f(x)=0 via Newton-Raphson.
 * @param f        Function whose root is sought.
 * @param df       Derivative of f.
 * @param x0       Initial guess.
 * @param tol      Convergence tolerance.
 * @param max_iter Maximum iterations allowed.
 * @param iters    Receives the number of iterations performed.
 * @return Approximate root.
 */
double numerical_newton(double (*f)(double), double (*df)(double), double x0, double tol,
                        int max_iter, int *iters);

/**
 * @brief Derivative function for ODE systems of the form dy/dt = f(t, y).
 *
 * Caller provides the derivative function and state vector.
 * @param t    Current time.
 * @param y    Current state vector (length n).
 * @param dydt Output derivative vector (length n).
 * @param ctx  User context pointer.
 */
typedef void (*OdeFunc)(double t, const double *y, double *dydt, void *ctx);

/**
 * @brief Advance ODE system by one step using 4th-order Runge-Kutta.
 * @param f   Derivative function.
 * @param t   Current time.
 * @param y   State vector, modified in-place.
 * @param n   Dimension of the system.
 * @param dt  Time step size.
 * @param ctx User context pointer passed to f.
 */
void numerical_rk4_step(OdeFunc f, double t, double *y, int n, double dt, void *ctx);

/**
 * @brief Integrate ODE from t0 to t1 with fixed step dt.
 * @param f   Derivative function.
 * @param t0  Start time.
 * @param t1  End time.
 * @param y   State vector, modified in-place.
 * @param n   Dimension of the system.
 * @param dt  Time step size.
 * @param ctx User context pointer passed to f.
 */
void numerical_rk4_integrate(OdeFunc f, double t0, double t1, double *y, int n, double dt,
                             void *ctx);

#endif /* SIMULATION_NUMERICAL_H */
