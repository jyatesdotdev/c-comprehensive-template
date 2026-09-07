/**
 * @file udp.c
 * @brief UDP datagram sockets (POSIX implementation, IPv4).
 */
#include "networking/udp.h"
#include "socket_internal.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

ErrorCode udp_open_host(UdpSocket *s, const char *host, uint16_t port) {
    if (!s || !host || !host[0]) return ERR_INVALID_ARG;
    s->fd = -1;

    char portstr[6];
    snprintf(portstr, sizeof(portstr), "%u", (unsigned)port);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = (strchr(host, ':') != NULL) ? AF_INET6 : AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    const char *node = (strcmp(host, "*") == 0 || strcmp(host, "0.0.0.0") == 0) ? NULL : host;
    if (!node) hints.ai_family = AF_INET;
    struct addrinfo *res = NULL;
    if (getaddrinfo(node, portstr, &hints, &res) != 0 || !res) return ERR_NOT_FOUND;

    int fd = nw_socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) {
        freeaddrinfo(res);
        return ERR_IO;
    }
    if (bind(fd, res->ai_addr, res->ai_addrlen) != 0) {
        close(fd);
        freeaddrinfo(res);
        return ERR_IO;
    }
    freeaddrinfo(res);
    s->fd = fd;
    return ERR_OK;
}

ErrorCode udp_open(UdpSocket *s, uint16_t port) {
    return udp_open_host(s, "*", port);
}

ErrorCode udp_local_port(const UdpSocket *s, uint16_t *out_port) {
    if (!s || s->fd < 0 || !out_port) return ERR_INVALID_ARG;

    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    if (getsockname(s->fd, (struct sockaddr *)&addr, &len) != 0) return ERR_IO;
    *out_port = ntohs(addr.sin_port);
    return ERR_OK;
}

ErrorCode udp_send_to(const UdpSocket *s, const char *host, uint16_t port, const void *buf,
                      size_t len) {
    if (!s || s->fd < 0 || !host || port == 0 || !buf || len == 0) return ERR_INVALID_ARG;

    char portstr[6];
    snprintf(portstr, sizeof(portstr), "%u", (unsigned)port);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; /* socket is AF_INET — resolve to IPv4 only */
    hints.ai_socktype = SOCK_DGRAM;

    struct addrinfo *res = NULL;
    if (getaddrinfo(host, portstr, &hints, &res) != 0 || !res) return ERR_NOT_FOUND;

    ssize_t n;
    do {
        n = sendto(s->fd, buf, len, 0, res->ai_addr, res->ai_addrlen);
    } while (n < 0 && errno == EINTR);
    freeaddrinfo(res);

    return (n < 0 || (size_t)n != len) ? ERR_IO : ERR_OK;
}

ErrorCode udp_recv_from(const UdpSocket *s, void *buf, size_t cap, size_t *out_len,
                        UdpEndpoint *out_from) {
    if (!s || s->fd < 0 || !buf || cap == 0 || !out_len) return ERR_INVALID_ARG;
    *out_len = 0;

    struct sockaddr_in from;
    socklen_t from_len = sizeof(from);
    ssize_t n;
    do {
        n = recvfrom(s->fd, buf, cap, 0, (struct sockaddr *)&from, &from_len);
    } while (n < 0 && errno == EINTR);
    if (n < 0) return ERR_IO;

    *out_len = (size_t)n;
    if (out_from) {
        out_from->host[0] = '\0';
        (void)inet_ntop(AF_INET, &from.sin_addr, out_from->host, sizeof(out_from->host));
        out_from->port = ntohs(from.sin_port);
    }
    return ERR_OK;
}

void udp_close(UdpSocket *s) {
    if (s && s->fd >= 0) {
        close(s->fd);
        s->fd = -1;
    }
}
