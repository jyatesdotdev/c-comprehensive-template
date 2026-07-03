/**
 * @file perf_test.h
 * @brief Minimal header-only performance testing utility.
 *
 * Usage:
 *   PERF_BEGIN(label);
 *   // ... code to benchmark ...
 *   PERF_END(label);           // prints elapsed time
 *   PERF_BENCH(label, iters, { code; });  // runs N iterations, prints stats
 */
#ifndef PERF_TEST_H
#define PERF_TEST_H

#include <stdio.h>
#include <time.h>
#include <float.h>

/** @brief Timer state for performance measurement. */
typedef struct {
    const char     *label;
    struct timespec start;
} PerfTimer;

/**
 * @brief Start a performance timer.
 * @param t     Timer to initialize.
 * @param label Descriptive label for output.
 */
static inline void perf_start(PerfTimer *t, const char *label) {
    t->label = label;
    clock_gettime(CLOCK_MONOTONIC, &t->start);
}

/**
 * @brief Return elapsed time in milliseconds since perf_start().
 * @param t Timer started with perf_start().
 * @return Elapsed time in milliseconds.
 */
static inline double perf_elapsed_ms(const PerfTimer *t) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double)(now.tv_sec - t->start.tv_sec) * 1e3 +
           (double)(now.tv_nsec - t->start.tv_nsec) * 1e-6;
}

/**
 * @brief Stop the timer and print elapsed time to stdout.
 * @param t Timer started with perf_start().
 */
static inline void perf_stop(const PerfTimer *t) {
    double ms = perf_elapsed_ms(t);
    printf("[PERF] %-30s %10.3f ms\n", t->label, ms);
}

/** @brief Start a named performance timer. */
#define PERF_BEGIN(label)    \
    PerfTimer _perf_##label; \
    perf_start(&_perf_##label, #label)

/** @brief Stop a named performance timer and print elapsed time. */
#define PERF_END(label) perf_stop(&_perf_##label)

/**
 * @brief Run a block `iters` times, report min/avg/max.
 *
 * Usage: PERF_BENCH("my_func", 1000, { my_func(); });
 */
#define PERF_BENCH(label, iters, block)                                                         \
    do {                                                                                        \
        double _min = DBL_MAX, _max = 0, _sum = 0;                                              \
        for (int _i = 0; _i < (iters); _i++) {                                                  \
            PerfTimer _t;                                                                       \
            perf_start(&_t, label);                                                             \
            block double _ms = perf_elapsed_ms(&_t);                                            \
            if (_ms < _min) _min = _ms;                                                         \
            if (_ms > _max) _max = _ms;                                                         \
            _sum += _ms;                                                                        \
        }                                                                                       \
        printf("[PERF] %-30s iters=%-6d min=%.3f avg=%.3f max=%.3f ms\n", label, (iters), _min, \
               _sum / (iters), _max);                                                           \
    } while (0)

#endif /* PERF_TEST_H */
