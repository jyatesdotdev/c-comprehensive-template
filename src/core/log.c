/**
 * @file log.c
 * @brief Leveled logging implementation.
 */
#include "core/log.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define LOG_LINE_MAX 1024

static LogLevel g_min_level = LOG_LEVEL_INFO;
static FILE    *g_stream = NULL; /* NULL means stderr (can't init statically, ISO C) */

static const char *level_name(LogLevel l) {
    switch (l) {
    case LOG_LEVEL_DEBUG: return "DEBUG";
    case LOG_LEVEL_INFO: return "INFO ";
    case LOG_LEVEL_WARN: return "WARN ";
    case LOG_LEVEL_ERROR: return "ERROR";
    default: return "?????";
    }
}

static const char *level_color(LogLevel l) {
    switch (l) {
    case LOG_LEVEL_DEBUG: return "\033[2m";  /* dim */
    case LOG_LEVEL_INFO: return "\033[32m";  /* green */
    case LOG_LEVEL_WARN: return "\033[33m";  /* yellow */
    case LOG_LEVEL_ERROR: return "\033[31m"; /* red */
    default: return "";
    }
}

void log_set_level(LogLevel min_level) {
    g_min_level = min_level;
}

LogLevel log_get_level(void) {
    return g_min_level;
}

void log_set_stream(FILE *stream) {
    g_stream = stream;
}

void log_msg(LogLevel level, const char *fmt, ...) {
    if (level < g_min_level || !fmt) return;
    FILE *out = g_stream ? g_stream : stderr;

    /* Wall-clock timestamp with millisecond precision. */
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tm_buf;
    localtime_r(&ts.tv_sec, &tm_buf);

    char    msg[LOG_LINE_MAX];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap); /* Flawfinder: ignore — format from trusted caller */
    va_end(ap);

    /* NOLINTNEXTLINE(concurrency-mt-unsafe) — read-only env access */
    int color = isatty(fileno(out)) && !getenv("NO_COLOR");

    /* One fprintf call per line keeps concurrent writers from interleaving. */
    fprintf(out, "%02d:%02d:%02d.%03ld %s%s%s %s\n", tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec,
            ts.tv_nsec / 1000000L, color ? level_color(level) : "", level_name(level),
            color ? "\033[0m" : "", msg);
}
