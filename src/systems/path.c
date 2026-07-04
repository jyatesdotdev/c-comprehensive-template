/**
 * @file path.c
 * @brief Filesystem path utilities implementation (POSIX).
 */
#include "systems/path.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

ErrorCode path_join(const char *a, const char *b, char *out, size_t cap) {
    if (!a || !b || !out || cap == 0) return ERR_INVALID_ARG;

    if (b[0] == '/' || a[0] == '\0') { /* absolute b wins; empty a is a no-op */
        if (strlen(b) + 1 > cap) return ERR_OVERFLOW;
        memcpy(out, b, strlen(b) + 1);
        return ERR_OK;
    }

    size_t alen = strlen(a);
    int    need_sep = a[alen - 1] != '/';
    size_t total = alen + (size_t)need_sep + strlen(b) + 1;
    if (total > cap) return ERR_OVERFLOW;

    int n = snprintf(out, cap, "%s%s%s", a, need_sep ? "/" : "", b);
    return n > 0 && (size_t)n < cap ? ERR_OK : ERR_OVERFLOW;
}

ErrorCode path_normalize(char *path) {
    if (!path) return ERR_INVALID_ARG;

    int    absolute = path[0] == '/';
    size_t len = strlen(path);

    /* Component stack: starts[i] = offset of component i in the output. */
    size_t out = 0;        /* write cursor */
    size_t comp_start[64]; /* offsets of kept components */
    size_t ncomp = 0;
    size_t leading_dotdot = 0; /* components like "../.." that can't pop */

    size_t i = 0;
    while (i < len) {
        while (i < len && path[i] == '/') i++; /* skip separators */
        size_t start = i;
        while (i < len && path[i] != '/') i++;
        size_t clen = i - start;
        if (clen == 0 || (clen == 1 && path[start] == '.')) continue;

        if (clen == 2 && path[start] == '.' && path[start + 1] == '.') {
            if (ncomp > leading_dotdot) {
                out = comp_start[--ncomp]; /* pop the previous component */
                continue;
            }
            if (absolute) continue; /* "/.." is just "/" */
            leading_dotdot++;       /* relative path keeps leading ".."s */
        }

        if (ncomp < sizeof(comp_start) / sizeof(comp_start[0])) comp_start[ncomp++] = out;
        if (out > 0 || absolute) path[out++] = '/';
        memmove(path + out, path + start, clen);
        out += clen;
    }

    if (out == 0) { path[out++] = absolute ? '/' : '.'; }
    path[out] = '\0';
    return ERR_OK;
}

const char *path_basename(const char *path) {
    if (!path) return "";
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

ErrorCode path_dirname(const char *path, char *out, size_t cap) {
    if (!path || !out || cap == 0) return ERR_INVALID_ARG;

    const char *slash = strrchr(path, '/');
    if (!slash) {
        if (cap < 2) return ERR_OVERFLOW;
        out[0] = '.';
        out[1] = '\0';
        return ERR_OK;
    }

    size_t n = slash == path ? 1 : (size_t)(slash - path); /* keep root's '/' */
    if (n + 1 > cap) return ERR_OVERFLOW;
    memcpy(out, path, n);
    out[n] = '\0';
    return ERR_OK;
}

const char *path_ext(const char *path) {
    const char *base = path_basename(path);
    const char *dot = strrchr(base, '.');
    /* A leading dot is a hidden file, not an extension. */
    return (dot && dot != base) ? dot : "";
}

bool path_exists(const char *path) {
    struct stat st;
    return path && stat(path, &st) == 0;
}

bool path_is_dir(const char *path) {
    struct stat st;
    return path && stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

ErrorCode path_mkdirs(const char *path, unsigned int mode) {
    if (!path || path[0] == '\0') return ERR_INVALID_ARG;

    char buf[1024];
    if (strlen(path) + 1 > sizeof(buf)) return ERR_INVALID_ARG;
    memcpy(buf, path, strlen(path) + 1);

    /* Create each prefix in turn; EEXIST is fine at every step. */
    for (char *p = buf + 1; *p; p++) {
        if (*p != '/') continue;
        *p = '\0';
        if (mkdir(buf, (mode_t)mode) != 0 && errno != EEXIST) return ERR_IO;
        *p = '/';
    }
    if (mkdir(buf, (mode_t)mode) != 0 && errno != EEXIST) return ERR_IO;
    return path_is_dir(path) ? ERR_OK : ERR_IO; /* EEXIST could have been a file */
}
