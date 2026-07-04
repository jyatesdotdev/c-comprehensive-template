/**
 * @file hash.c
 * @brief FNV-1a and CRC-32 implementations.
 */
#include "containers/hash.h"

#include <string.h>

#define FNV_OFFSET 14695981039346656037ULL
#define FNV_PRIME  1099511628211ULL

uint64_t hash_fnv1a(const void *data, size_t len) {
    uint64_t h = FNV_OFFSET;
    if (!data) return h; /* NULL hashes as the empty input */

    const unsigned char *p = data;
    for (size_t i = 0; i < len; i++) {
        h ^= p[i];
        h *= FNV_PRIME;
    }
    return h;
}

uint64_t hash_fnv1a_str(const char *s) {
    return s ? hash_fnv1a(s, strlen(s)) : hash_fnv1a(NULL, 0);
}

uint32_t hash_crc32(const void *data, size_t len) {
    /* Table-less bitwise CRC-32 (reflected, polynomial 0xEDB88320). Slower
       than a table but dependency-free and obviously correct. */
    if (!data) return 0; /* NULL hashes as the empty input */

    const unsigned char *p = data;
    uint32_t             crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc ^= p[i];
        for (int b = 0; b < 8; b++) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}
