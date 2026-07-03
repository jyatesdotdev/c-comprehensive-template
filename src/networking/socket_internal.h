/**
 * @file socket_internal.h
 * @brief Shared helpers for the networking module. Not part of the public API.
 *
 * TCP and Unix domain stream sockets have identical send/recv/accept
 * semantics, so both wrap these fd-level helpers (defined in socket.c).
 */
#ifndef NETWORKING_SOCKET_INTERNAL_H
#define NETWORKING_SOCKET_INTERNAL_H

#include "core/error.h"
#include <stddef.h>

/** Prevent SIGPIPE on writes to a closed peer (BSD/macOS; Linux uses MSG_NOSIGNAL). */
void nw_disable_sigpipe(int fd);

/** accept(2) retrying on EINTR. Returns the new fd, or -1 on error. */
int nw_accept_intr(int listen_fd);

/** Send the whole buffer, retrying on partial sends and EINTR. */
ErrorCode nw_send_all(int fd, const void *buf, size_t len);

/** Single blocking recv retrying on EINTR; *out_len == 0 means clean EOF. */
ErrorCode nw_recv(int fd, void *buf, size_t cap, size_t *out_len);

#endif /* NETWORKING_SOCKET_INTERNAL_H */
