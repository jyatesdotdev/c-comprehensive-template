/**
 * @file unix_socket.c
 * @brief Unix domain stream sockets (POSIX implementation).
 */
#include "networking/unix_socket.h"
#include "socket_internal.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

/** Build a sockaddr_un for path; ERR_INVALID_ARG if it doesn't fit. */
static ErrorCode make_addr(struct sockaddr_un *addr, const char *path) {
    memset(addr, 0, sizeof(*addr));
    addr->sun_family = AF_UNIX;
    if (strlen(path) >= sizeof(addr->sun_path)) return ERR_INVALID_ARG;
    snprintf(addr->sun_path, sizeof(addr->sun_path), "%s", path);
    return ERR_OK;
}

ErrorCode unix_listen(UnixSocket *s, const char *path, int backlog) {
    if (!s || !path || !path[0] || backlog <= 0) return ERR_INVALID_ARG;
    s->fd = -1;

    struct sockaddr_un addr;
    ErrorCode          err = make_addr(&addr, path);
    if (err) return err;

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return ERR_IO;
    nw_disable_sigpipe(fd);

    (void)unlink(path); /* remove stale socket file from a previous run */
    if (bind(fd, (const struct sockaddr *)&addr, sizeof(addr)) != 0 || listen(fd, backlog) != 0) {
        close(fd);
        return ERR_IO;
    }
    s->fd = fd;
    return ERR_OK;
}

ErrorCode unix_connect(UnixSocket *s, const char *path) {
    if (!s || !path || !path[0]) return ERR_INVALID_ARG;
    s->fd = -1;

    struct sockaddr_un addr;
    ErrorCode          err = make_addr(&addr, path);
    if (err) return err;

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return ERR_IO;
    nw_disable_sigpipe(fd);

    if (connect(fd, (const struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close(fd);
        return ERR_IO;
    }
    s->fd = fd;
    return ERR_OK;
}

ErrorCode unix_accept(const UnixSocket *listener, UnixSocket *out_client) {
    if (!listener || listener->fd < 0 || !out_client) return ERR_INVALID_ARG;
    out_client->fd = nw_accept_intr(listener->fd);
    return out_client->fd < 0 ? ERR_IO : ERR_OK;
}

ErrorCode unix_send_all(const UnixSocket *s, const void *buf, size_t len) {
    if (!s || s->fd < 0 || !buf || len == 0) return ERR_INVALID_ARG;
    return nw_send_all(s->fd, buf, len);
}

ErrorCode unix_recv(const UnixSocket *s, void *buf, size_t cap, size_t *out_len) {
    if (!s || s->fd < 0 || !buf || cap == 0 || !out_len) return ERR_INVALID_ARG;
    *out_len = 0;
    return nw_recv(s->fd, buf, cap, out_len);
}

void unix_close(UnixSocket *s) {
    if (s && s->fd >= 0) {
        close(s->fd);
        s->fd = -1;
    }
}
