/**
 * @file arena.c
 * @brief Linear (bump) arena allocator implementation.
 */
#include "memory/arena.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

ErrorCode arena_init(Arena *a, size_t capacity) {
    if (!a || capacity == 0) return ERR_INVALID_ARG;
    a->buf = malloc(capacity);
    if (!a->buf) return ERR_NOMEM;
    a->cap = capacity;
    a->pos = 0;
    return ERR_OK;
}

void *arena_alloc(Arena *a, size_t size, size_t align) {
    if (!a || !a->buf || size == 0) return NULL;
    /* Alignment must be a non-zero power of two: (x & ~(align-1)) depends on it. */
    if (align == 0 || (align & (align - 1)) != 0) return NULL;
    if (a->pos > SIZE_MAX - (align - 1)) return NULL;
    size_t aligned = (a->pos + align - 1) & ~(align - 1);
    if (aligned > a->cap || size > a->cap - aligned) return NULL;
    void *ptr = a->buf + aligned;
    a->pos = aligned + size;
    return ptr;
}

void arena_reset(Arena *a) {
    if (a) a->pos = 0;
}

void arena_destroy(Arena *a) {
    if (a) {
        free(a->buf);
        a->buf = NULL;
        a->cap = 0;
        a->pos = 0;
    }
}
