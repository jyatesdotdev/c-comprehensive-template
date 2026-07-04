/**
 * @file path.h
 * @brief Filesystem path utilities: join, normalize, components, mkdir -p.
 *
 * Path string operations are lexical (no filesystem access, no symlink
 * resolution); path_exists/path_is_dir/path_mkdirs touch the filesystem.
 * POSIX separators ('/') only.
 */
#ifndef SYSTEMS_PATH_H
#define SYSTEMS_PATH_H

#include "core/error.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Join two path segments with exactly one separator.
 *        An absolute b replaces a (like most path libraries).
 * @param a   Left segment (may be empty).
 * @param b   Right segment (may be empty).
 * @param out Receives the joined path.
 * @param cap Capacity of out.
 * @return ERR_OK, ERR_INVALID_ARG, or ERR_OVERFLOW if out is too small.
 */
ErrorCode path_join(const char *a, const char *b, char *out, size_t cap);

/**
 * @brief Lexically normalize a path in place: collapse "//" and "/./",
 *        resolve "name/.." pairs, drop trailing slashes (except root).
 *        Leading ".." components are preserved (nothing to pop).
 * @param path Path to rewrite (result is never longer than the input).
 */
ErrorCode path_normalize(char *path);

/** @brief Final component ("file.txt" of "/a/b/file.txt"); "" for trailing '/'. */
const char *path_basename(const char *path);

/**
 * @brief Directory part ("/a/b" of "/a/b/file.txt"; "." if no directory).
 * @param path Input path.
 * @param out  Receives the directory.
 * @param cap  Capacity of out.
 * @return ERR_OK, ERR_INVALID_ARG, or ERR_OVERFLOW.
 */
ErrorCode path_dirname(const char *path, char *out, size_t cap);

/** @brief Extension including the dot (".txt"), or "" if none. */
const char *path_ext(const char *path);

/** @brief True if path exists (any file type). */
bool path_exists(const char *path);

/** @brief True if path exists and is a directory. */
bool path_is_dir(const char *path);

/**
 * @brief Create a directory and any missing parents (like `mkdir -p`).
 *        Succeeds if the directory already exists.
 * @param path Directory path to create.
 * @param mode Permission bits for created directories (e.g. 0755).
 * @return ERR_OK, ERR_INVALID_ARG, or ERR_IO on failure.
 */
ErrorCode path_mkdirs(const char *path, unsigned int mode);

#endif /* SYSTEMS_PATH_H */
