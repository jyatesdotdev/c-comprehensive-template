/**
 * @file unix_socket.h
 * @brief Unix domain stream sockets (POSIX, local IPC).
 *
 * Stream sockets addressed by filesystem path — same semantics as TCP but
 * local-only and faster; ideal for IPC between processes on one machine.
 * Follows the project's ErrorCode conventions. POSIX only (Linux/macOS).
 */
#ifndef NETWORKING_UNIX_SOCKET_H
#define NETWORKING_UNIX_SOCKET_H

#include "core/error.h"
#include <stddef.h>

/** @brief A Unix domain socket handle. Obtain from the API below. */
typedef struct UnixSocket {
    int fd; /**< OS file descriptor, -1 when closed/invalid. */
} UnixSocket;

/**
 * @brief Listen on a Unix domain socket path. Removes a stale socket file first.
 * @param s       Socket to initialize as a listener.
 * @param path    Filesystem path to bind (max ~100 chars, platform-dependent).
 * @param backlog Maximum pending-connection queue length (must be > 0).
 * @return ERR_OK on success, ERR_INVALID_ARG for bad inputs (including a path
 *         too long for sockaddr_un), ERR_IO otherwise.
 * @note The socket file persists after close — the owner should unlink(path)
 *       when the server shuts down.
 */
ErrorCode unix_listen(UnixSocket *s, const char *path, int backlog);

/**
 * @brief Connect to a Unix domain socket path.
 * @param s    Socket to initialize with the connection.
 * @param path Filesystem path of the listening socket.
 * @return ERR_OK on success, ERR_INVALID_ARG for bad inputs, ERR_IO otherwise.
 */
ErrorCode unix_connect(UnixSocket *s, const char *path);

/**
 * @brief Accept one pending connection on a listening socket (blocking).
 * @param listener   Listening socket from unix_listen.
 * @param out_client Receives the connected client socket.
 * @return ERR_OK on success, ERR_INVALID_ARG for bad inputs, ERR_IO otherwise.
 */
ErrorCode unix_accept(const UnixSocket *listener, UnixSocket *out_client);

/**
 * @brief Send an entire buffer, retrying on partial sends and EINTR.
 * @param s   Connected socket.
 * @param buf Data to send.
 * @param len Number of bytes to send (must be > 0).
 * @return ERR_OK when all bytes were sent, ERR_INVALID_ARG for bad inputs,
 *         ERR_IO if the connection fails mid-send.
 */
ErrorCode unix_send_all(const UnixSocket *s, const void *buf, size_t len);

/**
 * @brief Receive up to cap bytes (blocking, single recv call).
 * @param s       Connected socket.
 * @param buf     Buffer to receive into.
 * @param cap     Buffer capacity in bytes (must be > 0).
 * @param out_len Receives the byte count; 0 means the peer closed cleanly.
 * @return ERR_OK on success (including clean EOF with *out_len == 0),
 *         ERR_INVALID_ARG for bad inputs, ERR_IO on connection errors.
 */
ErrorCode unix_recv(const UnixSocket *s, void *buf, size_t cap, size_t *out_len);

/**
 * @brief Close a socket and invalidate its descriptor. Safe on NULL/closed.
 * @param s Socket to close.
 */
void unix_close(UnixSocket *s);

#endif /* NETWORKING_UNIX_SOCKET_H */
