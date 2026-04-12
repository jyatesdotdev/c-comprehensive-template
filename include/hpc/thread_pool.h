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
 * @return ERR_OK on success, ERR_NOMEM on allocation failure.
 */
ErrorCode thread_pool_create(ThreadPool **pool, size_t num_threads);

/**
 * @brief Submit a task to the thread pool for asynchronous execution.
 * @param pool Pool to submit to.
 * @param func Task function to execute.
 * @param arg  Argument passed to func.
 * @return ERR_OK on success, or an error code.
 */
ErrorCode thread_pool_submit(ThreadPool *pool, TaskFunc func, void *arg);

/**
 * @brief Destroy the thread pool, waiting for all pending tasks to complete.
 * @param pool Pool to destroy.
 */
void      thread_pool_destroy(ThreadPool *pool);

#endif /* HPC_THREAD_POOL_H */
