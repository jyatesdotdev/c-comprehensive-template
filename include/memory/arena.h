/**
 * @file arena.h
 * @brief Arena (bump) allocator for fast, bulk-freed allocations.
 */
#ifndef MEMORY_ARENA_H
#define MEMORY_ARENA_H

#include "core/error.h"
#include <stddef.h>

typedef struct Arena {
    unsigned char *buf;
    size_t         cap;
    size_t         pos;
} Arena;

/**
 * @brief Initialize an arena with the given capacity.
 * @param a      Arena to initialize.
 * @param capacity  Total bytes to allocate for the backing buffer.
 * @return ERR_OK on success, ERR_NOMEM if malloc fails.
 */
ErrorCode arena_init(Arena *a, size_t capacity);

/**
 * @brief Allocate aligned memory from the arena.
 * @param a     Arena to allocate from.
 * @param size  Number of bytes requested.
 * @param align Alignment requirement (must be power of 2).
 * @return Pointer to allocated memory, or NULL if arena is full.
 */
void *arena_alloc(Arena *a, size_t size, size_t align);

/** @brief Reset the arena, freeing all allocations at once. */
void arena_reset(Arena *a);

/** @brief Destroy the arena and release its backing buffer. */
void arena_destroy(Arena *a);

#endif /* MEMORY_ARENA_H */
