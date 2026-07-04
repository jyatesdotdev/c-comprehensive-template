/**
 * @file hash.h
 * @brief Non-cryptographic hash functions: FNV-1a and CRC-32.
 *
 * For hash tables and integrity checks. NOT for security — use a real
 * cryptographic hash (via a vetted library) for anything adversarial.
 */
#ifndef CONTAINERS_HASH_H
#define CONTAINERS_HASH_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief 64-bit FNV-1a hash of a byte buffer. Fast, good distribution.
 * @param data Bytes to hash (may be NULL only if len is 0).
 * @param len  Byte count.
 */
uint64_t hash_fnv1a(const void *data, size_t len);

/** @brief 64-bit FNV-1a hash of a NUL-terminated string (NULL hashes as empty). */
uint64_t hash_fnv1a_str(const char *s);

/**
 * @brief CRC-32 (IEEE 802.3, reflected) of a byte buffer — matches zlib's crc32.
 * @param data Bytes to hash (may be NULL only if len is 0).
 * @param len  Byte count.
 */
uint32_t hash_crc32(const void *data, size_t len);

#endif /* CONTAINERS_HASH_H */
