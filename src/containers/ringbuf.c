/**
 * @file ringbuf.c
 * @brief Circular buffer implementation.
 */
#include "containers/ringbuf.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

ErrorCode ringbuf_init(RingBuf *r, size_t elem_size, size_t capacity) {
    if (!r || elem_size == 0 || capacity == 0) return ERR_INVALID_ARG;
    if (capacity > SIZE_MAX / elem_size) return ERR_OVERFLOW;

    r->data = malloc(capacity * elem_size);
    if (!r->data) return ERR_NOMEM;
    r->elem_size = elem_size;
    r->cap = capacity;
    r->head = 0;
    r->len = 0;
    return ERR_OK;
}

void ringbuf_destroy(RingBuf *r) {
    if (r) {
        free(r->data);
        r->data = NULL;
        r->cap = 0;
        r->len = 0;
    }
}

/** Slot index of the next write position. */
static size_t tail_index(const RingBuf *r) {
    return (r->head + r->len) % r->cap;
}

ErrorCode ringbuf_push(RingBuf *r, const void *elem) {
    if (!r || !r->data || !elem) return ERR_INVALID_ARG;
    if (r->len == r->cap) return ERR_OVERFLOW;
    memcpy(r->data + tail_index(r) * r->elem_size, elem, r->elem_size);
    r->len++;
    return ERR_OK;
}

ErrorCode ringbuf_push_overwrite(RingBuf *r, const void *elem) {
    if (!r || !r->data || !elem) return ERR_INVALID_ARG;
    if (r->len < r->cap) return ringbuf_push(r, elem);

    /* Full: overwrite the oldest slot and advance the head past it. */
    memcpy(r->data + r->head * r->elem_size, elem, r->elem_size);
    r->head = (r->head + 1) % r->cap;
    return ERR_OK;
}

ErrorCode ringbuf_pop(RingBuf *r, void *out) {
    if (!r || !r->data) return ERR_INVALID_ARG;
    if (r->len == 0) return ERR_NOT_FOUND;
    if (out) memcpy(out, r->data + r->head * r->elem_size, r->elem_size);
    r->head = (r->head + 1) % r->cap;
    r->len--;
    return ERR_OK;
}

size_t ringbuf_len(const RingBuf *r) {
    return r ? r->len : 0;
}

bool ringbuf_is_full(const RingBuf *r) {
    return r && r->len == r->cap;
}
