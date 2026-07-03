/**
 * @file socket.h
 * @brief Minimal TCP socket abstraction (POSIX).
 *
 * Blocking TCP client/server primitives following the project's ErrorCode
 * conventions. POSIX only (Linux/macOS); a Windows port would wrap Winsock.
 * For a concurrent server, pair with the thread pool in hpc/thread_pool.h
 * (one tcp_accept loop submitting each connection as a task).
 */
#ifndef NETWORKING_SOCKET_H
#define NETWORKING_SOCKET_H

#include "core/error.h"
#include <stddef.h>
#include <stdint.h>

/** @brief A TCP socket handle. Obtain from tcp_connect/tcp_listen/tcp_accept. */
typedef struct TcpSocket {
    int fd; /**< OS file descriptor, -1 when closed/invalid. */
} TcpSocket;

/**
 * @brief Connect to a TCP server (resolves hostnames via getaddrinfo).
 * @param s    Socket to initialize with the connection.
 * @param host Hostname or IP address (e.g. "127.0.0.1").
 * @param port Port to connect to (must be > 0).
 * @return ERR_OK on success, ERR_INVALID_ARG for bad inputs,
 *         ERR_NOT_FOUND if resolution fails, ERR_IO if connecting fails.
 */
ErrorCode tcp_connect(TcpSocket *s, const char *host, uint16_t port);

/**
 * @brief Open a listening TCP socket on all interfaces.
 * @param s       Socket to initialize as a listener.
 * @param port    Port to bind, or 0 for an ephemeral port (see tcp_local_port).
 * @param backlog Maximum pending-connection queue length (must be > 0).
 * @return ERR_OK on success, ERR_INVALID_ARG for bad inputs, ERR_IO otherwise.
 */
ErrorCode tcp_listen(TcpSocket *s, uint16_t port, int backlog);

/**
 * @brief Accept one pending connection on a listening socket (blocking).
 * @param listener   Listening socket from tcp_listen.
 * @param out_client Receives the connected client socket.
 * @return ERR_OK on success, ERR_INVALID_ARG for bad inputs, ERR_IO otherwise.
 */
ErrorCode tcp_accept(const TcpSocket *listener, TcpSocket *out_client);

/**
 * @brief Get the locally bound port (useful after tcp_listen with port 0).
 * @param s        Bound socket.
 * @param out_port Receives the port number in host byte order.
 * @return ERR_OK on success, ERR_INVALID_ARG for bad inputs, ERR_IO otherwise.
 */
ErrorCode tcp_local_port(const TcpSocket *s, uint16_t *out_port);

/**
 * @brief Send an entire buffer, retrying on partial sends and EINTR.
 * @param s   Connected socket.
 * @param buf Data to send.
 * @param len Number of bytes to send (must be > 0).
 * @return ERR_OK when all bytes were sent, ERR_INVALID_ARG for bad inputs,
 *         ERR_IO if the connection fails mid-send.
 */
ErrorCode tcp_send_all(const TcpSocket *s, const void *buf, size_t len);

/**
 * @brief Receive up to cap bytes (blocking, single recv call).
 * @param s       Connected socket.
 * @param buf     Buffer to receive into.
 * @param cap     Buffer capacity in bytes (must be > 0).
 * @param out_len Receives the byte count; 0 means the peer closed cleanly.
 * @return ERR_OK on success (including clean EOF with *out_len == 0),
 *         ERR_INVALID_ARG for bad inputs, ERR_IO on connection errors.
 */
ErrorCode tcp_recv(const TcpSocket *s, void *buf, size_t cap, size_t *out_len);

/**
 * @brief Close a socket and invalidate its descriptor. Safe on NULL/closed.
 * @param s Socket to close.
 */
void tcp_close(TcpSocket *s);

#endif /* NETWORKING_SOCKET_H */
