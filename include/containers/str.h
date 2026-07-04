/**
 * @file str.h
 * @brief Non-owning string views: slicing, trimming, splitting, parsing.
 *
 * A StrView points into memory owned by someone else — it is only valid as
 * long as the underlying buffer is. Views are not NUL-terminated; use
 * sv_to_cstr (or containers/strbuf.h) to materialize a C string.
 */
#ifndef CONTAINERS_STR_H
#define CONTAINERS_STR_H

#include "core/error.h"
#include <stdbool.h>
#include <stddef.h>

/** @brief Non-owning view of len bytes at data (not NUL-terminated). */
typedef struct StrView {
    const char *data; /**< First byte, or NULL for the empty/invalid view. */
    size_t      len;  /**< Byte count. */
} StrView;

/** @brief View over a NUL-terminated string (NULL yields the empty view). */
StrView sv_from(const char *s);

/** @brief View over an explicit byte range. */
StrView sv_from_n(const char *s, size_t len);

/** @brief Sub-view [start, end); indices are clamped to the view's length. */
StrView sv_slice(StrView sv, size_t start, size_t end);

/** @brief Byte-wise equality of two views. */
bool sv_eq(StrView a, StrView b);

/** @brief Compare a view against a NUL-terminated string. */
bool sv_eq_cstr(StrView sv, const char *s);

/** @brief True if the view begins with prefix. */
bool sv_starts_with(StrView sv, const char *prefix);

/** @brief True if the view ends with suffix. */
bool sv_ends_with(StrView sv, const char *suffix);

/** @brief View with leading and trailing ASCII whitespace removed. */
StrView sv_trim(StrView sv);

/**
 * @brief Split iterator: yields the next delim-separated token.
 *
 * Usage:
 * @code
 * StrView rest = sv_from("a,b,,c"), tok;
 * while (sv_split_next(&rest, ',', &tok)) { ... }  // yields "a","b","","c"
 * @endcode
 * @param rest      In/out remainder; starts as the full view.
 * @param delim     Delimiter byte.
 * @param out_token Receives the next token (may be empty).
 * @return true while a token was produced.
 */
bool sv_split_next(StrView *rest, char delim, StrView *out_token);

/**
 * @brief Copy a view into buf as a NUL-terminated string.
 * @return ERR_OK, ERR_INVALID_ARG, or ERR_OVERFLOW if cap < sv.len + 1.
 */
ErrorCode sv_to_cstr(StrView sv, char *buf, size_t cap);

/**
 * @brief Parse the entire view as a base-10 long (optional +/- sign).
 * @return ERR_OK, ERR_INVALID_ARG on empty/malformed input,
 *         ERR_OVERFLOW if out of range.
 */
ErrorCode sv_parse_long(StrView sv, long *out);

#endif /* CONTAINERS_STR_H */
