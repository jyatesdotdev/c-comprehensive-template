/**
 * @file pipeline_demo.c
 * @brief Producer/consumer pipeline: N producers feed a BlockingQueue, one
 *        consumer aggregates, and bq_close coordinates clean shutdown.
 *
 * The blocking queue is the backpressure point: producers stall when the
 * consumer falls behind, so memory stays bounded no matter the data volume.
 */
#include "core/log.h"
#include "core/time.h"
#include "hpc/queue.h"

#include <pthread.h>
#include <stdio.h>

enum { PRODUCERS = 3, ITEMS_PER_PRODUCER = 1000, QUEUE_CAP = 32 };

typedef struct WorkItem {
    int producer_id;
    int value;
} WorkItem;

typedef struct ProducerArgs {
    BlockingQueue *queue;
    int            id;
} ProducerArgs;

static void *producer_main(void *arg) {
    ProducerArgs *pa = arg;
    for (int i = 1; i <= ITEMS_PER_PRODUCER; i++) {
        WorkItem item = {.producer_id = pa->id, .value = i};
        if (bq_push(pa->queue, &item) != ERR_OK) break; /* queue closed */
    }
    LOG_DEBUG("producer %d finished", pa->id);
    return NULL;
}

int main(void) {
    log_set_level(LOG_LEVEL_DEBUG);
    Stopwatch sw;
    stopwatch_start(&sw);

    BlockingQueue queue;
    if (bq_init(&queue, sizeof(WorkItem), QUEUE_CAP) != ERR_OK) {
        fprintf(stderr, "bq_init failed\n");
        return 1;
    }

    pthread_t    threads[PRODUCERS];
    ProducerArgs args[PRODUCERS];
    for (int p = 0; p < PRODUCERS; p++) {
        args[p] = (ProducerArgs){.queue = &queue, .id = p};
        if (pthread_create(&threads[p], NULL, producer_main, &args[p]) != 0) {
            fprintf(stderr, "pthread_create failed\n");
            return 1;
        }
    }

    /* Consumer: this thread. We know the total, so consume exactly that
       many; open-ended pipelines would close the queue from a coordinator. */
    long long total = 0;
    long long per_producer[PRODUCERS] = {0};
    WorkItem  item;
    for (int n = 0; n < PRODUCERS * ITEMS_PER_PRODUCER; n++) {
        if (bq_pop(&queue, &item) != ERR_OK) break;
        total += item.value;
        per_producer[item.producer_id]++;
    }

    for (int p = 0; p < PRODUCERS; p++) pthread_join(threads[p], NULL);
    bq_close(&queue);

    for (int p = 0; p < PRODUCERS; p++)
        printf("producer %d delivered %lld items\n", p, per_producer[p]);
    printf("aggregated total: %lld (expected %lld)\n", total,
           (long long)PRODUCERS * ITEMS_PER_PRODUCER * (ITEMS_PER_PRODUCER + 1) / 2);
    LOG_INFO("pipeline finished in %.2f ms", stopwatch_elapsed_ms(&sw));

    bq_destroy(&queue);
    return 0;
}
