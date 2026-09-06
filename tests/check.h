/**
 * @file check.h
 * @brief Test assertion macros that survive NDEBUG (unlike assert()).
 */
#ifndef TESTS_CHECK_H
#define TESTS_CHECK_H

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

#endif /* TESTS_CHECK_H */
