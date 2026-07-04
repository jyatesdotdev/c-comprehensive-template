/**
 * @file strbuf.h
 * @brief Growable string builder — always NUL-terminated.
 */
#ifndef CONTAINERS_STRBUF_H
#define CONTAINERS_STRBUF_H

#include "core/error.h"
#include <stddef.h>

/** Marks a function as printf-style so compilers type-check its format arguments. */
#if defined(__GNUC__) || defined(__clang__)
#define STRBUF_PRINTF_FORMAT(fmt_idx, arg_idx) __attribute__((format(printf, fmt_idx, arg_idx)))
#else
#define STRBUF_PRINTF_FORMAT(fmt_idx, arg_idx)
#endif

/** @brief String builder. Initialize with strbuf_init, free with strbuf_destroy. */
typedef struct StrBuf {
    char  *data; /**< NUL-terminated contents (never NULL after init). */
    size_t len;  /**< Length excluding the NUL. */
    size_t cap;  /**< Allocated bytes including NUL space. */
} StrBuf;

/** @brief Initialize as the empty string. */
ErrorCode strbuf_init(StrBuf *sb);

/** @brief Free storage. Safe on NULL/already-destroyed. */
void strbuf_destroy(StrBuf *sb);

/** @brief Append a NUL-terminated string. */
ErrorCode strbuf_append(StrBuf *sb, const char *s);

/** @brief Append exactly n bytes of s (need not be NUL-terminated). */
ErrorCode strbuf_append_n(StrBuf *sb, const char *s, size_t n);

/** @brief Append a single character. */
ErrorCode strbuf_append_char(StrBuf *sb, char c);

/** @brief Append printf-formatted text. */
ErrorCode strbuf_appendf(StrBuf *sb, const char *fmt, ...) STRBUF_PRINTF_FORMAT(2, 3);

/** @brief Contents as a C string ("" when empty; owned by the builder). */
const char *strbuf_cstr(const StrBuf *sb);

/** @brief Reset to the empty string without releasing storage. */
void strbuf_clear(StrBuf *sb);

/**
 * @brief Take ownership of the buffer: the builder resets to empty and the
 *        returned string must be free()d by the caller. NULL on empty builder
 *        allocation failure.
 */
char *strbuf_take(StrBuf *sb);

#endif /* CONTAINERS_STRBUF_H */
