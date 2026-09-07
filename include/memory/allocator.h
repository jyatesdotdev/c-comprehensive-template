/**
 * @file allocator.h
 * @brief Pluggable allocator vtable (libc by default; swap jemalloc/mimalloc).
 *
 * Pass an Allocator to containers that support it (`vec_init_a`). A NULL
 * allocator means libc. All three function pointers must be non-NULL when
 * you supply a custom vtable. `realloc(ctx, NULL, n)` must behave like
 * `alloc`; `realloc`/`free` must accept a pointer from this same vtable.
 */
#ifndef MEMORY_ALLOCATOR_H
#define MEMORY_ALLOCATOR_H

#include <stddef.h>

/**
 * @brief Heap operations plus an opaque context pointer.
 *
 * @param alloc   Allocate @p size bytes (may be unused if growth is realloc-only).
 * @param realloc Resize @p ptr to @p size bytes; NULL @p ptr allocates.
 * @param free    Release @p ptr; NULL @p ptr is a no-op.
 * @param ctx     Passed to every callback (may be NULL).
 */
typedef struct Allocator {
    void *(*alloc)(void *ctx, size_t size);
    void *(*realloc)(void *ctx, void *ptr, size_t size);
    void (*free)(void *ctx, void *ptr);
    void *ctx;
} Allocator;

/** @brief Process heap: malloc / realloc / free. */
extern const Allocator allocator_libc;

#endif /* MEMORY_ALLOCATOR_H */
