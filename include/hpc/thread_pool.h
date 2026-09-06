/**
 * @file thread_pool.h
 * @brief Simple pthreads-based thread pool.
 */
#ifndef HPC_THREAD_POOL_H
#define HPC_THREAD_POOL_H

#include "core/error.h"
#include <stddef.h>

/** @brief Opaque thread pool handle. */
typedef struct ThreadPool ThreadPool;

/** @brief Task function signature. */
typedef void (*TaskFunc)(void *arg);

/**
 * @brief Create a thread pool with the given number of worker threads.
 * @param pool        Receives the allocated pool handle.
 * @param num_threads Number of worker threads to spawn.
 * @return ERR_OK on success, ERR_INVALID_ARG, ERR_OVERFLOW, ERR_NOMEM, or ERR_IO.
 */
ErrorCode thread_pool_create(ThreadPool **pool, size_t num_threads);

/**
 * @brief Submit a task to the thread pool for asynchronous execution.
 *
 * Not safe to call concurrently with thread_pool_destroy.
 * @param pool Pool to submit to.
 * @param func Task function to execute.
 * @param arg  Argument passed to func.
 * @return ERR_OK on success, ERR_INVALID_ARG, ERR_NOMEM, or ERR_UNSUPPORTED if
 *         the pool is shutting down.
 */
ErrorCode thread_pool_submit(ThreadPool *pool, TaskFunc func, void *arg);

/**
 * @brief Destroy the thread pool, waiting for workers to finish queued tasks.
 *
 * Must not race with thread_pool_submit. Tasks submitted after shutdown begins
 * are rejected; any leftover nodes after workers exit are discarded unrun.
 * @param pool Pool to destroy.
 */
void thread_pool_destroy(ThreadPool *pool);

#endif /* HPC_THREAD_POOL_H */
