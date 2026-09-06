/**
 * @file pool.h
 * @brief Fixed-size block pool allocator.
 */
#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include "core/error.h"
#include <stddef.h>

typedef struct Pool {
    unsigned char *buf;
    size_t         block_size;
    size_t         block_count;
    void          *free_list;
} Pool;

/**
 * @brief Initialize a pool of fixed-size blocks.
 * @param p           Pool to initialize.
 * @param block_size  Size of each block in bytes. Silently raised to
 *                    sizeof(void *) if smaller, so the free list can store a pointer.
 * @param block_count Number of blocks to pre-allocate.
 * @return ERR_OK on success, ERR_INVALID_ARG if p is NULL or block_count is 0,
 *         ERR_OVERFLOW if block_size * block_count overflows, ERR_NOMEM if malloc fails.
 */
ErrorCode pool_init(Pool *p, size_t block_size, size_t block_count);

/**
 * @brief Allocate one block from the pool.
 * @param p Pool to allocate from.
 * @return Pointer to a block, or NULL if pool is exhausted.
 */
void *pool_alloc(Pool *p);

/**
 * @brief Return a block to the pool.
 * @param p   Pool that owns the block.
 * @param ptr Pointer previously returned by pool_alloc(). Ignored if NULL or
 *            not an aligned pointer inside this pool's buffer.
 */
void pool_free(Pool *p, void *ptr);

/** @brief Destroy the pool and release its backing buffer. */
void pool_destroy(Pool *p);

#endif /* MEMORY_POOL_H */
