/**
 * @file test_containers.c
 * @brief Tests for the containers module and core log/time utilities.
 */
#include "containers/hash.h"
#include "containers/hashmap.h"
#include "containers/ringbuf.h"
#include "containers/strbuf.h"
#include "containers/vec.h"
#include "core/log.h"
#include "core/time.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(cond)                                                                    \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

static void test_hash(void) {
    /* Known FNV-1a vectors */
    CHECK(hash_fnv1a("", 0) == 14695981039346656037ULL);
    CHECK(hash_fnv1a_str("a") == 12638187200555641996ULL);
    CHECK(hash_fnv1a_str("hello") == hash_fnv1a("hello", 5));
    CHECK(hash_fnv1a_str(NULL) == hash_fnv1a("", 0));
    CHECK(hash_fnv1a_str("hello") != hash_fnv1a_str("hellp"));

    /* Known CRC-32 vectors (matches zlib) */
    CHECK(hash_crc32("", 0) == 0x00000000u);
    CHECK(hash_crc32("123456789", 9) == 0xCBF43926u);
    CHECK(hash_crc32("hello", 5) == 0x3610A686u);
}

static void test_vec(void) {
    Vec v;
    CHECK(vec_init(NULL, 4) == ERR_INVALID_ARG);
    CHECK(vec_init(&v, 0) == ERR_INVALID_ARG);
    CHECK(vec_init(&v, sizeof(int)) == ERR_OK);
    CHECK(v.len == 0);
    CHECK(vec_at(&v, 0) == NULL);

    /* Push enough to force several growths */
    for (int i = 0; i < 100; i++) CHECK(vec_push(&v, &i) == ERR_OK);
    CHECK(v.len == 100);
    for (int i = 0; i < 100; i++) {
        int *p = vec_at(&v, (size_t)i);
        CHECK(p && *p == i);
    }
    CHECK(vec_at(&v, 100) == NULL);

    int out = -1;
    CHECK(vec_pop(&v, &out) == ERR_OK);
    CHECK(out == 99 && v.len == 99);
    CHECK(vec_pop(&v, NULL) == ERR_OK);
    CHECK(v.len == 98);

    vec_clear(&v);
    CHECK(v.len == 0);
    CHECK(vec_pop(&v, &out) == ERR_NOT_FOUND);

    CHECK(vec_reserve(&v, 1000) == ERR_OK);
    CHECK(v.cap >= 1000 && v.len == 0);

    vec_destroy(&v);
    vec_destroy(&v); /* double-destroy is safe */
    vec_destroy(NULL);
}

static void test_hashmap(void) {
    HashMap m;
    CHECK(hashmap_init(NULL) == ERR_INVALID_ARG);
    CHECK(hashmap_init(&m) == ERR_OK);
    CHECK(hashmap_len(&m) == 0);
    CHECK(hashmap_get(&m, "missing") == NULL);
    CHECK(!hashmap_contains(&m, "missing"));
    CHECK(hashmap_remove(&m, "missing") == ERR_NOT_FOUND);

    /* Insert enough entries to force several rehashes */
    static int values[200];
    char       key[32];
    for (int i = 0; i < 200; i++) {
        values[i] = i * 7;
        snprintf(key, sizeof(key), "key_%d", i);
        CHECK(hashmap_put(&m, key, &values[i]) == ERR_OK);
    }
    CHECK(hashmap_len(&m) == 200);
    for (int i = 0; i < 200; i++) {
        snprintf(key, sizeof(key), "key_%d", i);
        int *p = hashmap_get(&m, key);
        CHECK(p && *p == i * 7);
    }

    /* Update in place doesn't change the count */
    CHECK(hashmap_put(&m, "key_5", &values[0]) == ERR_OK);
    CHECK(hashmap_len(&m) == 200);
    CHECK(hashmap_get(&m, "key_5") == &values[0]);

    /* Remove half, confirm the rest still resolve (tombstone probing) */
    for (int i = 0; i < 200; i += 2) {
        snprintf(key, sizeof(key), "key_%d", i);
        CHECK(hashmap_remove(&m, key) == ERR_OK);
    }
    CHECK(hashmap_len(&m) == 100);
    for (int i = 1; i < 200; i += 2) {
        snprintf(key, sizeof(key), "key_%d", i);
        CHECK(hashmap_contains(&m, key));
    }
    CHECK(!hashmap_contains(&m, "key_4"));

    /* Reinsert into tombstoned territory */
    CHECK(hashmap_put(&m, "key_4", &values[4]) == ERR_OK);
    CHECK(hashmap_len(&m) == 101);

    /* Iterator visits every live entry exactly once */
    HashMapIter it = {0};
    const char *k = NULL;
    void       *val = NULL;
    size_t      seen = 0;
    while (hashmap_next(&m, &it, &k, &val)) {
        CHECK(k && val);
        seen++;
    }
    CHECK(seen == 101);

    /* NULL values: present but hashmap_get returns NULL — use contains */
    CHECK(hashmap_put(&m, "null_val", NULL) == ERR_OK);
    CHECK(hashmap_get(&m, "null_val") == NULL);
    CHECK(hashmap_contains(&m, "null_val"));

    hashmap_destroy(&m);
    hashmap_destroy(&m); /* double-destroy is safe */
    hashmap_destroy(NULL);
}

static void test_strbuf(void) {
    StrBuf sb;
    CHECK(strbuf_init(NULL) == ERR_INVALID_ARG);
    CHECK(strbuf_init(&sb) == ERR_OK);
    CHECK(strcmp(strbuf_cstr(&sb), "") == 0);

    CHECK(strbuf_append(&sb, "hello") == ERR_OK);
    CHECK(strbuf_append_char(&sb, ',') == ERR_OK);
    CHECK(strbuf_append_n(&sb, " world!!!", 6) == ERR_OK);
    CHECK(strbuf_appendf(&sb, " x=%d y=%.1f", 42, 3.5) == ERR_OK);
    CHECK(strcmp(strbuf_cstr(&sb), "hello, world x=42 y=3.5") == 0);
    CHECK(sb.len == strlen(strbuf_cstr(&sb)));

    /* Force growth well past the initial capacity */
    strbuf_clear(&sb);
    for (int i = 0; i < 100; i++) CHECK(strbuf_appendf(&sb, "%d,", i) == ERR_OK);
    CHECK(sb.len > 32);
    CHECK(strncmp(strbuf_cstr(&sb), "0,1,2,", 6) == 0);

    /* take: caller owns the string; builder resets to empty */
    char *owned = strbuf_take(&sb);
    CHECK(owned && strncmp(owned, "0,1,2,", 6) == 0);
    CHECK(strcmp(strbuf_cstr(&sb), "") == 0);
    CHECK(strbuf_append(&sb, "reusable") == ERR_OK);
    free(owned);

    CHECK(strbuf_append(&sb, NULL) == ERR_INVALID_ARG);
    strbuf_destroy(&sb);
    strbuf_destroy(&sb);
    strbuf_destroy(NULL);
}

static void test_ringbuf(void) {
    RingBuf r;
    CHECK(ringbuf_init(NULL, 4, 4) == ERR_INVALID_ARG);
    CHECK(ringbuf_init(&r, 0, 4) == ERR_INVALID_ARG);
    CHECK(ringbuf_init(&r, sizeof(int), 4) == ERR_OK);

    int out = -1;
    CHECK(ringbuf_pop(&r, &out) == ERR_NOT_FOUND);

    for (int i = 0; i < 4; i++) CHECK(ringbuf_push(&r, &i) == ERR_OK);
    CHECK(ringbuf_is_full(&r));
    int five = 5;
    CHECK(ringbuf_push(&r, &five) == ERR_OVERFLOW);

    /* FIFO order across wraparound */
    CHECK(ringbuf_pop(&r, &out) == ERR_OK && out == 0);
    CHECK(ringbuf_push(&r, &five) == ERR_OK);
    for (int expect = 1; expect <= 3; expect++) {
        CHECK(ringbuf_pop(&r, &out) == ERR_OK);
        CHECK(out == expect);
    }
    CHECK(ringbuf_pop(&r, &out) == ERR_OK && out == 5);
    CHECK(ringbuf_len(&r) == 0);

    /* Overwrite mode drops the oldest */
    for (int i = 0; i < 6; i++) CHECK(ringbuf_push_overwrite(&r, &i) == ERR_OK);
    CHECK(ringbuf_len(&r) == 4);
    CHECK(ringbuf_pop(&r, &out) == ERR_OK && out == 2); /* 0 and 1 were dropped */

    ringbuf_destroy(&r);
    ringbuf_destroy(&r);
    ringbuf_destroy(NULL);
}

static void test_time(void) {
    uint64_t a = time_now_ns();
    uint64_t b = time_now_ns();
    CHECK(b >= a); /* monotonic */

    Stopwatch sw;
    stopwatch_start(&sw);
    time_sleep_ms(20);
    double elapsed = stopwatch_elapsed_ms(&sw);
    CHECK(elapsed >= 15.0);  /* slept at least roughly the requested time */
    CHECK(elapsed < 5000.0); /* and not absurdly long */
    CHECK(time_now_ms() > 0.0);

    stopwatch_start(NULL); /* safe */
    CHECK(stopwatch_elapsed_ms(NULL) == 0.0);
}

static void test_log(void) {
    /* Capture output via tmpfile to verify filtering and formatting. */
    FILE *f = tmpfile();
    CHECK(f != NULL);
    log_set_stream(f);
    log_set_level(LOG_LEVEL_WARN);
    CHECK(log_get_level() == LOG_LEVEL_WARN);

    LOG_DEBUG("dropped %d", 1);
    LOG_INFO("dropped too");
    LOG_WARN("kept: %s", "warning");
    LOG_ERROR("kept: code %d", 7);
    log_msg(LOG_LEVEL_ERROR, NULL); /* NULL fmt is a safe no-op */

    fflush(f);
    CHECK(fseek(f, 0, SEEK_SET) == 0);
    char   buf[4096] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    CHECK(n > 0);
    CHECK(strstr(buf, "kept: warning") != NULL);
    CHECK(strstr(buf, "kept: code 7") != NULL);
    CHECK(strstr(buf, "WARN") != NULL);
    CHECK(strstr(buf, "ERROR") != NULL);
    CHECK(strstr(buf, "dropped") == NULL);

    /* Exactly two lines survived the level filter */
    int lines = 0;
    for (size_t i = 0; i < n; i++)
        if (buf[i] == '\n') lines++;
    CHECK(lines == 2);

    log_set_stream(NULL); /* back to stderr */
    log_set_level(LOG_LEVEL_INFO);
    (void)fclose(f);
}

int main(void) {
    test_hash();
    test_vec();
    test_hashmap();
    test_strbuf();
    test_ringbuf();
    test_time();
    test_log();
    printf("All containers tests passed.\n");
    return 0;
}
