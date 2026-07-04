/**
 * @file queue.c
 * @brief SPSC ring and blocking MPMC queue implementations.
 */
#include "hpc/queue.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ── SPSC ───────────────────────────────────────────────────────────────── */

ErrorCode spsc_init(SpscQueue *q, size_t elem_size, size_t capacity) {
    if (!q || elem_size == 0 || capacity == 0) return ERR_INVALID_ARG;
    if (capacity + 1 > SIZE_MAX / elem_size) return ERR_OVERFLOW;

    /* One slot is always left empty so head == tail means "empty"
       and (tail + 1) % slots == head means "full". */
    q->data = malloc((capacity + 1) * elem_size);
    if (!q->data) return ERR_NOMEM;
    q->elem_size = elem_size;
    q->slots = capacity + 1;
    atomic_store_explicit(&q->head, 0, memory_order_relaxed);
    atomic_store_explicit(&q->tail, 0, memory_order_relaxed);
    return ERR_OK;
}

void spsc_destroy(SpscQueue *q) {
    if (q) {
        free(q->data);
        q->data = NULL;
        q->slots = 0;
    }
}

bool spsc_push(SpscQueue *q, const void *elem) {
    if (!q || !q->data || !elem) return false;

    size_t tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
    size_t next = (tail + 1) % q->slots;
    /* acquire pairs with the consumer's head release: slot is truly free. */
    if (next == atomic_load_explicit(&q->head, memory_order_acquire)) return false; /* full */

    memcpy(q->data + tail * q->elem_size, elem, q->elem_size);
    /* release publishes the element bytes before the new tail is visible. */
    atomic_store_explicit(&q->tail, next, memory_order_release);
    return true;
}

bool spsc_pop(SpscQueue *q, void *out) {
    if (!q || !q->data || !out) return false;

    size_t head = atomic_load_explicit(&q->head, memory_order_relaxed);
    /* acquire pairs with the producer's tail release: element bytes visible. */
    if (head == atomic_load_explicit(&q->tail, memory_order_acquire)) return false; /* empty */

    memcpy(out, q->data + head * q->elem_size, q->elem_size);
    /* release lets the producer reuse the slot only after the copy is done. */
    atomic_store_explicit(&q->head, (head + 1) % q->slots, memory_order_release);
    return true;
}

size_t spsc_len(const SpscQueue *q) {
    if (!q || !q->data) return 0;
    size_t head = atomic_load_explicit(&q->head, memory_order_relaxed);
    size_t tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
    return (tail + q->slots - head) % q->slots;
}

/* ── Blocking MPMC ──────────────────────────────────────────────────────── */

ErrorCode bq_init(BlockingQueue *q, size_t elem_size, size_t capacity) {
    if (!q || elem_size == 0 || capacity == 0) return ERR_INVALID_ARG;
    if (capacity > SIZE_MAX / elem_size) return ERR_OVERFLOW;

    memset(q, 0, sizeof(*q));
    q->data = malloc(capacity * elem_size);
    if (!q->data) return ERR_NOMEM;
    q->elem_size = elem_size;
    q->cap = capacity;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
    return ERR_OK;
}

void bq_destroy(BlockingQueue *q) {
    if (!q || !q->data) return;
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
    free(q->data);
    q->data = NULL;
    q->cap = 0;
    q->len = 0;
}

ErrorCode bq_push(BlockingQueue *q, const void *elem) {
    if (!q || !q->data || !elem) return ERR_INVALID_ARG;

    pthread_mutex_lock(&q->mutex);
    while (q->len == q->cap && !q->closed) pthread_cond_wait(&q->not_full, &q->mutex);
    if (q->closed) {
        pthread_mutex_unlock(&q->mutex);
        return ERR_UNSUPPORTED;
    }

    size_t tail = (q->head + q->len) % q->cap;
    memcpy(q->data + tail * q->elem_size, elem, q->elem_size);
    q->len++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
    return ERR_OK;
}

ErrorCode bq_pop(BlockingQueue *q, void *out) {
    if (!q || !q->data || !out) return ERR_INVALID_ARG;

    pthread_mutex_lock(&q->mutex);
    while (q->len == 0 && !q->closed) pthread_cond_wait(&q->not_empty, &q->mutex);
    if (q->len == 0) { /* closed and drained */
        pthread_mutex_unlock(&q->mutex);
        return ERR_NOT_FOUND;
    }

    memcpy(out, q->data + q->head * q->elem_size, q->elem_size);
    q->head = (q->head + 1) % q->cap;
    q->len--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mutex);
    return ERR_OK;
}

void bq_close(BlockingQueue *q) {
    if (!q || !q->data) return;
    pthread_mutex_lock(&q->mutex);
    q->closed = 1;
    pthread_cond_broadcast(&q->not_empty);
    pthread_cond_broadcast(&q->not_full);
    pthread_mutex_unlock(&q->mutex);
}
