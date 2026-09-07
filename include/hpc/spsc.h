/**
 * @file spsc.h
 * @brief Lock-free single-producer/single-consumer ring buffer.
 *
 * Exactly one thread may push and one thread may pop. Non-blocking:
 * push/pop return false when full/empty; callers retry or back off.
 */
#ifndef HPC_SPSC_H
#define HPC_SPSC_H

#include "core/error.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>

/** @brief Lock-free single-producer/single-consumer ring buffer. */
typedef struct SpscQueue {
    unsigned char *data; /**< Slot storage ((cap + 1) * elem_size bytes). */
    size_t elem_size;    /**< Size of one element. */
    size_t slots;        /**< Slot count (capacity + 1; one slot stays empty). */
    /* head and tail live on separate cache lines to avoid false sharing. */
    _Alignas(64) _Atomic size_t head; /**< Consumer position. */
    _Alignas(64) _Atomic size_t tail; /**< Producer position. */
} SpscQueue;

/**
 * @brief Initialize a ring holding up to capacity elements.
 * @param q         Queue to initialize.
 * @param elem_size Size of one element in bytes.
 * @param capacity  Maximum element count (> 0).
 * @return ERR_OK, ERR_INVALID_ARG, ERR_OVERFLOW, or ERR_NOMEM.
 */
ErrorCode spsc_init(SpscQueue *q, size_t elem_size, size_t capacity);

/** @brief Free storage. Both threads must be done with the queue. */
void spsc_destroy(SpscQueue *q);

/** @brief Producer only: enqueue a copy of *elem. False when full. */
bool spsc_push(SpscQueue *q, const void *elem);

/** @brief Consumer only: dequeue into *out. False when empty. */
bool spsc_pop(SpscQueue *q, void *out);

/** @brief Approximate element count (racy by nature — for monitoring only). */
size_t spsc_len(const SpscQueue *q);

#endif /* HPC_SPSC_H */
