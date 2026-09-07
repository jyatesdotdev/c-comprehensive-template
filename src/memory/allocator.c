/**
 * @file allocator.c
 * @brief Libc-backed Allocator vtable.
 */
#include "memory/allocator.h"

#include <stdlib.h>

static void *libc_alloc(void *ctx, size_t size) {
    (void)ctx;
    return malloc(size);
}

static void *libc_realloc(void *ctx, void *ptr, size_t size) {
    (void)ctx;
    return realloc(ptr, size);
}

static void libc_free(void *ctx, void *ptr) {
    (void)ctx;
    free(ptr);
}

const Allocator allocator_libc = {
    .alloc = libc_alloc,
    .realloc = libc_realloc,
    .free = libc_free,
    .ctx = NULL,
};
