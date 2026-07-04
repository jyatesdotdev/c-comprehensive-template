/**
 * @file thread_pool.c
 * @brief Thread pool with a lock-based task queue.
 */
#include "hpc/thread_pool.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>

/** @brief A single queued task node in the pool's linked list. */
typedef struct Task {
    TaskFunc     func;
    void        *arg;
    struct Task *next;
} Task;

/** @brief Internal thread pool state (opaque to callers). */
struct ThreadPool {
    pthread_t      *threads;
    size_t          num_threads;
    Task           *queue_head;
    Task           *queue_tail;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    bool            shutdown;
};

/** @brief Worker loop: dequeue and execute tasks until shutdown. */
static void *worker(void *arg) {
    ThreadPool *p = arg;
    for (;;) {
        pthread_mutex_lock(&p->mutex);
        while (!p->queue_head && !p->shutdown) pthread_cond_wait(&p->cond, &p->mutex);
        if (p->shutdown && !p->queue_head) {
            pthread_mutex_unlock(&p->mutex);
            return NULL;
        }
        Task *t = p->queue_head;
        p->queue_head = t->next;
        if (!p->queue_head) p->queue_tail = NULL;
        pthread_mutex_unlock(&p->mutex);
        t->func(t->arg);
        free(t);
    }
}

ErrorCode thread_pool_create(ThreadPool **pool, size_t num_threads) {
    if (!pool || num_threads == 0) return ERR_INVALID_ARG;
    ThreadPool *p = calloc(1, sizeof(ThreadPool));
    if (!p) return ERR_NOMEM;
    p->threads = malloc(num_threads * sizeof(pthread_t));
    if (!p->threads) {
        free(p);
        return ERR_NOMEM;
    }
    p->num_threads = num_threads;
    pthread_mutex_init(&p->mutex, NULL);
    pthread_cond_init(&p->cond, NULL);
    for (size_t i = 0; i < num_threads; i++) {
        if (pthread_create(&p->threads[i], NULL, worker, p) != 0) {
            /* Shut down the workers that did start, then fail cleanly. */
            pthread_mutex_lock(&p->mutex);
            p->shutdown = true;
            pthread_cond_broadcast(&p->cond);
            pthread_mutex_unlock(&p->mutex);
            for (size_t j = 0; j < i; j++) pthread_join(p->threads[j], NULL);
            pthread_mutex_destroy(&p->mutex);
            pthread_cond_destroy(&p->cond);
            free(p->threads);
            free(p);
            return ERR_IO;
        }
    }
    *pool = p;
    return ERR_OK;
}

ErrorCode thread_pool_submit(ThreadPool *pool, TaskFunc func, void *arg) {
    if (!pool || !func) return ERR_INVALID_ARG;
    Task *t = malloc(sizeof(Task));
    if (!t) return ERR_NOMEM;
    t->func = func;
    t->arg = arg;
    t->next = NULL;
    pthread_mutex_lock(&pool->mutex);
    if (pool->queue_tail) pool->queue_tail->next = t;
    else pool->queue_head = t;
    pool->queue_tail = t;
    pthread_cond_signal(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);
    return ERR_OK;
}

void thread_pool_destroy(ThreadPool *pool) {
    if (!pool) return;
    pthread_mutex_lock(&pool->mutex);
    pool->shutdown = true;
    pthread_cond_broadcast(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);
    for (size_t i = 0; i < pool->num_threads; i++) pthread_join(pool->threads[i], NULL);
    /* Drain remaining tasks */
    while (pool->queue_head) {
        Task *t = pool->queue_head;
        pool->queue_head = t->next;
        free(t);
    }
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond);
    free(pool->threads);
    free(pool);
}
