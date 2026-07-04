/**
 * @file event_loop.c
 * @brief poll(2)-based event loop implementation.
 *
 * Registrations live in a parallel pair of arrays (entries + pollfds) kept
 * in index lockstep. Removal during dispatch tombstones the slot (fd = -1)
 * instead of moving entries, so the dispatch walk stays valid; tombstones
 * are compacted after each dispatch round.
 */
#include "networking/event_loop.h"

#include <poll.h>
#include <stdlib.h>

typedef struct EventEntry {
    int           fd;
    unsigned int  events;
    EventCallback cb;
    void         *user_data;
} EventEntry;

struct EventLoop {
    EventEntry    *entries;
    struct pollfd *pollfds;
    size_t         len;
    size_t         cap;
    int            running;
    int            dispatching; /* nonzero while callbacks run (defers compaction) */
};

static short interest_to_poll(unsigned int events) {
    short p = 0;
    if (events & EV_READ) p |= POLLIN;
    if (events & EV_WRITE) p |= POLLOUT;
    return p;
}

static unsigned int poll_to_ready(short revents) {
    unsigned int e = 0;
    /* HUP/ERR surface as readable: the owner reads to observe EOF/error. */
    if (revents & (POLLIN | POLLHUP | POLLERR)) e |= EV_READ;
    if (revents & POLLOUT) e |= EV_WRITE;
    return e;
}

static long find_index(const EventLoop *loop, int fd) {
    for (size_t i = 0; i < loop->len; i++)
        if (loop->entries[i].fd == fd) return (long)i;
    return -1;
}

ErrorCode event_loop_create(EventLoop **out) {
    if (!out) return ERR_INVALID_ARG;
    EventLoop *loop = calloc(1, sizeof(*loop));
    if (!loop) return ERR_NOMEM;
    *out = loop;
    return ERR_OK;
}

void event_loop_destroy(EventLoop *loop) {
    if (!loop) return;
    free(loop->entries);
    free(loop->pollfds);
    free(loop);
}

ErrorCode event_loop_add(EventLoop *loop, int fd, unsigned int events, EventCallback cb,
                         void *user_data) {
    if (!loop || fd < 0 || !cb || events == 0) return ERR_INVALID_ARG;
    if (find_index(loop, fd) >= 0) return ERR_INVALID_ARG; /* already registered */

    if (loop->len == loop->cap) {
        size_t      new_cap = loop->cap == 0 ? 8 : loop->cap * 2;
        EventEntry *e = realloc(loop->entries, new_cap * sizeof(EventEntry));
        if (!e) return ERR_NOMEM;
        loop->entries = e;
        struct pollfd *p = realloc(loop->pollfds, new_cap * sizeof(struct pollfd));
        if (!p) return ERR_NOMEM; /* entries realloc is kept — harmless */
        loop->pollfds = p;
        loop->cap = new_cap;
    }

    loop->entries[loop->len] =
        (EventEntry){.fd = fd, .events = events, .cb = cb, .user_data = user_data};
    loop->len++;
    return ERR_OK;
}

ErrorCode event_loop_mod(EventLoop *loop, int fd, unsigned int events) {
    if (!loop || fd < 0 || events == 0) return ERR_INVALID_ARG;
    long i = find_index(loop, fd);
    if (i < 0) return ERR_NOT_FOUND;
    loop->entries[i].events = events;
    return ERR_OK;
}

ErrorCode event_loop_remove(EventLoop *loop, int fd) {
    if (!loop || fd < 0) return ERR_INVALID_ARG;
    long i = find_index(loop, fd);
    if (i < 0) return ERR_NOT_FOUND;

    if (loop->dispatching) {
        loop->entries[i].fd = -1; /* tombstone — compacted after dispatch */
    } else {
        loop->entries[i] = loop->entries[loop->len - 1];
        loop->len--;
    }
    return ERR_OK;
}

/** Drop tombstoned entries after a dispatch round. */
static void compact(EventLoop *loop) {
    size_t kept = 0;
    for (size_t i = 0; i < loop->len; i++) {
        if (loop->entries[i].fd >= 0) loop->entries[kept++] = loop->entries[i];
    }
    loop->len = kept;
}

ErrorCode event_loop_poll_once(EventLoop *loop, int timeout_ms) {
    if (!loop) return ERR_INVALID_ARG;
    if (loop->len == 0) return ERR_OK;

    for (size_t i = 0; i < loop->len; i++) {
        loop->pollfds[i].fd = loop->entries[i].fd;
        loop->pollfds[i].events = interest_to_poll(loop->entries[i].events);
        loop->pollfds[i].revents = 0;
    }

    int ready = poll(loop->pollfds, (nfds_t)loop->len, timeout_ms);
    if (ready < 0) return ERR_IO;
    if (ready == 0) return ERR_OK; /* timeout */

    /* Dispatch over the pre-poll count: callbacks may append new entries,
       but those only participate from the next iteration. */
    size_t count = loop->len;
    loop->dispatching = 1;
    for (size_t i = 0; i < count; i++) {
        if (loop->entries[i].fd < 0) continue; /* removed by an earlier callback */
        unsigned int e = poll_to_ready(loop->pollfds[i].revents);
        if (e) loop->entries[i].cb(loop->entries[i].fd, e, loop->entries[i].user_data);
    }
    loop->dispatching = 0;
    compact(loop);
    return ERR_OK;
}

ErrorCode event_loop_run(EventLoop *loop) {
    if (!loop) return ERR_INVALID_ARG;
    loop->running = 1;
    while (loop->running) {
        ErrorCode err = event_loop_poll_once(loop, 100);
        if (err != ERR_OK) {
            loop->running = 0;
            return err;
        }
    }
    return ERR_OK;
}

void event_loop_stop(EventLoop *loop) {
    if (loop) loop->running = 0;
}

size_t event_loop_count(const EventLoop *loop) {
    return loop ? loop->len : 0;
}
