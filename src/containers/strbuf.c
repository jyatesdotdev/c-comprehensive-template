/**
 * @file strbuf.c
 * @brief String builder implementation.
 */
#include "containers/strbuf.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STRBUF_INITIAL_CAP 32

ErrorCode strbuf_init(StrBuf *sb) {
    if (!sb) return ERR_INVALID_ARG;
    sb->data = malloc(STRBUF_INITIAL_CAP);
    if (!sb->data) return ERR_NOMEM;
    sb->data[0] = '\0';
    sb->len = 0;
    sb->cap = STRBUF_INITIAL_CAP;
    return ERR_OK;
}

void strbuf_destroy(StrBuf *sb) {
    if (sb) {
        free(sb->data);
        sb->data = NULL;
        sb->len = 0;
        sb->cap = 0;
    }
}

/** Ensure room for extra more bytes plus the NUL. */
static ErrorCode ensure(StrBuf *sb, size_t extra) {
    if (extra > SIZE_MAX - sb->len - 1) return ERR_OVERFLOW;
    size_t need = sb->len + extra + 1;
    if (need <= sb->cap) return ERR_OK;

    size_t new_cap = sb->cap;
    while (new_cap < need) {
        if (new_cap > SIZE_MAX / 2) return ERR_OVERFLOW;
        new_cap *= 2;
    }
    char *tmp = realloc(sb->data, new_cap);
    if (!tmp) return ERR_NOMEM;
    sb->data = tmp;
    sb->cap = new_cap;
    return ERR_OK;
}

ErrorCode strbuf_append_n(StrBuf *sb, const char *s, size_t n) {
    if (!sb || !sb->data || (!s && n > 0)) return ERR_INVALID_ARG;
    ErrorCode err = ensure(sb, n);
    if (err) return err;
    memcpy(sb->data + sb->len, s, n);
    sb->len += n;
    sb->data[sb->len] = '\0';
    return ERR_OK;
}

ErrorCode strbuf_append(StrBuf *sb, const char *s) {
    if (!s) return ERR_INVALID_ARG;
    return strbuf_append_n(sb, s, strlen(s));
}

ErrorCode strbuf_append_char(StrBuf *sb, char c) {
    return strbuf_append_n(sb, &c, 1);
}

ErrorCode strbuf_appendf(StrBuf *sb, const char *fmt, ...) {
    if (!sb || !sb->data || !fmt) return ERR_INVALID_ARG;

    va_list ap;
    va_start(ap, fmt);
    va_list ap2;
    va_copy(ap2, ap);

    /* First pass sizes the output, second writes it in place. */
    int needed = vsnprintf(NULL, 0, fmt, ap); /* Flawfinder: ignore — trusted caller */
    va_end(ap);
    if (needed < 0) {
        va_end(ap2);
        return ERR_INVALID_ARG;
    }

    ErrorCode err = ensure(sb, (size_t)needed);
    if (!err) {
        vsnprintf(sb->data + sb->len, (size_t)needed + 1, fmt, ap2); /* Flawfinder: ignore */
        sb->len += (size_t)needed;
    }
    va_end(ap2);
    return err;
}

const char *strbuf_cstr(const StrBuf *sb) {
    return (sb && sb->data) ? sb->data : "";
}

void strbuf_clear(StrBuf *sb) {
    if (sb && sb->data) {
        sb->len = 0;
        sb->data[0] = '\0';
    }
}

char *strbuf_take(StrBuf *sb) {
    if (!sb || !sb->data) return NULL;
    char *out = sb->data;
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
    /* Re-init so the builder stays usable; on failure it's left destroyed. */
    if (strbuf_init(sb) != ERR_OK) sb->data = NULL;
    return out;
}
