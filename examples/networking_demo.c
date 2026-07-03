/**
 * @file networking_demo.c
 * @brief Loopback tour of the networking module: TCP, UDP, and Unix sockets.
 *
 * Runs client and server in one process over loopback, so it is fully
 * self-contained. For a real client/server pair running as separate
 * processes, see echo_server_demo.c and echo_client_demo.c.
 */
#include "networking/socket.h"
#include "networking/udp.h"
#include "networking/unix_socket.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define DIE(err, what)                                              \
    do {                                                            \
        fprintf(stderr, "%s failed: %s\n", (what), error_str(err)); \
        return 1;                                                   \
    } while (0)

static int demo_tcp(void) {
    TcpSocket server, client, conn;
    uint16_t  port = 0;

    ErrorCode err = tcp_listen(&server, 0, 4);
    if (err) DIE(err, "tcp_listen");
    err = tcp_local_port(&server, &port);
    if (err) DIE(err, "tcp_local_port");
    err = tcp_connect(&client, "127.0.0.1", port);
    if (err) DIE(err, "tcp_connect");
    err = tcp_accept(&server, &conn);
    if (err) DIE(err, "tcp_accept");

    const char msg[] = "hello over TCP";
    char       buf[64];
    size_t     got = 0;
    err = tcp_send_all(&client, msg, sizeof(msg));
    if (err) DIE(err, "tcp_send_all");
    err = tcp_recv(&conn, buf, sizeof(buf), &got);
    if (err) DIE(err, "tcp_recv");
    printf("TCP  (port %u): received %zu bytes: \"%s\"\n", (unsigned)port, got, buf);

    tcp_close(&conn);
    tcp_close(&client);
    tcp_close(&server);
    return 0;
}

static int demo_udp(void) {
    UdpSocket server, client;
    uint16_t  port = 0;

    ErrorCode err = udp_open(&server, 0);
    if (err) DIE(err, "udp_open(server)");
    err = udp_open(&client, 0);
    if (err) DIE(err, "udp_open(client)");
    err = udp_local_port(&server, &port);
    if (err) DIE(err, "udp_local_port");

    const char  msg[] = "hello over UDP";
    char        buf[64];
    size_t      got = 0;
    UdpEndpoint from;
    err = udp_send_to(&client, "127.0.0.1", port, msg, sizeof(msg));
    if (err) DIE(err, "udp_send_to");
    err = udp_recv_from(&server, buf, sizeof(buf), &got, &from);
    if (err) DIE(err, "udp_recv_from");
    printf("UDP  (port %u): received %zu bytes from %s:%u: \"%s\"\n", (unsigned)port, got,
           from.host, (unsigned)from.port, buf);

    udp_close(&client);
    udp_close(&server);
    return 0;
}

static int demo_unix(void) {
    const char *path = "networking_demo.sock";
    UnixSocket  server, client, conn;

    ErrorCode err = unix_listen(&server, path, 4);
    if (err) DIE(err, "unix_listen");
    err = unix_connect(&client, path);
    if (err) DIE(err, "unix_connect");
    err = unix_accept(&server, &conn);
    if (err) DIE(err, "unix_accept");

    const char msg[] = "hello over Unix socket";
    char       buf[64];
    size_t     got = 0;
    err = unix_send_all(&client, msg, sizeof(msg));
    if (err) DIE(err, "unix_send_all");
    err = unix_recv(&conn, buf, sizeof(buf), &got);
    if (err) DIE(err, "unix_recv");
    printf("UNIX (%s): received %zu bytes: \"%s\"\n", path, got, buf);

    unix_close(&conn);
    unix_close(&client);
    unix_close(&server);
    (void)unlink(path);
    return 0;
}

int main(void) {
    if (demo_tcp() != 0) return 1;
    if (demo_udp() != 0) return 1;
    if (demo_unix() != 0) return 1;
    printf("All transports OK.\n");
    return 0;
}
