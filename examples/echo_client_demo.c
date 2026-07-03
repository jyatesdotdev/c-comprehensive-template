/**
 * @file echo_client_demo.c
 * @brief TCP echo client — pairs with echo_server_demo.
 *
 * Usage: example_echo_client [host] [port] [message]
 *        (defaults: 127.0.0.1 7777 "hello from echo client")
 */
#include "networking/socket.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    const char *host = argc > 1 ? argv[1] : "127.0.0.1";
    uint16_t    port = argc > 2 ? (uint16_t)strtoul(argv[2], NULL, 10) : 7777;
    const char *msg = argc > 3 ? argv[3] : "hello from echo client";

    TcpSocket s;
    ErrorCode err = tcp_connect(&s, host, port);
    if (err) {
        fprintf(stderr, "tcp_connect to %s:%u failed: %s\n", host, (unsigned)port, error_str(err));
        return 1;
    }

    size_t len = strlen(msg);
    err = tcp_send_all(&s, msg, len);
    if (err) {
        fprintf(stderr, "tcp_send_all failed: %s\n", error_str(err));
        tcp_close(&s);
        return 1;
    }

    char   buf[1024];
    size_t total = 0;
    while (total < len) {
        size_t got = 0;
        err = tcp_recv(&s, buf + total, sizeof(buf) - 1 - total, &got);
        if (err != ERR_OK || got == 0) break;
        total += got;
    }
    buf[total] = '\0';
    printf("sent:   \"%s\"\nechoed: \"%s\"\n", msg, buf);

    tcp_close(&s);
    return total == len ? 0 : 1;
}
