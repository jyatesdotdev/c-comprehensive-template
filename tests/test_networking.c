/**
 * @file test_networking.c
 * @brief Tests for the networking module: TCP, UDP, and Unix domain sockets.
 *
 * All network tests run over loopback / local paths — no external network.
 *
 * Uses a local CHECK macro instead of assert(): Release builds define NDEBUG,
 * which compiles assert() bodies away entirely — any test logic (or loop
 * progress) inside an assert would silently disappear.
 */
#include "networking/socket.h"
#include "networking/udp.h"
#include "networking/unix_socket.h"

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

#define UNIX_TEST_PATH "test_networking_unix.sock"

static void test_tcp_invalid_args(void) {
    TcpSocket s = {.fd = -1};
    uint16_t  port = 0;
    size_t    n = 0;
    char      buf[8];

    CHECK(tcp_connect(NULL, "127.0.0.1", 80) == ERR_INVALID_ARG);
    CHECK(tcp_connect(&s, NULL, 80) == ERR_INVALID_ARG);
    CHECK(tcp_connect(&s, "127.0.0.1", 0) == ERR_INVALID_ARG);
    CHECK(tcp_listen(NULL, 0, 4) == ERR_INVALID_ARG);
    CHECK(tcp_listen(&s, 0, 0) == ERR_INVALID_ARG);
    s.fd = -1;
    CHECK(tcp_accept(&s, &s) == ERR_INVALID_ARG);
    CHECK(tcp_local_port(&s, &port) == ERR_INVALID_ARG);
    CHECK(tcp_send_all(&s, buf, 4) == ERR_INVALID_ARG);
    CHECK(tcp_recv(&s, buf, sizeof(buf), &n) == ERR_INVALID_ARG);
    tcp_close(&s);   /* safe on already-invalid socket */
    tcp_close(NULL); /* safe on NULL */
}

static void test_tcp_loopback_roundtrip(void) {
    TcpSocket server, client, conn;
    CHECK(tcp_listen(&server, 0, 4) == ERR_OK);

    uint16_t port = 0;
    CHECK(tcp_local_port(&server, &port) == ERR_OK);
    CHECK(port != 0);

    CHECK(tcp_connect(&client, "127.0.0.1", port) == ERR_OK);
    CHECK(tcp_accept(&server, &conn) == ERR_OK);

    const char msg[] = "ping";
    CHECK(tcp_send_all(&client, msg, sizeof(msg)) == ERR_OK);

    char   buf[16];
    size_t total = 0;
    while (total < sizeof(msg)) {
        size_t got = 0;
        CHECK(tcp_recv(&conn, buf + total, sizeof(buf) - total, &got) == ERR_OK);
        CHECK(got > 0);
        total += got;
    }
    CHECK(total == sizeof(msg));
    CHECK(memcmp(buf, msg, total) == 0);

    /* echo back */
    CHECK(tcp_send_all(&conn, buf, total) == ERR_OK);
    size_t got = 0;
    CHECK(tcp_recv(&client, buf, sizeof(buf), &got) == ERR_OK);
    CHECK(got > 0);

    /* clean EOF after peer closes */
    tcp_close(&client);
    size_t eof_len = 99;
    CHECK(tcp_recv(&conn, buf, sizeof(buf), &eof_len) == ERR_OK);
    CHECK(eof_len == 0);

    tcp_close(&conn);
    tcp_close(&server);
}

static void test_udp_invalid_args(void) {
    UdpSocket s = {.fd = -1};
    uint16_t  port = 0;
    size_t    n = 0;
    char      buf[8];

    CHECK(udp_open(NULL, 0) == ERR_INVALID_ARG);
    CHECK(udp_local_port(&s, &port) == ERR_INVALID_ARG);
    CHECK(udp_send_to(&s, "127.0.0.1", 80, buf, 4) == ERR_INVALID_ARG);
    CHECK(udp_recv_from(&s, buf, sizeof(buf), &n, NULL) == ERR_INVALID_ARG);
    udp_close(&s);
    udp_close(NULL);
}

static void test_udp_loopback_roundtrip(void) {
    UdpSocket server, client;
    CHECK(udp_open(&server, 0) == ERR_OK);
    CHECK(udp_open(&client, 0) == ERR_OK);

    uint16_t server_port = 0;
    CHECK(udp_local_port(&server, &server_port) == ERR_OK);
    CHECK(server_port != 0);

    const char msg[] = "datagram";
    CHECK(udp_send_to(&client, "127.0.0.1", server_port, msg, sizeof(msg)) == ERR_OK);

    char        buf[32];
    size_t      got = 0;
    UdpEndpoint from;
    CHECK(udp_recv_from(&server, buf, sizeof(buf), &got, &from) == ERR_OK);
    CHECK(got == sizeof(msg));
    CHECK(memcmp(buf, msg, got) == 0);
    CHECK(from.port != 0);
    CHECK(from.host[0] != '\0');

    /* reply to the sender using the reported endpoint */
    CHECK(udp_send_to(&server, from.host, from.port, "ack", 4) == ERR_OK);
    CHECK(udp_recv_from(&client, buf, sizeof(buf), &got, NULL) == ERR_OK);
    CHECK(got == 4);
    CHECK(memcmp(buf, "ack", 4) == 0);

    udp_close(&client);
    udp_close(&server);
}

static void test_unix_invalid_args(void) {
    UnixSocket s = {.fd = -1};
    size_t     n = 0;
    char       buf[8];
    char       long_path[256];

    CHECK(unix_listen(NULL, UNIX_TEST_PATH, 4) == ERR_INVALID_ARG);
    CHECK(unix_listen(&s, "", 4) == ERR_INVALID_ARG);
    CHECK(unix_listen(&s, UNIX_TEST_PATH, 0) == ERR_INVALID_ARG);
    CHECK(unix_connect(&s, NULL) == ERR_INVALID_ARG);
    memset(long_path, 'a', sizeof(long_path) - 1);
    long_path[sizeof(long_path) - 1] = '\0';
    CHECK(unix_listen(&s, long_path, 4) == ERR_INVALID_ARG);
    CHECK(unix_connect(&s, long_path) == ERR_INVALID_ARG);
    s.fd = -1;
    CHECK(unix_accept(&s, &s) == ERR_INVALID_ARG);
    CHECK(unix_send_all(&s, buf, 4) == ERR_INVALID_ARG);
    CHECK(unix_recv(&s, buf, sizeof(buf), &n) == ERR_INVALID_ARG);
    unix_close(&s);
    unix_close(NULL);
}

static void test_unix_roundtrip(void) {
    UnixSocket server, client, conn;
    CHECK(unix_listen(&server, UNIX_TEST_PATH, 4) == ERR_OK);
    CHECK(unix_connect(&client, UNIX_TEST_PATH) == ERR_OK);
    CHECK(unix_accept(&server, &conn) == ERR_OK);

    const char msg[] = "local ipc";
    CHECK(unix_send_all(&client, msg, sizeof(msg)) == ERR_OK);

    char   buf[32];
    size_t total = 0;
    while (total < sizeof(msg)) {
        size_t got = 0;
        CHECK(unix_recv(&conn, buf + total, sizeof(buf) - total, &got) == ERR_OK);
        CHECK(got > 0);
        total += got;
    }
    CHECK(memcmp(buf, msg, sizeof(msg)) == 0);

    /* clean EOF after peer closes */
    unix_close(&client);
    size_t eof_len = 99;
    CHECK(unix_recv(&conn, buf, sizeof(buf), &eof_len) == ERR_OK);
    CHECK(eof_len == 0);

    unix_close(&conn);
    unix_close(&server);
    (void)unlink(UNIX_TEST_PATH);

    /* connecting to a removed path fails with ERR_IO */
    CHECK(unix_connect(&client, UNIX_TEST_PATH) == ERR_IO);
}

int main(void) {
    test_tcp_invalid_args();
    test_tcp_loopback_roundtrip();
    test_udp_invalid_args();
    test_udp_loopback_roundtrip();
    test_unix_invalid_args();
    test_unix_roundtrip();
    printf("All networking tests passed.\n");
    return 0;
}
