/**
 * @file file_io.c
 * @brief File I/O utilities: read/write, memory-mapped I/O, directory walking.
 */
#include "systems/file_io.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
/* Minimal stubs — full Win32 impl left as exercise. */
ErrorCode file_mmap_read(const char *path, MappedFile *out) {
    (void)path;
    (void)out;
    return ERR_UNSUPPORTED;
}
ErrorCode file_mmap_rw(const char *path, MappedFile *out) {
    (void)path;
    (void)out;
    return ERR_UNSUPPORTED;
}
ErrorCode file_munmap(MappedFile *mf) {
    (void)mf;
    return ERR_UNSUPPORTED;
}
ErrorCode dir_walk(const char *p, DirEntryCallback cb, void *ud) {
    (void)p;
    (void)cb;
    (void)ud;
    return ERR_UNSUPPORTED;
}
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#endif

/* ── Standard file I/O ──────────────────────────────────────────────────── */

ErrorCode file_read_all(const char *path, unsigned char **out_buf, size_t *out_size) {
    if (!path || !out_buf || !out_size) return ERR_INVALID_ARG;
    FILE *f = fopen(path, "rb");
    if (!f) return ERR_IO;

    if (fseek(f, 0, SEEK_END) != 0) {
        (void)fclose(f); /* already returning an error */
        return ERR_IO;
    }
    long len = ftell(f);
    if (len < 0 || fseek(f, 0, SEEK_SET) != 0) {
        (void)fclose(f);
        return ERR_IO;
    }

    /* malloc(0) may return NULL; treat an empty file as a 1-byte dummy buffer. */
    *out_buf = malloc(len == 0 ? 1 : (size_t)len);
    if (!*out_buf) {
        (void)fclose(f);
        return ERR_NOMEM;
    }

    if (len > 0 && fread(*out_buf, 1, (size_t)len, f) != (size_t)len) {
        free(*out_buf);
        *out_buf = NULL;
        (void)fclose(f);
        return ERR_IO;
    }
    if (fclose(f) != 0) {
        free(*out_buf);
        *out_buf = NULL;
        return ERR_IO;
    }
    *out_size = (size_t)len;
    return ERR_OK;
}

ErrorCode file_write_all(const char *path, const unsigned char *buf, size_t size) {
    if (!path || (!buf && size > 0)) return ERR_INVALID_ARG;

    size_t path_len = strlen(path);
    if (path_len > SIZE_MAX - 5) return ERR_OVERFLOW;
    char *tmp = malloc(path_len + 5); /* ".tmp" + NUL */
    if (!tmp) return ERR_NOMEM;
    memcpy(tmp, path, path_len);
    memcpy(tmp + path_len, ".tmp", 5);

    FILE *f = fopen(tmp, "wb");
    if (!f) {
        free(tmp);
        return ERR_IO;
    }
    if (size > 0 && fwrite(buf, 1, size, f) != size) {
        (void)fclose(f);
        (void)remove(tmp);
        free(tmp);
        return ERR_IO;
    }
    if (fclose(f) != 0) {
        (void)remove(tmp);
        free(tmp);
        return ERR_IO;
    }
#ifdef _WIN32
    (void)remove(path); /* POSIX rename replaces; Windows rename does not. */
#endif
    if (rename(tmp, path) != 0) {
        (void)remove(tmp);
        free(tmp);
        return ERR_IO;
    }
    free(tmp);
    return ERR_OK;
}

/* ── POSIX memory-mapped I/O & directory ops ────────────────────────────── */
#ifndef _WIN32

/**
 * @brief Open and memory-map a file with the given flags and protections.
 * @param path    File path to map.
 * @param oflags  open() flags (e.g. O_RDONLY).
 * @param prot    mmap protection flags.
 * @param mflags  mmap mapping flags.
 * @param out     Output MappedFile struct.
 * @return ERR_OK on success, or an error code.
 */
static ErrorCode mmap_open(const char *path, int oflags, int prot, int mflags, MappedFile *out) {
    if (!path || !out) return ERR_INVALID_ARG;

    int fd = open(path, oflags);
    if (fd < 0) return ERR_IO;

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return ERR_IO;
    }

    if (st.st_size == 0) {
        close(fd);
        return ERR_UNSUPPORTED; /* mmap(size=0) is EINVAL on POSIX */
    }

    void *data = mmap(NULL, (size_t)st.st_size, prot, mflags, fd, 0);
    if (data == MAP_FAILED) {
        close(fd);
        return ERR_IO;
    }

    out->data = data;
    out->size = (size_t)st.st_size;
    out->fd = fd;
    return ERR_OK;
}

ErrorCode file_mmap_read(const char *path, MappedFile *out) {
    return mmap_open(path, O_RDONLY, PROT_READ, MAP_PRIVATE, out);
}

ErrorCode file_mmap_rw(const char *path, MappedFile *out) {
    return mmap_open(path, O_RDWR, PROT_READ | PROT_WRITE, MAP_SHARED, out);
}

ErrorCode file_munmap(MappedFile *mf) {
    if (!mf || !mf->data) return ERR_INVALID_ARG;
    munmap(mf->data, mf->size);
    close(mf->fd);
    mf->data = NULL;
    mf->size = 0;
    mf->fd = -1;
    return ERR_OK;
}

ErrorCode dir_walk(const char *path, DirEntryCallback cb, void *user_data) {
    if (!path || !cb) return ERR_INVALID_ARG;
    DIR *d = opendir(path);
    if (!d) return ERR_IO;

    struct dirent *ent;
    /* NOLINTNEXTLINE(concurrency-mt-unsafe) — readdir is the modern API; only unsafe if one DIR* is shared across threads */
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.' &&
            (ent->d_name[1] == '\0' || (ent->d_name[1] == '.' && ent->d_name[2] == '\0')))
            continue;
        bool is_dir = (ent->d_type == DT_DIR);
        if (ent->d_type == DT_UNKNOWN) {
            char full[1024];
            if (snprintf(full, sizeof(full), "%s/%s", path, ent->d_name) < (int)sizeof(full)) {
                struct stat st;
                if (stat(full, &st) == 0) is_dir = S_ISDIR(st.st_mode);
            }
        }
        if (!cb(ent->d_name, is_dir, user_data)) break;
    }
    closedir(d);
    return ERR_OK;
}

#endif /* !_WIN32 */
