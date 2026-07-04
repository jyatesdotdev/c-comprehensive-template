/**
 * @file time.h
 * @brief Monotonic clocks, stopwatch, and sleep (POSIX).
 *
 * Monotonic time is unaffected by wall-clock adjustments — always use it
 * for measuring durations. For repeated benchmark harnesses see
 * testing/perf_test.h, which builds on the same clock.
 */
#ifndef CORE_TIME_H
#define CORE_TIME_H

#include <stdint.h>

/** @brief Monotonic time in nanoseconds (epoch is arbitrary; compare only). */
uint64_t time_now_ns(void);

/** @brief Monotonic time in milliseconds (epoch is arbitrary; compare only). */
double time_now_ms(void);

/** @brief Sleep for at least ms milliseconds (resumes after signals). */
void time_sleep_ms(unsigned int ms);

/** @brief Simple elapsed-time measurement around a code region. */
typedef struct Stopwatch {
    uint64_t start_ns; /**< Monotonic timestamp at start. */
} Stopwatch;

/** @brief Start (or restart) a stopwatch. */
void stopwatch_start(Stopwatch *sw);

/** @brief Milliseconds elapsed since stopwatch_start. */
double stopwatch_elapsed_ms(const Stopwatch *sw);

#endif /* CORE_TIME_H */
