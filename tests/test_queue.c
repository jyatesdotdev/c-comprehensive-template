/**
 * @file test_queue.c
 * @brief Tests for hpc/queue.h: SPSC ring and blocking MPMC queue.
 */
#include "hpc/queue.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(cond)                                                                    \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

/* ── SPSC: single-threaded semantics ────────────────────────────────────── */

static void test_spsc_basics(void) {
    SpscQueue q;
    CHECK(spsc_init(NULL, 4, 4) == ERR_INVALID_ARG);
    CHECK(spsc_init(&q, 0, 4) == ERR_INVALID_ARG);
    CHECK(spsc_init(&q, sizeof(int), 3) == ERR_OK);

    int out = -1;
    CHECK(!spsc_pop(&q, &out)); /* empty */
    CHECK(spsc_len(&q) == 0);

    for (int i = 0; i < 3; i++) CHECK(spsc_push(&q, &i));
    CHECK(spsc_len(&q) == 3);
    int four = 4;
    CHECK(!spsc_push(&q, &four)); /* full at capacity */

    /* FIFO order across many wraparounds */
    CHECK(spsc_pop(&q, &out) && out == 0);
    for (int i = 3; i < 50; i++) {
        CHECK(spsc_push(&q, &i));
        int expect = i - 2;
        CHECK(spsc_pop(&q, &out) && out == expect);
    }
    while (spsc_pop(&q, &out)) {}
    CHECK(spsc_len(&q) == 0);

    spsc_destroy(&q);
    spsc_destroy(&q);
    spsc_destroy(NULL);
}

/* ── SPSC: cross-thread stress ──────────────────────────────────────────── */

enum { SPSC_ITEMS = 100000 };

static void *spsc_producer(void *arg) {
    SpscQueue *q = arg;
    for (int i = 1; i <= SPSC_ITEMS; i++) {
        while (!spsc_push(q, &i)) sched_yield();
    }
    return NULL;
}

static void test_spsc_threads(void) {
    SpscQueue q;
    CHECK(spsc_init(&q, sizeof(int), 64) == ERR_OK);

    pthread_t producer;
    CHECK(pthread_create(&producer, NULL, spsc_producer, &q) == 0);

    /* Consumer: every value arrives exactly once, in order. */
    long long sum = 0;
    int       expect = 1, v = 0;
    while (expect <= SPSC_ITEMS) {
        if (spsc_pop(&q, &v)) {
            CHECK(v == expect);
            sum += v;
            expect++;
        } else {
            sched_yield();
        }
    }
    CHECK(pthread_join(producer, NULL) == 0);
    CHECK(sum == (long long)SPSC_ITEMS * (SPSC_ITEMS + 1) / 2);
    spsc_destroy(&q);
}

/* ── BlockingQueue: multiple producers, one consumer ────────────────────── */

enum { BQ_PRODUCERS = 4, BQ_PER_PRODUCER = 5000 };

static void *bq_producer(void *arg) {
    BlockingQueue *q = arg;
    for (int i = 1; i <= BQ_PER_PRODUCER; i++) {
        if (bq_push(q, &i) != ERR_OK) break;
    }
    return NULL;
}

static void test_bq_threads(void) {
    BlockingQueue q;
    CHECK(bq_init(&q, sizeof(int), 16) == ERR_OK);

    pthread_t producers[BQ_PRODUCERS];
    for (int p = 0; p < BQ_PRODUCERS; p++)
        CHECK(pthread_create(&producers[p], NULL, bq_producer, &q) == 0);

    /* Consume exactly the expected total, then close and drain. */
    long long expect_sum =
        (long long)BQ_PRODUCERS * ((long long)BQ_PER_PRODUCER * (BQ_PER_PRODUCER + 1) / 2);
    long long sum = 0;
    int       v = 0;
    for (int n = 0; n < BQ_PRODUCERS * BQ_PER_PRODUCER; n++) {
        CHECK(bq_pop(&q, &v) == ERR_OK);
        sum += v;
    }
    CHECK(sum == expect_sum);

    for (int p = 0; p < BQ_PRODUCERS; p++) CHECK(pthread_join(producers[p], NULL) == 0);

    /* Close: pending pops report exhaustion, pushes are refused. */
    bq_close(&q);
    CHECK(bq_pop(&q, &v) == ERR_NOT_FOUND);
    int one = 1;
    CHECK(bq_push(&q, &one) == ERR_UNSUPPORTED);

    bq_destroy(&q);
    bq_destroy(&q);
    bq_destroy(NULL);
}

/* ── BlockingQueue: close drains queued items first ─────────────────────── */

static void test_bq_close_drains(void) {
    BlockingQueue q;
    CHECK(bq_init(&q, sizeof(int), 8) == ERR_OK);
    for (int i = 0; i < 5; i++) CHECK(bq_push(&q, &i) == ERR_OK);
    bq_close(&q);

    int v = -1;
    for (int i = 0; i < 5; i++) {
        CHECK(bq_pop(&q, &v) == ERR_OK);
        CHECK(v == i);
    }
    CHECK(bq_pop(&q, &v) == ERR_NOT_FOUND);
    bq_destroy(&q);
}

static void test_bq_invalid(void) {
    BlockingQueue q;
    CHECK(bq_init(NULL, 4, 4) == ERR_INVALID_ARG);
    CHECK(bq_init(&q, 0, 4) == ERR_INVALID_ARG);
    CHECK(bq_init(&q, 4, 0) == ERR_INVALID_ARG);
    CHECK(bq_init(&q, sizeof(int), 2) == ERR_OK);
    CHECK(bq_push(&q, NULL) == ERR_INVALID_ARG);
    CHECK(bq_pop(&q, NULL) == ERR_INVALID_ARG);
    bq_destroy(&q);
}

int main(void) {
    test_spsc_basics();
    test_spsc_threads();
    test_bq_threads();
    test_bq_close_drains();
    test_bq_invalid();
    printf("All queue tests passed.\n");
    return 0;
}
