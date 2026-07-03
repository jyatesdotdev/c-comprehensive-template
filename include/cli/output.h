/**
 * @file output.h
 * @brief CLI output formatting: ANSI colors, table printing, progress bars.
 */
#ifndef CLI_OUTPUT_H
#define CLI_OUTPUT_H

#include <stdbool.h>
#include <stdio.h>

/* ── ANSI Color Codes ───────────────────────────────────────────────────── */

#define CLR_RESET   "\033[0m"
#define CLR_BOLD    "\033[1m"
#define CLR_DIM     "\033[2m"
#define CLR_RED     "\033[31m"
#define CLR_GREEN   "\033[32m"
#define CLR_YELLOW  "\033[33m"
#define CLR_BLUE    "\033[34m"
#define CLR_MAGENTA "\033[35m"
#define CLR_CYAN    "\033[36m"

/**
 * @brief Check if color output should be used (isatty + NO_COLOR env).
 * @param stream Output stream to check.
 * @return true if colors should be used, false otherwise.
 */
bool cli_colors_enabled(FILE *stream);

/** Marks a function as printf-style so compilers type-check its format arguments. */
#if defined(__GNUC__) || defined(__clang__)
#define CLI_PRINTF_FORMAT(fmt_idx, arg_idx) __attribute__((format(printf, fmt_idx, arg_idx)))
#else
#define CLI_PRINTF_FORMAT(fmt_idx, arg_idx)
#endif

/**
 * @brief Print colored text to stream. No-op color if colors disabled.
 * @param stream Output stream.
 * @param color  ANSI color code string (e.g. CLR_RED).
 * @param fmt    printf-style format string.
 * @param ...    Format arguments.
 */
void cli_color_fprintf(FILE *stream, const char *color, const char *fmt, ...)
    CLI_PRINTF_FORMAT(3, 4);

/* ── Table Formatting ───────────────────────────────────────────────────── */

#define CLI_TABLE_MAX_COLS 16

/** @brief Table state for formatted column output. */
typedef struct {
    int   num_cols;
    int   widths[CLI_TABLE_MAX_COLS];      /**< Column widths. */
    char  headers[CLI_TABLE_MAX_COLS][64]; /**< Column header labels. */
    FILE *out;
} CliTable;

/**
 * @brief Initialize a table.
 * @param t        Table to initialize.
 * @param out      Output stream.
 * @param num_cols Number of columns.
 * @param widths   Array of column widths (num_cols entries).
 * @param headers  Array of column header strings (num_cols entries).
 */
void cli_table_init(CliTable *t, FILE *out, int num_cols, const int *widths, const char **headers);

/**
 * @brief Print the header row with separator line.
 * @param t Table to print header for.
 */
void cli_table_header(const CliTable *t);

/**
 * @brief Print one data row.
 * @param t      Table context.
 * @param values Array of strings (num_cols entries).
 */
void cli_table_row(const CliTable *t, const char **values);

/**
 * @brief Print a separator line.
 * @param t Table context.
 */
void cli_table_separator(const CliTable *t);

/* ── Progress Bar ───────────────────────────────────────────────────────── */

/**
 * @brief Render a progress bar on the current line (uses \\r).
 * @param current  Current progress value.
 * @param total    Total value (100%).
 * @param width    Bar width in characters (excluding brackets/percentage).
 * @param stream   Output stream (typically stderr).
 */
void cli_progress(int current, int total, int width, FILE *stream);

/**
 * @brief Finish progress bar output (prints newline).
 * @param stream Output stream.
 */
void cli_progress_done(FILE *stream);

#endif /* CLI_OUTPUT_H */
