/**
 * @file vec.c
 * @brief Growable array implementation.
 */
#include "containers/vec.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define VEC_INITIAL_CAP 8

static const Allocator *vec_heap(const Allocator *alloc) {
    if (!alloc) return &allocator_libc;
    if (!alloc->realloc || !alloc->free) return NULL;
    return alloc;
}

ErrorCode vec_init_a(Vec *v, size_t elem_size, const Allocator *alloc) {
    if (!v || elem_size == 0) return ERR_INVALID_ARG;
    alloc = vec_heap(alloc);
    if (!alloc) return ERR_INVALID_ARG;
    v->data = NULL;
    v->len = 0;
    v->cap = 0;
    v->elem_size = elem_size;
    v->alloc = alloc;
    return ERR_OK;
}

ErrorCode vec_init(Vec *v, size_t elem_size) {
    return vec_init_a(v, elem_size, &allocator_libc);
}

void vec_destroy(Vec *v) {
    if (!v) return;
    if (v->data) {
        if (v->alloc && v->alloc->free) v->alloc->free(v->alloc->ctx, v->data);
        else free(v->data);
    }
    v->data = NULL;
    v->len = 0;
    v->cap = 0;
    v->elem_size = 0;
    v->alloc = NULL;
}

ErrorCode vec_reserve(Vec *v, size_t cap) {
    if (!v || v->elem_size == 0 || !v->alloc || !v->alloc->realloc) return ERR_INVALID_ARG;
    if (cap <= v->cap) return ERR_OK;
    if (cap > SIZE_MAX / v->elem_size) return ERR_OVERFLOW;

    unsigned char *tmp = v->alloc->realloc(v->alloc->ctx, v->data, cap * v->elem_size);
    if (!tmp) return ERR_NOMEM;
    v->data = tmp;
    v->cap = cap;
    return ERR_OK;
}

ErrorCode vec_push(Vec *v, const void *elem) {
    if (!v || !elem || v->elem_size == 0) return ERR_INVALID_ARG;

    if (v->len == v->cap) {
        size_t new_cap = v->cap == 0 ? VEC_INITIAL_CAP : v->cap * 2;
        if (new_cap <= v->cap) return ERR_OVERFLOW; /* cap * 2 wrapped */
        ErrorCode err = vec_reserve(v, new_cap);
        if (err) return err;
    }
    memcpy(v->data + v->len * v->elem_size, elem, v->elem_size);
    v->len++;
    return ERR_OK;
}

ErrorCode vec_pop(Vec *v, void *out) {
    if (!v || !v->data) return ERR_INVALID_ARG;
    if (v->len == 0) return ERR_NOT_FOUND;
    v->len--;
    if (out) memcpy(out, v->data + v->len * v->elem_size, v->elem_size);
    return ERR_OK;
}

void *vec_at(const Vec *v, size_t i) {
    if (!v || !v->data || i >= v->len) return NULL;
    return v->data + i * v->elem_size;
}

void vec_clear(Vec *v) {
    if (v) v->len = 0;
}
