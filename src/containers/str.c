/**
 * @file str.c
 * @brief String view implementation.
 */
#include "containers/str.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

StrView sv_from(const char *s) {
    return (StrView){s, s ? strlen(s) : 0};
}

StrView sv_from_n(const char *s, size_t len) {
    return (StrView){s, s ? len : 0};
}

StrView sv_slice(StrView sv, size_t start, size_t end) {
    if (start > sv.len) start = sv.len;
    if (end > sv.len) end = sv.len;
    if (start > end) start = end;
    return (StrView){sv.data + start, end - start};
}

bool sv_eq(StrView a, StrView b) {
    if (a.len != b.len) return false;
    return a.len == 0 || memcmp(a.data, b.data, a.len) == 0;
}

bool sv_eq_cstr(StrView sv, const char *s) {
    return s && sv_eq(sv, sv_from(s));
}

bool sv_starts_with(StrView sv, const char *prefix) {
    if (!prefix) return false;
    size_t n = strlen(prefix);
    return n <= sv.len && (n == 0 || memcmp(sv.data, prefix, n) == 0);
}

bool sv_ends_with(StrView sv, const char *suffix) {
    if (!suffix) return false;
    size_t n = strlen(suffix);
    return n <= sv.len && (n == 0 || memcmp(sv.data + sv.len - n, suffix, n) == 0);
}

StrView sv_trim(StrView sv) {
    while (sv.len > 0 && isspace((unsigned char)sv.data[0])) {
        sv.data++;
        sv.len--;
    }
    while (sv.len > 0 && isspace((unsigned char)sv.data[sv.len - 1])) sv.len--;
    return sv;
}

bool sv_split_next(StrView *rest, char delim, StrView *out_token) {
    if (!rest || !out_token || !rest->data) return false;

    const char *sep = memchr(rest->data, delim, rest->len);
    if (sep) {
        size_t tok_len = (size_t)(sep - rest->data);
        *out_token = (StrView){rest->data, tok_len};
        rest->data = sep + 1;
        rest->len -= tok_len + 1;
    } else {
        *out_token = *rest;
        rest->data = NULL; /* exhausted — next call returns false */
        rest->len = 0;
    }
    return true;
}

ErrorCode sv_to_cstr(StrView sv, char *buf, size_t cap) {
    if (!buf || (sv.len > 0 && !sv.data)) return ERR_INVALID_ARG;
    if (cap < sv.len + 1) return ERR_OVERFLOW;
    if (sv.len > 0) memcpy(buf, sv.data, sv.len);
    buf[sv.len] = '\0';
    return ERR_OK;
}

ErrorCode sv_parse_long(StrView sv, long *out) {
    if (!out) return ERR_INVALID_ARG;

    /* Longest possible long is well under 24 chars including sign. */
    char buf[24];
    if (sv.len == 0 || sv.len >= sizeof(buf)) {
        return sv.len == 0 ? ERR_INVALID_ARG : ERR_OVERFLOW;
    }
    ErrorCode err = sv_to_cstr(sv, buf, sizeof(buf));
    if (err) return err;

    errno = 0;
    char *end = NULL;
    long  v = strtol(buf, &end, 10);
    if (errno == ERANGE) return ERR_OVERFLOW;
    if (end == buf || *end != '\0') return ERR_INVALID_ARG; /* junk or partial */
    *out = v;
    return ERR_OK;
}
