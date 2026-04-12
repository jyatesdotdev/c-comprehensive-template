/** Thread pool demo */
#include "hpc/thread_pool.h"
#include <stdio.h>
#include <unistd.h>

static void task(void *arg) {
    int id = *(int *)arg;
    printf("Task %d running\n", id);
}

int main(void) {
    ThreadPool *pool;
    if (thread_pool_create(&pool, 4) != ERR_OK) return 1;

    int ids[8];
    for (int i = 0; i < 8; i++) {
        ids[i] = i;
        thread_pool_submit(pool, task, &ids[i]);
    }
    usleep(100000); /* let tasks complete */
    thread_pool_destroy(pool);
    return 0;
}
