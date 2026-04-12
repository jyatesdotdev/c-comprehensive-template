/**
 * @file parallel.c
 * @brief Parallel for-loop and map-reduce using pthreads.
 */
#include "hpc/parallel.h"
#include <pthread.h>
#include <stdlib.h>

/* ── parallel_for ─────────────────────────────────────────────────────── */

/** @brief Per-thread argument for parallel_for chunk execution. */
typedef struct {
    size_t start, end;
    void (*body)(size_t, size_t, void *);
    void *ctx;
} ChunkArg;

/** @brief Thread entry point that executes a chunk of the parallel_for range. */
static void *chunk_worker(void *arg) {
    ChunkArg *c = arg;
    c->body(c->start, c->end, c->ctx);
    return NULL;
}

void parallel_for(size_t n, size_t num_threads,
                  void (*body)(size_t start, size_t end, void *ctx), void *ctx) {
    if (n == 0 || num_threads == 0) return;
    if (num_threads > n) num_threads = n;

    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    ChunkArg  *args    = malloc(num_threads * sizeof(ChunkArg));
    if (!threads || !args) { free(threads); free(args); body(0, n, ctx); return; }

    size_t chunk = n / num_threads, rem = n % num_threads, off = 0;
    for (size_t i = 0; i < num_threads; i++) {
        args[i] = (ChunkArg){off, off + chunk + (i < rem ? 1 : 0), body, ctx};
        off = args[i].end;
        pthread_create(&threads[i], NULL, chunk_worker, &args[i]);
    }
    for (size_t i = 0; i < num_threads; i++)
        pthread_join(threads[i], NULL);
    free(threads);
    free(args);
}

/* ── parallel_reduce ──────────────────────────────────────────────────── */

/** @brief Per-thread argument for parallel_reduce chunk execution. */
typedef struct {
    size_t start, end;
    double (*map)(size_t, size_t, void *);
    void *ctx;
    double result;
} ReduceArg;

/** @brief Thread entry point that maps a chunk and stores the partial result. */
static void *reduce_worker(void *arg) {
    ReduceArg *r = arg;
    r->result = r->map(r->start, r->end, r->ctx);
    return NULL;
}

double parallel_reduce(size_t n, size_t num_threads,
                       double (*map)(size_t start, size_t end, void *ctx),
                       void *ctx,
                       double (*reduce)(double a, double b)) {
    if (n == 0) return 0.0;
    if (num_threads <= 1) return map(0, n, ctx);

    if (num_threads > n) num_threads = n;
    pthread_t  *threads = malloc(num_threads * sizeof(pthread_t));
    ReduceArg  *args    = malloc(num_threads * sizeof(ReduceArg));
    if (!threads || !args) { free(threads); free(args); return map(0, n, ctx); }

    size_t chunk = n / num_threads, rem = n % num_threads, off = 0;
    for (size_t i = 0; i < num_threads; i++) {
        args[i] = (ReduceArg){off, off + chunk + (i < rem ? 1 : 0), map, ctx, 0.0};
        off = args[i].end;
        pthread_create(&threads[i], NULL, reduce_worker, &args[i]);
    }

    double result = 0.0;
    for (size_t i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        result = (i == 0) ? args[i].result : reduce(result, args[i].result);
    }
    free(threads);
    free(args);
    return result;
}
