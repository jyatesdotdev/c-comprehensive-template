/**
 * @file echo_server_demo.c
 * @brief Concurrent TCP echo server: networking module + hpc thread pool.
 *
 * Each accepted connection is handed to a worker thread, so slow clients
 * don't block new ones. Run it, then connect with echo_client_demo (or
 * `nc localhost 7777`) from another terminal. Stop with Ctrl-C.
 *
 * Usage: example_echo_server [port]   (default 7777, 0 = ephemeral)
 */
#include "hpc/thread_pool.h"
#include "networking/socket.h"

#include <stdio.h>
#include <stdlib.h>

enum { DEFAULT_PORT = 7777, NUM_WORKERS = 4, BACKLOG = 16 };

/** Echo everything a client sends until it disconnects. Owns and frees `arg`. */
static void handle_client(void *arg) {
    TcpSocket *client = arg;
    char       buf[1024];
    size_t     got = 0;

    while (tcp_recv(client, buf, sizeof(buf), &got) == ERR_OK && got > 0) {
        if (tcp_send_all(client, buf, got) != ERR_OK) break;
    }
    tcp_close(client);
    free(client);
}

int main(int argc, char **argv) {
    uint16_t port = DEFAULT_PORT;
    if (argc > 1) port = (uint16_t)strtoul(argv[1], NULL, 10);

    TcpSocket listener;
    ErrorCode err = tcp_listen(&listener, port, BACKLOG);
    if (err) {
        fprintf(stderr, "tcp_listen failed: %s\n", error_str(err));
        return 1;
    }

    uint16_t bound = 0;
    (void)tcp_local_port(&listener, &bound);

    ThreadPool *pool = NULL;
    err = thread_pool_create(&pool, NUM_WORKERS);
    if (err) {
        fprintf(stderr, "thread_pool_create failed: %s\n", error_str(err));
        tcp_close(&listener);
        return 1;
    }

    printf("Echo server listening on port %u (%d workers). Ctrl-C to stop.\n", (unsigned)bound,
           NUM_WORKERS);
    printf("Try: ./example_echo_client 127.0.0.1 %u \"hello\"\n", (unsigned)bound);

    for (;;) {
        TcpSocket *client = malloc(sizeof(*client));
        if (!client) break;

        if (tcp_accept(&listener, client) != ERR_OK) {
            free(client);
            break;
        }
        if (thread_pool_submit(pool, handle_client, client) != ERR_OK) {
            tcp_close(client);
            free(client);
            break;
        }
    }

    thread_pool_destroy(pool);
    tcp_close(&listener);
    return 0;
}
