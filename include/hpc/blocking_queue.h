/**
 * @file blocking_queue.h
 * @brief Bounded blocking MPMC queue (pthread mutex + condition variables).
 *
 * Any number of producers and consumers. Push blocks while full, pop blocks
 * while empty; bq_close wakes everyone, lets consumers drain, then pops
 * report ERR_NOT_FOUND — the standard shutdown pattern.
 */
#ifndef HPC_BLOCKING_QUEUE_H
#define HPC_BLOCKING_QUEUE_H

#include "core/error.h"
#include <pthread.h>
#include <stddef.h>

/** @brief Bounded blocking queue for any number of producers/consumers. */
typedef struct BlockingQueue {
    unsigned char *data;      /**< Slot storage. */
    size_t elem_size;         /**< Size of one element. */
    size_t cap;               /**< Maximum element count. */
    size_t head;              /**< Oldest element index. */
    size_t len;               /**< Current element count. */
    int closed;               /**< Nonzero once bq_close was called. */
    pthread_mutex_t mutex;    /**< Protects all fields. */
    pthread_cond_t not_empty; /**< Signaled on push and close. */
    pthread_cond_t not_full;  /**< Signaled on pop and close. */
} BlockingQueue;

/**
 * @brief Initialize a queue holding up to capacity elements.
 * @param q         Queue to initialize.
 * @param elem_size Size of one element in bytes.
 * @param capacity  Maximum element count (> 0).
 * @return ERR_OK, ERR_INVALID_ARG, ERR_OVERFLOW, or ERR_NOMEM.
 */
ErrorCode bq_init(BlockingQueue *q, size_t elem_size, size_t capacity);

/** @brief Free storage. No thread may be blocked in push/pop. */
void bq_destroy(BlockingQueue *q);

/**
 * @brief Enqueue a copy of *elem, blocking while the queue is full.
 * @param q    Queue.
 * @param elem Element to copy in.
 * @return ERR_OK, ERR_INVALID_ARG, or ERR_UNSUPPORTED if the queue is closed.
 */
ErrorCode bq_push(BlockingQueue *q, const void *elem);

/**
 * @brief Dequeue into *out, blocking while the queue is empty.
 * @param q   Queue.
 * @param out Receives the element.
 * @return ERR_OK, ERR_INVALID_ARG, or ERR_NOT_FOUND once the queue is
 *         closed and fully drained.
 */
ErrorCode bq_pop(BlockingQueue *q, void *out);

/**
 * @brief Close the queue: wakes all blocked threads; subsequent pushes fail,
 *        pops drain the remaining elements then report ERR_NOT_FOUND.
 * @param q Queue to close.
 */
void bq_close(BlockingQueue *q);

#endif /* HPC_BLOCKING_QUEUE_H */
