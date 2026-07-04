/**
 * @file time.c
 * @brief Monotonic clock utilities (POSIX implementation).
 */
#include "core/time.h"

#include <errno.h>
#include <time.h>

uint64_t time_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

double time_now_ms(void) {
    return (double)time_now_ns() / 1.0e6;
}

void time_sleep_ms(unsigned int ms) {
    struct timespec req = {
        .tv_sec = ms / 1000u,
        .tv_nsec = (long)(ms % 1000u) * 1000000L,
    };
    /* nanosleep writes the remaining time on EINTR — resume until done. */
    while (nanosleep(&req, &req) != 0 && errno == EINTR) {}
}

void stopwatch_start(Stopwatch *sw) {
    if (sw) sw->start_ns = time_now_ns();
}

double stopwatch_elapsed_ms(const Stopwatch *sw) {
    if (!sw) return 0.0;
    return (double)(time_now_ns() - sw->start_ns) / 1.0e6;
}
