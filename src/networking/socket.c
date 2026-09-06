/**
 * @file socket.c
 * @brief TCP socket abstraction (POSIX) + shared fd-level helpers.
 */
#include "networking/socket.h"
#include "socket_internal.h"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0 /* not on macOS — SIGPIPE is disabled per-socket instead */
#endif

/* ── Shared helpers (used by TCP and Unix domain sockets) ───────────────── */

int nw_socket(int domain, int type, int protocol) {
#ifdef SOCK_CLOEXEC
    int fd = socket(domain, type | SOCK_CLOEXEC, protocol);
    if (fd >= 0) return fd;
        /* Some kernels reject SOCK_CLOEXEC; fall through to fcntl. */
#endif
    int fd = socket(domain, type, protocol);
    if (fd >= 0) (void)fcntl(fd, F_SETFD, FD_CLOEXEC);
    return fd;
}

void nw_disable_sigpipe(int fd) {
#ifdef SO_NOSIGPIPE
    int one = 1;
    (void)setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#else
    (void)fd;
#endif
}

int nw_accept_intr(int listen_fd) {
    int fd;
    do { fd = accept(listen_fd, NULL, NULL); } while (fd < 0 && errno == EINTR);
    if (fd >= 0) nw_disable_sigpipe(fd);
    return fd;
}

ErrorCode nw_send_all(int fd, const void *buf, size_t len) {
    const unsigned char *p = buf;
    size_t               sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, p + sent, len - sent, MSG_NOSIGNAL);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) continue;
            return ERR_IO;
        }
        sent += (size_t)n;
    }
    return ERR_OK;
}

ErrorCode nw_recv(int fd, void *buf, size_t cap, size_t *out_len) {
    ssize_t n;
    do { n = recv(fd, buf, cap, 0); } while (n < 0 && errno == EINTR);
    if (n < 0) return ERR_IO;
    *out_len = (size_t)n;
    return ERR_OK;
}

/* ── TCP API ────────────────────────────────────────────────────────────── */

ErrorCode tcp_connect(TcpSocket *s, const char *host, uint16_t port) {
    if (!s || !host || port == 0) return ERR_INVALID_ARG;
    s->fd = -1;

    char portstr[6];
    snprintf(portstr, sizeof(portstr), "%u", (unsigned)port);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *res = NULL;
    if (getaddrinfo(host, portstr, &hints, &res) != 0 || !res) return ERR_NOT_FOUND;

    ErrorCode err = ERR_IO;
    for (const struct addrinfo *ai = res; ai; ai = ai->ai_next) {
        int fd = nw_socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0) continue;
        if (connect(fd, ai->ai_addr, ai->ai_addrlen) == 0) {
            nw_disable_sigpipe(fd);
            s->fd = fd;
            err = ERR_OK;
            break;
        }
        close(fd);
    }
    freeaddrinfo(res);
    return err;
}

ErrorCode tcp_listen_host(TcpSocket *s, const char *host, uint16_t port, int backlog) {
    if (!s || !host || !host[0] || backlog <= 0) return ERR_INVALID_ARG;
    s->fd = -1;

    char portstr[6];
    snprintf(portstr, sizeof(portstr), "%u", (unsigned)port);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = (strchr(host, ':') != NULL) ? AF_INET6 : AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    const char *node = (strcmp(host, "*") == 0 || strcmp(host, "0.0.0.0") == 0) ? NULL : host;
    if (!node) hints.ai_family = AF_INET; /* "*" stays IPv4 so 127.0.0.1 tests keep working */
    struct addrinfo *res = NULL;
    if (getaddrinfo(node, portstr, &hints, &res) != 0 || !res) return ERR_NOT_FOUND;

    int fd = nw_socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) {
        freeaddrinfo(res);
        return ERR_IO;
    }
    int one = 1;
    (void)setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    nw_disable_sigpipe(fd);
    if (bind(fd, res->ai_addr, res->ai_addrlen) != 0 || listen(fd, backlog) != 0) {
        close(fd);
        freeaddrinfo(res);
        return ERR_IO;
    }
    freeaddrinfo(res);
    s->fd = fd;
    return ERR_OK;
}

ErrorCode tcp_listen(TcpSocket *s, uint16_t port, int backlog) {
    return tcp_listen_host(s, "*", port, backlog);
}

ErrorCode tcp_accept(const TcpSocket *listener, TcpSocket *out_client) {
    if (!listener || listener->fd < 0 || !out_client) return ERR_INVALID_ARG;
    out_client->fd = nw_accept_intr(listener->fd);
    return out_client->fd < 0 ? ERR_IO : ERR_OK;
}

ErrorCode tcp_local_port(const TcpSocket *s, uint16_t *out_port) {
    if (!s || s->fd < 0 || !out_port) return ERR_INVALID_ARG;

    struct sockaddr_storage ss;
    socklen_t               len = sizeof(ss);
    if (getsockname(s->fd, (struct sockaddr *)&ss, &len) != 0) return ERR_IO;

    if (ss.ss_family == AF_INET) {
        *out_port = ntohs(((const struct sockaddr_in *)&ss)->sin_port);
    } else if (ss.ss_family == AF_INET6) {
        *out_port = ntohs(((const struct sockaddr_in6 *)&ss)->sin6_port);
    } else {
        return ERR_UNSUPPORTED;
    }
    return ERR_OK;
}

ErrorCode tcp_send_all(const TcpSocket *s, const void *buf, size_t len) {
    if (!s || s->fd < 0 || !buf || len == 0) return ERR_INVALID_ARG;
    return nw_send_all(s->fd, buf, len);
}

ErrorCode tcp_recv(const TcpSocket *s, void *buf, size_t cap, size_t *out_len) {
    if (!s || s->fd < 0 || !buf || cap == 0 || !out_len) return ERR_INVALID_ARG;
    *out_len = 0;
    return nw_recv(s->fd, buf, cap, out_len);
}

void tcp_close(TcpSocket *s) {
    if (s && s->fd >= 0) {
        close(s->fd);
        s->fd = -1;
    }
}
