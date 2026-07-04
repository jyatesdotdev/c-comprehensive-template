/**
 * @file hashmap.h
 * @brief String-keyed hash map (open addressing, linear probing, FNV-1a).
 *
 * Keys are copied and owned by the map; values are opaque pointers owned by
 * the caller (the map never frees them). Load factor 0.75, capacity doubles
 * on growth. Not thread-safe — guard with a mutex for shared use.
 */
#ifndef CONTAINERS_HASHMAP_H
#define CONTAINERS_HASHMAP_H

#include "core/error.h"
#include <stdbool.h>
#include <stddef.h>

/** @brief One slot: key NULL = empty; key set + dead = tombstone. */
typedef struct HashMapSlot {
    char    *key;   /**< Owned copy of the key, or NULL if never used. */
    void    *value; /**< Caller-owned value pointer. */
    unsigned dead;  /**< Nonzero marks a deleted (tombstone) slot. */
} HashMapSlot;

/** @brief Hash map. Initialize with hashmap_init, free with hashmap_destroy. */
typedef struct HashMap {
    HashMapSlot *slots; /**< Slot array (cap entries). */
    size_t       cap;   /**< Slot count (power of two). */
    size_t       len;   /**< Live entry count. */
    size_t       used;  /**< Live entries + tombstones (drives rehashing). */
} HashMap;

/** @brief Initialize an empty map. */
ErrorCode hashmap_init(HashMap *m);

/** @brief Free the map and its key copies (values are untouched). */
void hashmap_destroy(HashMap *m);

/**
 * @brief Insert or update key → value (key is copied).
 * @return ERR_OK, ERR_INVALID_ARG, or ERR_NOMEM.
 */
ErrorCode hashmap_put(HashMap *m, const char *key, void *value);

/** @brief Value for key, or NULL if absent (or if the stored value is NULL). */
void *hashmap_get(const HashMap *m, const char *key);

/** @brief True if key is present (distinguishes NULL values from absence). */
bool hashmap_contains(const HashMap *m, const char *key);

/**
 * @brief Remove key.
 * @return ERR_OK, ERR_INVALID_ARG, or ERR_NOT_FOUND if absent.
 */
ErrorCode hashmap_remove(HashMap *m, const char *key);

/** @brief Live entry count. */
size_t hashmap_len(const HashMap *m);

/** @brief Iterator state. Zero-initialize: `HashMapIter it = {0};` */
typedef struct HashMapIter {
    size_t idx; /**< Next slot to examine. */
} HashMapIter;

/**
 * @brief Advance the iterator; yields entries in unspecified order.
 *        Do not put/remove while iterating.
 * @param m         Map to iterate.
 * @param it        Iterator state (zero-initialized before first call).
 * @param out_key   Receives the key (owned by the map; may be NULL).
 * @param out_value Receives the value (may be NULL).
 * @return true while an entry was produced, false when exhausted.
 */
bool hashmap_next(const HashMap *m, HashMapIter *it, const char **out_key, void **out_value);

#endif /* CONTAINERS_HASHMAP_H */
