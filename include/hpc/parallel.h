/**
 * @file parallel.h
 * @brief Parallel algorithm primitives using pthreads.
 */
#ifndef HPC_PARALLEL_H
#define HPC_PARALLEL_H

#include <stddef.h>

/**
 * @brief Parallel for: splits [0, n) across num_threads, calling body(start, end, ctx).
 * @param n           Total iteration count.
 * @param num_threads Number of threads to use.
 * @param body        Callback invoked with each thread's [start, end) range.
 * @param ctx         User context pointer passed to body.
 */
void parallel_for(size_t n, size_t num_threads, void (*body)(size_t start, size_t end, void *ctx),
                  void *ctx);

/**
 * @brief Parallel reduce: map-reduce across threads.
 *
 * Splits [0, n) across threads, each calls map(start, end, ctx) returning
 * a partial result, then combines partials pairwise with reduce(a, b).
 * @param n           Total iteration count.
 * @param num_threads Number of threads to use.
 * @param map         Map function returning a partial result per chunk.
 * @param ctx         User context pointer passed to map.
 * @param reduce      Reduction function combining two partial results.
 * @return Final reduced value.
 */
double parallel_reduce(size_t n, size_t                                          num_threads,
                       double (*map)(size_t start, size_t end, void *ctx), void *ctx,
                       double (*reduce)(double a, double b));

#endif /* HPC_PARALLEL_H */
