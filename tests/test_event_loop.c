/**
 * @file test_event_loop.c
 * @brief Tests for the poll-based event loop (networking/event_loop.h).
 */
#include "networking/event_loop.h"
#include "networking/socket.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CHECK(cond)                                                                    \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

/* ── Read readiness via a pipe ──────────────────────────────────────────── */

typedef struct ReadCtx {
    int          fired;
    unsigned int events;
    char         byte;
} ReadCtx;

static void on_readable(int fd, unsigned int events, void *ud) {
    ReadCtx *ctx = ud;
    ctx->fired++;
    ctx->events = events;
    CHECK(read(fd, &ctx->byte, 1) == 1);
}

static void test_read_event(void) {
    EventLoop *loop = NULL;
    CHECK(event_loop_create(&loop) == ERR_OK);

    int fds[2];
    CHECK(pipe(fds) == 0);

    ReadCtx ctx = {0};
    CHECK(event_loop_add(loop, fds[0], EV_READ, on_readable, &ctx) == ERR_OK);
    CHECK(event_loop_count(loop) == 1);
    CHECK(event_loop_add(loop, fds[0], EV_READ, on_readable, &ctx) == ERR_INVALID_ARG);

    /* Nothing ready: 0-timeout poll fires nothing */
    CHECK(event_loop_poll_once(loop, 0) == ERR_OK);
    CHECK(ctx.fired == 0);

    /* Write → readable exactly once per poll */
    CHECK(write(fds[1], "x", 1) == 1);
    CHECK(event_loop_poll_once(loop, 1000) == ERR_OK);
    CHECK(ctx.fired == 1);
    CHECK(ctx.events & EV_READ);
    CHECK(ctx.byte == 'x');

    /* Consumed: quiet again */
    CHECK(event_loop_poll_once(loop, 0) == ERR_OK);
    CHECK(ctx.fired == 1);

    CHECK(event_loop_remove(loop, fds[0]) == ERR_OK);
    CHECK(event_loop_count(loop) == 0);
    CHECK(event_loop_remove(loop, fds[0]) == ERR_NOT_FOUND);

    close(fds[0]);
    close(fds[1]);
    event_loop_destroy(loop);
    event_loop_destroy(NULL);
}

/* ── Write readiness + mod ──────────────────────────────────────────────── */

static void on_writable(int fd, unsigned int events, void *ud) {
    (void)fd;
    int *count = ud;
    if (events & EV_WRITE) (*count)++;
}

static void test_write_event_and_mod(void) {
    EventLoop *loop = NULL;
    CHECK(event_loop_create(&loop) == ERR_OK);

    int fds[2];
    CHECK(pipe(fds) == 0);

    int writable_count = 0;
    CHECK(event_loop_add(loop, fds[1], EV_WRITE, on_writable, &writable_count) == ERR_OK);

    /* An empty pipe's write end is immediately writable */
    CHECK(event_loop_poll_once(loop, 1000) == ERR_OK);
    CHECK(writable_count == 1);

    /* Switch interest to EV_READ: write end never reads → no more events */
    CHECK(event_loop_mod(loop, fds[1], EV_READ) == ERR_OK);
    CHECK(event_loop_poll_once(loop, 0) == ERR_OK);
    CHECK(writable_count == 1);
    CHECK(event_loop_mod(loop, 999, EV_READ) == ERR_NOT_FOUND);

    close(fds[0]);
    close(fds[1]);
    event_loop_destroy(loop);
}

/* ── Echo server driven entirely by the loop ────────────────────────────── */

typedef struct ServerCtx {
    EventLoop *loop;
    TcpSocket *listener;
    int        clients_served;
} ServerCtx;

static void on_client_data(int fd, unsigned int events, void *ud) {
    ServerCtx *srv = ud;
    (void)events;

    TcpSocket client = {.fd = fd};
    char      buf[256];
    size_t    got = 0;
    if (tcp_recv(&client, buf, sizeof(buf), &got) != ERR_OK || got == 0) {
        CHECK(event_loop_remove(srv->loop, fd) == ERR_OK); /* EOF: remove from inside cb */
        tcp_close(&client);
        srv->clients_served++;
        return;
    }
    CHECK(tcp_send_all(&client, buf, got) == ERR_OK);
}

static void on_accept_ready(int fd, unsigned int events, void *ud) {
    ServerCtx *srv = ud;
    (void)fd;
    (void)events;
    TcpSocket conn;
    CHECK(tcp_accept(srv->listener, &conn) == ERR_OK);
    CHECK(event_loop_add(srv->loop, conn.fd, EV_READ, on_client_data, srv) == ERR_OK);
}

static void test_echo_server_loop(void) {
    EventLoop *loop = NULL;
    CHECK(event_loop_create(&loop) == ERR_OK);

    TcpSocket listener;
    CHECK(tcp_listen(&listener, 0, 4) == ERR_OK);
    uint16_t port = 0;
    CHECK(tcp_local_port(&listener, &port) == ERR_OK);

    ServerCtx srv = {.loop = loop, .listener = &listener};
    CHECK(event_loop_add(loop, listener.fd, EV_READ, on_accept_ready, &srv) == ERR_OK);

    /* Two clients, one round-trip each — the loop multiplexes both. */
    TcpSocket c1, c2;
    CHECK(tcp_connect(&c1, "127.0.0.1", port) == ERR_OK);
    CHECK(tcp_connect(&c2, "127.0.0.1", port) == ERR_OK);
    for (int i = 0; i < 4; i++) CHECK(event_loop_poll_once(loop, 1000) == ERR_OK);
    CHECK(event_loop_count(loop) == 3); /* listener + 2 clients */

    CHECK(tcp_send_all(&c1, "one", 3) == ERR_OK);
    CHECK(tcp_send_all(&c2, "two", 3) == ERR_OK);
    for (int i = 0; i < 4; i++) CHECK(event_loop_poll_once(loop, 1000) == ERR_OK);

    char   buf[16];
    size_t got = 0;
    CHECK(tcp_recv(&c1, buf, sizeof(buf), &got) == ERR_OK && got == 3);
    CHECK(memcmp(buf, "one", 3) == 0);
    CHECK(tcp_recv(&c2, buf, sizeof(buf), &got) == ERR_OK && got == 3);
    CHECK(memcmp(buf, "two", 3) == 0);

    /* Disconnects arrive as EV_READ/EOF; callbacks remove themselves. */
    tcp_close(&c1);
    tcp_close(&c2);
    for (int i = 0; i < 4 && srv.clients_served < 2; i++)
        CHECK(event_loop_poll_once(loop, 1000) == ERR_OK);
    CHECK(srv.clients_served == 2);
    CHECK(event_loop_count(loop) == 1); /* just the listener again */

    tcp_close(&listener);
    event_loop_destroy(loop);
}

/* ── stop() from inside a callback ends run() ───────────────────────────── */

typedef struct StopCtx {
    EventLoop *loop;
    int        drained;
} StopCtx;

static void on_stop_byte(int fd, unsigned int events, void *ud) {
    (void)events;
    StopCtx *ctx = ud;
    char     c;
    CHECK(read(fd, &c, 1) == 1);
    ctx->drained++;
    event_loop_stop(ctx->loop);
}

static void test_run_and_stop(void) {
    EventLoop *loop = NULL;
    CHECK(event_loop_create(&loop) == ERR_OK);

    int fds[2];
    CHECK(pipe(fds) == 0);
    StopCtx ctx = {.loop = loop};
    CHECK(event_loop_add(loop, fds[0], EV_READ, on_stop_byte, &ctx) == ERR_OK);
    CHECK(write(fds[1], "q", 1) == 1);

    CHECK(event_loop_run(loop) == ERR_OK); /* returns because the callback stops it */
    CHECK(ctx.drained == 1);

    close(fds[0]);
    close(fds[1]);
    event_loop_destroy(loop);
}

static void test_invalid_args(void) {
    EventLoop *loop = NULL;
    CHECK(event_loop_create(NULL) == ERR_INVALID_ARG);
    CHECK(event_loop_create(&loop) == ERR_OK);
    CHECK(event_loop_add(loop, -1, EV_READ, on_writable, NULL) == ERR_INVALID_ARG);
    CHECK(event_loop_add(loop, 0, 0, on_writable, NULL) == ERR_INVALID_ARG);
    CHECK(event_loop_add(loop, 0, EV_READ, NULL, NULL) == ERR_INVALID_ARG);
    CHECK(event_loop_poll_once(NULL, 0) == ERR_INVALID_ARG);
    CHECK(event_loop_poll_once(loop, 0) == ERR_OK); /* empty loop is a no-op */
    CHECK(event_loop_run(NULL) == ERR_INVALID_ARG);
    event_loop_stop(NULL); /* safe */
    event_loop_destroy(loop);
}

int main(void) {
    test_read_event();
    test_write_event_and_mod();
    test_echo_server_loop();
    test_run_and_stop();
    test_invalid_args();
    printf("All event loop tests passed.\n");
    return 0;
}
