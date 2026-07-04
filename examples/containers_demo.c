/**
 * @file containers_demo.c
 * @brief Tour of the containers module plus core logging and timing:
 *        word-frequency counting with Vec + HashMap + StrBuf + Stopwatch.
 */
#include "containers/hashmap.h"
#include "containers/ringbuf.h"
#include "containers/strbuf.h"
#include "containers/vec.h"
#include "core/log.h"
#include "core/time.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    log_set_level(LOG_LEVEL_DEBUG);
    Stopwatch sw;
    stopwatch_start(&sw);

    /* ── Count word frequencies with a HashMap ─────────────────────────── */
    const char  *words[] = {"the", "quick", "brown", "fox", "jumps", "over",
                            "the", "lazy",  "dog",   "the", "fox"};
    const size_t num_words = sizeof(words) / sizeof(words[0]);

    HashMap counts;
    if (hashmap_init(&counts) != ERR_OK) return 1;

    /* Values live in a Vec so their addresses are stable per insertion. */
    Vec storage;
    if (vec_init(&storage, sizeof(int)) != ERR_OK) return 1;
    if (vec_reserve(&storage, num_words) != ERR_OK) return 1; /* no realloc → stable */

    for (size_t i = 0; i < num_words; i++) {
        int *count = hashmap_get(&counts, words[i]);
        if (count) {
            (*count)++;
        } else {
            int one = 1;
            if (vec_push(&storage, &one) != ERR_OK) return 1;
            if (hashmap_put(&counts, words[i], vec_at(&storage, storage.len - 1)) != ERR_OK)
                return 1;
        }
    }
    LOG_DEBUG("counted %zu words into %zu unique keys", num_words, hashmap_len(&counts));

    /* ── Render a report with StrBuf ───────────────────────────────────── */
    StrBuf report;
    if (strbuf_init(&report) != ERR_OK) return 1;

    HashMapIter it = {0};
    const char *key = NULL;
    void       *val = NULL;
    while (hashmap_next(&counts, &it, &key, &val)) {
        if (strbuf_appendf(&report, "%s=%d ", key, *(int *)val) != ERR_OK) return 1;
    }
    printf("word counts: %s\n", strbuf_cstr(&report));

    /* ── RingBuf as a fixed-size "recent events" log ───────────────────── */
    RingBuf recent;
    if (ringbuf_init(&recent, sizeof(int), 3) != ERR_OK) return 1;
    for (int event = 1; event <= 5; event++) {
        (void)ringbuf_push_overwrite(&recent, &event); /* keeps only the last 3 */
    }
    printf("last 3 events: ");
    int ev = 0;
    while (ringbuf_pop(&recent, &ev) == ERR_OK) printf("%d ", ev);
    printf("\n");

    LOG_INFO("demo finished in %.2f ms", stopwatch_elapsed_ms(&sw));

    ringbuf_destroy(&recent);
    strbuf_destroy(&report);
    vec_destroy(&storage);
    hashmap_destroy(&counts);
    return 0;
}
