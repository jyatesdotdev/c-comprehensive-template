/**
 * @file ringbuf.h
 * @brief Fixed-capacity circular buffer of fixed-size elements (FIFO).
 *
 * Single-threaded. For a lock-free cross-thread queue, see hpc/queue.h.
 */
#ifndef CONTAINERS_RINGBUF_H
#define CONTAINERS_RINGBUF_H

#include "core/error.h"
#include <stdbool.h>
#include <stddef.h>

/** @brief Circular FIFO. Initialize with ringbuf_init, free with ringbuf_destroy. */
typedef struct RingBuf {
    unsigned char *data;      /**< Element storage (cap * elem_size bytes). */
    size_t         elem_size; /**< Size of one element. */
    size_t         cap;       /**< Maximum element count. */
    size_t         head;      /**< Index of the oldest element. */
    size_t         len;       /**< Current element count. */
} RingBuf;

/**
 * @brief Initialize a ring holding up to capacity elements of elem_size bytes.
 * @return ERR_OK, ERR_INVALID_ARG, ERR_OVERFLOW, or ERR_NOMEM.
 */
ErrorCode ringbuf_init(RingBuf *r, size_t elem_size, size_t capacity);

/** @brief Free storage. Safe on NULL/already-destroyed. */
void ringbuf_destroy(RingBuf *r);

/**
 * @brief Append a copy of *elem at the tail.
 * @return ERR_OK, ERR_INVALID_ARG, or ERR_OVERFLOW when full.
 */
ErrorCode ringbuf_push(RingBuf *r, const void *elem);

/** @brief Like ringbuf_push, but overwrites the oldest element when full. */
ErrorCode ringbuf_push_overwrite(RingBuf *r, const void *elem);

/**
 * @brief Remove the oldest element.
 * @param r   Ring.
 * @param out Receives the element (may be NULL to discard).
 * @return ERR_OK, ERR_INVALID_ARG, or ERR_NOT_FOUND when empty.
 */
ErrorCode ringbuf_pop(RingBuf *r, void *out);

/** @brief Current element count. */
size_t ringbuf_len(const RingBuf *r);

/** @brief True when no more elements fit. */
bool ringbuf_is_full(const RingBuf *r);

#endif /* CONTAINERS_RINGBUF_H */
