/**
 * @file queue.h
 * @brief Cross-thread queues: lock-free SPSC ring and blocking MPMC queue.
 *
 * SpscQueue — single producer, single consumer, lock-free (C11 atomics).
 *   Exactly one thread may push and one thread may pop. Non-blocking:
 *   push/pop return false when full/empty; callers retry or back off.
 *
 * BlockingQueue — any number of producers and consumers (pthread mutex +
 *   condition variables). Push blocks while full, pop blocks while empty;
 *   bq_close wakes everyone, lets consumers drain, then pops report
 *   ERR_NOT_FOUND — the standard shutdown pattern.
 */
#ifndef HPC_QUEUE_H
#define HPC_QUEUE_H

#include "core/error.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>

/* ── SPSC lock-free ring ────────────────────────────────────────────────── */

/** @brief Lock-free single-producer/single-consumer ring buffer. */
typedef struct SpscQueue {
    unsigned char *data;      /**< Slot storage ((cap + 1) * elem_size bytes). */
    size_t         elem_size; /**< Size of one element. */
    size_t         slots;     /**< Slot count (capacity + 1; one slot stays empty). */
    /* head and tail live on separate cache lines to avoid false sharing. */
    _Alignas(64) _Atomic size_t head; /**< Consumer position. */
    _Alignas(64) _Atomic size_t tail; /**< Producer position. */
} SpscQueue;

/**
 * @brief Initialize a ring holding up to capacity elements.
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

/* ── Blocking MPMC queue ────────────────────────────────────────────────── */

/** @brief Bounded blocking queue for any number of producers/consumers. */
typedef struct BlockingQueue {
    unsigned char  *data;      /**< Slot storage. */
    size_t          elem_size; /**< Size of one element. */
    size_t          cap;       /**< Maximum element count. */
    size_t          head;      /**< Oldest element index. */
    size_t          len;       /**< Current element count. */
    int             closed;    /**< Nonzero once bq_close was called. */
    pthread_mutex_t mutex;     /**< Protects all fields. */
    pthread_cond_t  not_empty; /**< Signaled on push and close. */
    pthread_cond_t  not_full;  /**< Signaled on pop and close. */
} BlockingQueue;

/**
 * @brief Initialize a queue holding up to capacity elements.
 * @return ERR_OK, ERR_INVALID_ARG, ERR_OVERFLOW, or ERR_NOMEM.
 */
ErrorCode bq_init(BlockingQueue *q, size_t elem_size, size_t capacity);

/** @brief Free storage. No thread may be blocked in push/pop. */
void bq_destroy(BlockingQueue *q);

/**
 * @brief Enqueue a copy of *elem, blocking while the queue is full.
 * @return ERR_OK, ERR_INVALID_ARG, or ERR_UNSUPPORTED if the queue is closed.
 */
ErrorCode bq_push(BlockingQueue *q, const void *elem);

/**
 * @brief Dequeue into *out, blocking while the queue is empty.
 * @return ERR_OK, ERR_INVALID_ARG, or ERR_NOT_FOUND once the queue is
 *         closed and fully drained.
 */
ErrorCode bq_pop(BlockingQueue *q, void *out);

/**
 * @brief Close the queue: wakes all blocked threads; subsequent pushes fail,
 *        pops drain the remaining elements then report ERR_NOT_FOUND.
 */
void bq_close(BlockingQueue *q);

#endif /* HPC_QUEUE_H */
