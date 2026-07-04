/**
 * @file evloop_server_demo.c
 * @brief Single-threaded TCP echo server on the poll-based event loop.
 *
 * Contrast with echo_server_demo.c (thread pool, one task per client): here
 * ONE thread multiplexes the listener and every client via readiness events.
 * No locks, no per-connection threads — this is the C10K-era pattern that
 * epoll/kqueue refined.
 *
 * Usage: example_evloop_server [port]   (default 7778, 0 = ephemeral)
 * Try:   ./example_echo_client 127.0.0.1 7778 "hello event loop"
 */
#include "core/log.h"
#include "networking/event_loop.h"
#include "networking/socket.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct Server {
    EventLoop *loop;
    TcpSocket *listener;
} Server;

static void on_client(int fd, unsigned int events, void *ud) {
    Server *srv = ud;
    (void)events;

    TcpSocket client = {.fd = fd};
    char      buf[1024];
    size_t    got = 0;
    if (tcp_recv(&client, buf, sizeof(buf), &got) != ERR_OK || got == 0) {
        /* Removal is deferred during dispatch, so don't report counts here. */
        (void)event_loop_remove(srv->loop, fd);
        tcp_close(&client);
        LOG_INFO("client fd=%d disconnected", fd);
        return;
    }
    if (tcp_send_all(&client, buf, got) != ERR_OK) {
        (void)event_loop_remove(srv->loop, fd);
        tcp_close(&client);
    }
}

static void on_accept(int fd, unsigned int events, void *ud) {
    Server *srv = ud;
    (void)fd;
    (void)events;

    TcpSocket conn;
    if (tcp_accept(srv->listener, &conn) != ERR_OK) return;
    if (event_loop_add(srv->loop, conn.fd, EV_READ, on_client, srv) != ERR_OK) {
        tcp_close(&conn);
        return;
    }
    LOG_INFO("client fd=%d connected (%zu connected)", conn.fd, event_loop_count(srv->loop) - 1);
}

int main(int argc, char **argv) {
    uint16_t port = 7778;
    if (argc > 1) port = (uint16_t)strtoul(argv[1], NULL, 10);

    TcpSocket listener;
    ErrorCode err = tcp_listen(&listener, port, 16);
    if (err) {
        fprintf(stderr, "tcp_listen failed: %s\n", error_str(err));
        return 1;
    }
    uint16_t bound = 0;
    (void)tcp_local_port(&listener, &bound);

    EventLoop *loop = NULL;
    if (event_loop_create(&loop) != ERR_OK) {
        tcp_close(&listener);
        return 1;
    }

    Server srv = {.loop = loop, .listener = &listener};
    if (event_loop_add(loop, listener.fd, EV_READ, on_accept, &srv) != ERR_OK) {
        event_loop_destroy(loop);
        tcp_close(&listener);
        return 1;
    }

    printf("Event-loop echo server on port %u (single thread). Ctrl-C to stop.\n", (unsigned)bound);
    err = event_loop_run(loop);

    event_loop_destroy(loop);
    tcp_close(&listener);
    return err == ERR_OK ? 0 : 1;
}
