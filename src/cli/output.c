/**
 * @file output.c
 * @brief CLI output utilities: color printing, table formatting, progress bar.
 */
#include "cli/output.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ── ANSI Colors ────────────────────────────────────────────────────────── */

bool cli_colors_enabled(FILE *stream) {
    /* NOLINTNEXTLINE(concurrency-mt-unsafe) — read-only env access; unsafe only alongside setenv from another thread */
    if (getenv("NO_COLOR")) return false;
    return isatty(fileno(stream));
}

void cli_color_fprintf(FILE *stream, const char *color, const char *fmt, ...) {
    bool use_color = cli_colors_enabled(stream);
    if (use_color) fputs(color, stream);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stream, fmt, ap); /* Flawfinder: ignore — format from trusted caller */
    va_end(ap);

    if (use_color) fputs(CLR_RESET, stream);
}

/* ── Table Formatting ───────────────────────────────────────────────────── */

void cli_table_init(CliTable *t, FILE *out, int num_cols, const int *widths, const char **headers) {
    memset(t, 0, sizeof(*t));
    t->out = out;
    t->num_cols = num_cols > CLI_TABLE_MAX_COLS ? CLI_TABLE_MAX_COLS : num_cols;
    for (int i = 0; i < t->num_cols; i++) {
        t->widths[i] = widths[i];
        snprintf(t->headers[i], sizeof(t->headers[i]), "%s", headers[i] ? headers[i] : "");
    }
}

void cli_table_separator(const CliTable *t) {
    for (int i = 0; i < t->num_cols; i++) {
        if (i > 0) fputs("-+-", t->out);
        for (int j = 0; j < t->widths[i]; j++) fputc('-', t->out);
    }
    fputc('\n', t->out);
}

void cli_table_header(const CliTable *t) {
    for (int i = 0; i < t->num_cols; i++) {
        if (i > 0) fputs(" | ", t->out);
        fprintf(t->out, "%-*s", t->widths[i], t->headers[i]);
    }
    fputc('\n', t->out);
    cli_table_separator(t);
}

void cli_table_row(const CliTable *t, const char **values) {
    for (int i = 0; i < t->num_cols; i++) {
        if (i > 0) fputs(" | ", t->out);
        fprintf(t->out, "%-*s", t->widths[i], values[i] ? values[i] : "");
    }
    fputc('\n', t->out);
}

/* ── Progress Bar ───────────────────────────────────────────────────────── */

void cli_progress(int current, int total, int width, FILE *stream) {
    if (total <= 0) return;
    int pct = (current * 100) / total;
    if (pct > 100) pct = 100;
    int filled = (pct * width) / 100;

    fputc('\r', stream);
    fputc('[', stream);
    for (int i = 0; i < width; i++) {
        if (i < filled) fputc('=', stream);
        else if (i == filled) fputc('>', stream);
        else fputc(' ', stream);
    }
    fprintf(stream, "] %3d%%", pct);
    fflush(stream);
}

void cli_progress_done(FILE *stream) {
    fputc('\n', stream);
    fflush(stream);
}
