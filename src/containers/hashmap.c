/**
 * @file hashmap.c
 * @brief Open-addressing hash map implementation (linear probing + tombstones).
 */
#include "containers/hashmap.h"
#include "containers/hash.h"

#include <stdlib.h>
#include <string.h>

#define HASHMAP_INITIAL_CAP 16 /* must be a power of two */

ErrorCode hashmap_init(HashMap *m) {
    if (!m) return ERR_INVALID_ARG;
    m->slots = calloc(HASHMAP_INITIAL_CAP, sizeof(HashMapSlot));
    if (!m->slots) return ERR_NOMEM;
    m->cap = HASHMAP_INITIAL_CAP;
    m->len = 0;
    m->used = 0;
    return ERR_OK;
}

void hashmap_destroy(HashMap *m) {
    if (!m || !m->slots) return;
    for (size_t i = 0; i < m->cap; i++) free(m->slots[i].key);
    free(m->slots);
    m->slots = NULL;
    m->cap = 0;
    m->len = 0;
    m->used = 0;
}

/** Find the slot for key: the live match if present, else the first free slot. */
static HashMapSlot *probe(HashMapSlot *slots, size_t cap, const char *key) {
    size_t       i = (size_t)(hash_fnv1a_str(key) & (cap - 1));
    HashMapSlot *first_tombstone = NULL;

    for (size_t step = 0; step < cap; step++, i = (i + 1) & (cap - 1)) {
        HashMapSlot *s = &slots[i];
        if (!s->key) return first_tombstone ? first_tombstone : s;
        if (s->dead) {
            if (!first_tombstone) first_tombstone = s;
        } else if (strcmp(s->key, key) == 0) {
            return s;
        }
    }
    return first_tombstone; /* table full of tombstones — caller rehashes first */
}

/** Grow (or compact tombstones) into a table of new_cap slots. */
static ErrorCode rehash(HashMap *m, size_t new_cap) {
    HashMapSlot *fresh = calloc(new_cap, sizeof(HashMapSlot));
    if (!fresh) return ERR_NOMEM;

    for (size_t i = 0; i < m->cap; i++) {
        HashMapSlot *s = &m->slots[i];
        if (s->key && !s->dead) {
            HashMapSlot *dst = probe(fresh, new_cap, s->key);
            dst->key = s->key; /* move ownership */
            dst->value = s->value;
        } else {
            free(s->key); /* tombstone keys die here */
        }
    }
    free(m->slots);
    m->slots = fresh;
    m->cap = new_cap;
    m->used = m->len;
    return ERR_OK;
}

ErrorCode hashmap_put(HashMap *m, const char *key, void *value) {
    if (!m || !m->slots || !key) return ERR_INVALID_ARG;

    /* Keep load factor (including tombstones) at or below 3/4. */
    if ((m->used + 1) * 4 > m->cap * 3) {
        ErrorCode err = rehash(m, m->cap * 2);
        if (err) return err;
    }

    HashMapSlot *s = probe(m->slots, m->cap, key);
    if (s->key && !s->dead) { /* update in place */
        s->value = value;
        return ERR_OK;
    }

    char *copy = strdup(key);
    if (!copy) return ERR_NOMEM;
    if (!s->key) m->used++; /* fresh slot (reusing a tombstone doesn't add) */
    free(s->key);           /* tombstone's old key, if any */
    s->key = copy;
    s->value = value;
    s->dead = 0;
    m->len++;
    return ERR_OK;
}

/** Live slot for key, or NULL. */
static HashMapSlot *find_live(const HashMap *m, const char *key) {
    if (!m || !m->slots || !key) return NULL;
    HashMapSlot *s = probe(m->slots, m->cap, key);
    return (s && s->key && !s->dead) ? s : NULL;
}

void *hashmap_get(const HashMap *m, const char *key) {
    HashMapSlot *s = find_live(m, key);
    return s ? s->value : NULL;
}

bool hashmap_contains(const HashMap *m, const char *key) {
    return find_live(m, key) != NULL;
}

ErrorCode hashmap_remove(HashMap *m, const char *key) {
    if (!m || !m->slots || !key) return ERR_INVALID_ARG;
    HashMapSlot *s = find_live(m, key);
    if (!s) return ERR_NOT_FOUND;
    s->dead = 1; /* keep the key so probe chains stay intact until rehash */
    s->value = NULL;
    m->len--;
    return ERR_OK;
}

size_t hashmap_len(const HashMap *m) {
    return m ? m->len : 0;
}

bool hashmap_next(const HashMap *m, HashMapIter *it, const char **out_key, void **out_value) {
    if (!m || !m->slots || !it) return false;
    while (it->idx < m->cap) {
        const HashMapSlot *s = &m->slots[it->idx++];
        if (s->key && !s->dead) {
            if (out_key) *out_key = s->key;
            if (out_value) *out_value = s->value;
            return true;
        }
    }
    return false;
}
