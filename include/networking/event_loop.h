/**
 * @file event_loop.h
 * @brief poll(2)-based readiness event loop for single-threaded I/O.
 *
 * Register file descriptors with a callback and an interest mask; the loop
 * invokes callbacks as descriptors become readable/writable. One thread runs
 * the loop; callbacks execute on that thread and may add/remove descriptors
 * (including their own) and stop the loop.
 *
 * This scales a server past thread-per-connection (see
 * examples/evloop_server_demo.c). poll(2) is portable POSIX; swapping in
 * epoll/kqueue is a per-platform optimization left to real projects.
 */
#ifndef NETWORKING_EVENT_LOOP_H
#define NETWORKING_EVENT_LOOP_H

#include "core/error.h"
#include <stddef.h>

/** @brief Interest/readiness bits (bitwise-OR them). */
enum {
    EV_READ = 1,  /**< Readable (includes EOF/error — read to find out). */
    EV_WRITE = 2, /**< Writable. */
};

/** @brief Opaque event loop handle. */
typedef struct EventLoop EventLoop;

/**
 * @brief Called when fd is ready.
 * @param fd        The ready descriptor.
 * @param events    Readiness bits (EV_READ and/or EV_WRITE).
 * @param user_data Pointer given to event_loop_add.
 */
typedef void (*EventCallback)(int fd, unsigned int events, void *user_data);

/** @brief Allocate an empty loop. Free with event_loop_destroy. */
ErrorCode event_loop_create(EventLoop **out);

/** @brief Destroy the loop (registered fds are NOT closed — callers own them). */
void event_loop_destroy(EventLoop *loop);

/**
 * @brief Register fd with an interest mask and callback.
 * @return ERR_OK, ERR_INVALID_ARG (bad args or fd already registered),
 *         or ERR_NOMEM.
 */
ErrorCode event_loop_add(EventLoop *loop, int fd, unsigned int events, EventCallback cb,
                         void *user_data);

/** @brief Change the interest mask of a registered fd. */
ErrorCode event_loop_mod(EventLoop *loop, int fd, unsigned int events);

/**
 * @brief Unregister fd (safe from inside a callback; the fd is not closed).
 * @return ERR_OK, ERR_INVALID_ARG, or ERR_NOT_FOUND.
 */
ErrorCode event_loop_remove(EventLoop *loop, int fd);

/**
 * @brief One poll iteration: wait up to timeout_ms, dispatch ready callbacks.
 * @param loop       Loop to poll.
 * @param timeout_ms Max wait in milliseconds; 0 returns immediately,
 *                   -1 waits indefinitely.
 * @return ERR_OK (whether or not events fired), ERR_INVALID_ARG, or ERR_IO.
 */
ErrorCode event_loop_poll_once(EventLoop *loop, int timeout_ms);

/**
 * @brief Run poll iterations until event_loop_stop is called.
 * @return ERR_OK on clean stop, ERR_INVALID_ARG, or ERR_IO on poll failure.
 */
ErrorCode event_loop_run(EventLoop *loop);

/** @brief Request that event_loop_run return (callable from callbacks). */
void event_loop_stop(EventLoop *loop);

/** @brief Number of currently registered descriptors. */
size_t event_loop_count(const EventLoop *loop);

#endif /* NETWORKING_EVENT_LOOP_H */
