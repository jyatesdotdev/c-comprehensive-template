/**
 * @file test_networking.c
 * @brief Tests for the networking module: TCP, UDP, and Unix domain sockets.
 *
 * All network tests run over loopback / local paths — no external network.
 */
#include "networking/socket.h"
#include "networking/udp.h"
#include "networking/unix_socket.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define UNIX_TEST_PATH "test_networking_unix.sock"

static void test_tcp_invalid_args(void) {
    TcpSocket s = {.fd = -1};
    uint16_t  port = 0;
    size_t    n = 0;
    char      buf[8];

    assert(tcp_connect(NULL, "127.0.0.1", 80) == ERR_INVALID_ARG);
    assert(tcp_connect(&s, NULL, 80) == ERR_INVALID_ARG);
    assert(tcp_connect(&s, "127.0.0.1", 0) == ERR_INVALID_ARG);
    assert(tcp_listen(NULL, 0, 4) == ERR_INVALID_ARG);
    assert(tcp_listen(&s, 0, 0) == ERR_INVALID_ARG);
    s.fd = -1;
    assert(tcp_accept(&s, &s) == ERR_INVALID_ARG);
    assert(tcp_local_port(&s, &port) == ERR_INVALID_ARG);
    assert(tcp_send_all(&s, buf, 4) == ERR_INVALID_ARG);
    assert(tcp_recv(&s, buf, sizeof(buf), &n) == ERR_INVALID_ARG);
    tcp_close(&s);   /* safe on already-invalid socket */
    tcp_close(NULL); /* safe on NULL */
}

static void test_tcp_loopback_roundtrip(void) {
    TcpSocket server, client, conn;
    assert(tcp_listen(&server, 0, 4) == ERR_OK);

    uint16_t port = 0;
    assert(tcp_local_port(&server, &port) == ERR_OK);
    assert(port != 0);

    assert(tcp_connect(&client, "127.0.0.1", port) == ERR_OK);
    assert(tcp_accept(&server, &conn) == ERR_OK);

    const char msg[] = "ping";
    assert(tcp_send_all(&client, msg, sizeof(msg)) == ERR_OK);

    char   buf[16];
    size_t total = 0;
    while (total < sizeof(msg)) {
        size_t got = 0;
        assert(tcp_recv(&conn, buf + total, sizeof(buf) - total, &got) == ERR_OK);
        assert(got > 0);
        total += got;
    }
    assert(total == sizeof(msg));
    assert(memcmp(buf, msg, total) == 0);

    /* echo back */
    assert(tcp_send_all(&conn, buf, total) == ERR_OK);
    size_t got = 0;
    assert(tcp_recv(&client, buf, sizeof(buf), &got) == ERR_OK);
    assert(got > 0);

    /* clean EOF after peer closes */
    tcp_close(&client);
    size_t eof_len = 99;
    assert(tcp_recv(&conn, buf, sizeof(buf), &eof_len) == ERR_OK);
    assert(eof_len == 0);

    tcp_close(&conn);
    tcp_close(&server);
}

static void test_udp_invalid_args(void) {
    UdpSocket s = {.fd = -1};
    uint16_t  port = 0;
    size_t    n = 0;
    char      buf[8];

    assert(udp_open(NULL, 0) == ERR_INVALID_ARG);
    assert(udp_local_port(&s, &port) == ERR_INVALID_ARG);
    assert(udp_send_to(&s, "127.0.0.1", 80, buf, 4) == ERR_INVALID_ARG);
    assert(udp_recv_from(&s, buf, sizeof(buf), &n, NULL) == ERR_INVALID_ARG);
    udp_close(&s);
    udp_close(NULL);
}

static void test_udp_loopback_roundtrip(void) {
    UdpSocket server, client;
    assert(udp_open(&server, 0) == ERR_OK);
    assert(udp_open(&client, 0) == ERR_OK);

    uint16_t server_port = 0;
    assert(udp_local_port(&server, &server_port) == ERR_OK);
    assert(server_port != 0);

    const char msg[] = "datagram";
    assert(udp_send_to(&client, "127.0.0.1", server_port, msg, sizeof(msg)) == ERR_OK);

    char        buf[32];
    size_t      got = 0;
    UdpEndpoint from;
    assert(udp_recv_from(&server, buf, sizeof(buf), &got, &from) == ERR_OK);
    assert(got == sizeof(msg));
    assert(memcmp(buf, msg, got) == 0);
    assert(from.port != 0);
    assert(from.host[0] != '\0');

    /* reply to the sender using the reported endpoint */
    assert(udp_send_to(&server, from.host, from.port, "ack", 4) == ERR_OK);
    assert(udp_recv_from(&client, buf, sizeof(buf), &got, NULL) == ERR_OK);
    assert(got == 4);
    assert(memcmp(buf, "ack", 4) == 0);

    udp_close(&client);
    udp_close(&server);
}

static void test_unix_invalid_args(void) {
    UnixSocket s = {.fd = -1};
    size_t     n = 0;
    char       buf[8];
    char       long_path[256];

    assert(unix_listen(NULL, UNIX_TEST_PATH, 4) == ERR_INVALID_ARG);
    assert(unix_listen(&s, "", 4) == ERR_INVALID_ARG);
    assert(unix_listen(&s, UNIX_TEST_PATH, 0) == ERR_INVALID_ARG);
    assert(unix_connect(&s, NULL) == ERR_INVALID_ARG);
    memset(long_path, 'a', sizeof(long_path) - 1);
    long_path[sizeof(long_path) - 1] = '\0';
    assert(unix_listen(&s, long_path, 4) == ERR_INVALID_ARG);
    assert(unix_connect(&s, long_path) == ERR_INVALID_ARG);
    s.fd = -1;
    assert(unix_accept(&s, &s) == ERR_INVALID_ARG);
    assert(unix_send_all(&s, buf, 4) == ERR_INVALID_ARG);
    assert(unix_recv(&s, buf, sizeof(buf), &n) == ERR_INVALID_ARG);
    unix_close(&s);
    unix_close(NULL);
}

static void test_unix_roundtrip(void) {
    UnixSocket server, client, conn;
    assert(unix_listen(&server, UNIX_TEST_PATH, 4) == ERR_OK);
    assert(unix_connect(&client, UNIX_TEST_PATH) == ERR_OK);
    assert(unix_accept(&server, &conn) == ERR_OK);

    const char msg[] = "local ipc";
    assert(unix_send_all(&client, msg, sizeof(msg)) == ERR_OK);

    char   buf[32];
    size_t total = 0;
    while (total < sizeof(msg)) {
        size_t got = 0;
        assert(unix_recv(&conn, buf + total, sizeof(buf) - total, &got) == ERR_OK);
        assert(got > 0);
        total += got;
    }
    assert(memcmp(buf, msg, sizeof(msg)) == 0);

    /* clean EOF after peer closes */
    unix_close(&client);
    size_t eof_len = 99;
    assert(unix_recv(&conn, buf, sizeof(buf), &eof_len) == ERR_OK);
    assert(eof_len == 0);

    unix_close(&conn);
    unix_close(&server);
    (void)unlink(UNIX_TEST_PATH);

    /* connecting to a removed path fails with ERR_IO */
    assert(unix_connect(&client, UNIX_TEST_PATH) == ERR_IO);
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
