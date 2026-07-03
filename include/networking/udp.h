/**
 * @file udp.h
 * @brief UDP datagram sockets (POSIX, IPv4).
 *
 * Connectionless datagram messaging following the project's ErrorCode
 * conventions. Datagrams are unreliable and unordered — pair with your own
 * acknowledgement scheme if delivery matters. POSIX only (Linux/macOS).
 */
#ifndef NETWORKING_UDP_H
#define NETWORKING_UDP_H

#include "core/error.h"
#include <stddef.h>
#include <stdint.h>

/** @brief A UDP socket handle. Obtain from udp_open. */
typedef struct UdpSocket {
    int fd; /**< OS file descriptor, -1 when closed/invalid. */
} UdpSocket;

/** @brief Maximum numeric address string length (fits IPv6 + NUL). */
#define UDP_HOST_MAX 46

/** @brief Sender identity reported by udp_recv_from; pass back to udp_send_to to reply. */
typedef struct UdpEndpoint {
    char     host[UDP_HOST_MAX]; /**< Numeric IP address string. */
    uint16_t port;               /**< Port in host byte order. */
} UdpEndpoint;

/**
 * @brief Open a UDP socket bound to all interfaces.
 * @param s    Socket to initialize.
 * @param port Port to bind, or 0 for an ephemeral port (see udp_local_port).
 * @return ERR_OK on success, ERR_INVALID_ARG for bad inputs, ERR_IO otherwise.
 */
ErrorCode udp_open(UdpSocket *s, uint16_t port);

/**
 * @brief Get the locally bound port (useful after udp_open with port 0).
 * @param s        Bound socket.
 * @param out_port Receives the port number in host byte order.
 * @return ERR_OK on success, ERR_INVALID_ARG for bad inputs, ERR_IO otherwise.
 */
ErrorCode udp_local_port(const UdpSocket *s, uint16_t *out_port);

/**
 * @brief Send one datagram to host:port (resolves hostnames via getaddrinfo).
 * @param s    Open socket.
 * @param host Destination hostname or IPv4 address.
 * @param port Destination port (must be > 0).
 * @param buf  Datagram payload.
 * @param len  Payload size in bytes (must be > 0).
 * @return ERR_OK on success, ERR_INVALID_ARG for bad inputs,
 *         ERR_NOT_FOUND if resolution fails, ERR_IO if sending fails.
 */
ErrorCode udp_send_to(const UdpSocket *s, const char *host, uint16_t port, const void *buf,
                      size_t len);

/**
 * @brief Receive one datagram (blocking). Datagrams longer than cap are truncated.
 * @param s        Open socket.
 * @param buf      Buffer to receive into.
 * @param cap      Buffer capacity in bytes (must be > 0).
 * @param out_len  Receives the payload byte count.
 * @param out_from Optional (may be NULL): receives the sender's address and port.
 * @return ERR_OK on success, ERR_INVALID_ARG for bad inputs, ERR_IO otherwise.
 */
ErrorCode udp_recv_from(const UdpSocket *s, void *buf, size_t cap, size_t *out_len,
                        UdpEndpoint *out_from);

/**
 * @brief Close a socket and invalidate its descriptor. Safe on NULL/closed.
 * @param s Socket to close.
 */
void udp_close(UdpSocket *s);

#endif /* NETWORKING_UDP_H */
