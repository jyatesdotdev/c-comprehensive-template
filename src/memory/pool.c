/**
 * @file pool.c
 * @brief Fixed-size block pool allocator with free-list implementation.
 */
#include "memory/pool.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

ErrorCode pool_init(Pool *p, size_t block_size, size_t block_count) {
    if (!p || block_count == 0) return ERR_INVALID_ARG;
    /* Ensure blocks can hold a pointer for the free list */
    if (block_size < sizeof(void *)) block_size = sizeof(void *);
    if (block_count > SIZE_MAX / block_size) return ERR_OVERFLOW;
    p->block_size = block_size;
    p->block_count = block_count;
    p->buf = malloc(block_size * block_count);
    if (!p->buf) return ERR_NOMEM;

    /* Thread blocks into a free list */
    p->free_list = NULL;
    for (size_t i = block_count; i-- > 0;) {
        void *block = p->buf + i * block_size;
        *(void **)block = p->free_list;
        p->free_list = block;
    }
    return ERR_OK;
}

void *pool_alloc(Pool *p) {
    if (!p || !p->free_list) return NULL;
    void *block = p->free_list;
    p->free_list = *(void **)block;
    return block;
}

void pool_free(Pool *p, void *ptr) {
    if (!p || !ptr) return;
    *(void **)ptr = p->free_list;
    p->free_list = ptr;
}

void pool_destroy(Pool *p) {
    if (p) {
        free(p->buf);
        p->buf = NULL;
        p->free_list = NULL;
    }
}
