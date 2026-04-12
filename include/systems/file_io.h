/**
 * @file file_io.h
 * @brief Safe file I/O wrappers with error handling, including memory-mapped I/O.
 */
#ifndef SYSTEMS_FILE_IO_H
#define SYSTEMS_FILE_IO_H

#include "core/error.h"
#include <stddef.h>
#include <stdbool.h>

/* ── Standard file I/O ──────────────────────────────────────────────────── */

/**
 * @brief Read entire file into a caller-freed buffer.
 * @param path     File path to read.
 * @param out_buf  Receives malloc'd buffer with file contents.
 * @param out_size Receives byte count.
 * @return ERR_OK on success, ERR_IO on file error, ERR_NOMEM on allocation failure.
 */
ErrorCode file_read_all(const char *path, unsigned char **out_buf, size_t *out_size);

/**
 * @brief Write buffer to file atomically (write-to-temp then rename).
 * @param path File path to write.
 * @param buf  Data to write.
 * @param size Number of bytes to write.
 * @return ERR_OK on success, ERR_IO on file error.
 */
ErrorCode file_write_all(const char *path, const unsigned char *buf, size_t size);

/* ── Memory-mapped file I/O (POSIX) ─────────────────────────────────────── */

/** @brief Handle for a memory-mapped file. */
typedef struct MappedFile {
    void  *data;   /**< Pointer to mapped region. */
    size_t size;   /**< Size of mapped region in bytes. */
    int    fd;     /**< Underlying file descriptor. */
} MappedFile;

/**
 * @brief Map a file read-only into memory.
 * @param path File path to map.
 * @param out  Receives the mapped file handle. Caller must call file_munmap().
 * @return ERR_OK on success, ERR_IO on failure.
 */
ErrorCode file_mmap_read(const char *path, MappedFile *out);

/**
 * @brief Map a file read-write into memory. Changes are written back on munmap.
 * @param path File path to map.
 * @param out  Receives the mapped file handle. Caller must call file_munmap().
 * @return ERR_OK on success, ERR_IO on failure.
 */
ErrorCode file_mmap_rw(const char *path, MappedFile *out);

/**
 * @brief Unmap a previously mapped file.
 * @param mf Mapped file handle to release.
 * @return ERR_OK on success, ERR_IO on failure.
 */
ErrorCode file_munmap(MappedFile *mf);

/* ── Directory operations ───────────────────────────────────────────────── */

/**
 * @brief Callback invoked for each directory entry.
 * @param name      Entry name (not full path).
 * @param is_dir    true if the entry is a directory.
 * @param user_data User context pointer.
 * @return false to stop iteration, true to continue.
 */
typedef bool (*DirEntryCallback)(const char *name, bool is_dir, void *user_data);

/**
 * @brief Iterate entries in a directory, invoking cb for each.
 * @param path      Directory path.
 * @param cb        Callback function.
 * @param user_data User context pointer passed to cb.
 * @return ERR_OK on success, ERR_IO on failure.
 */
ErrorCode dir_walk(const char *path, DirEntryCallback cb, void *user_data);

#endif /* SYSTEMS_FILE_IO_H */
