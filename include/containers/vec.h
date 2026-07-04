/**
 * @file vec.h
 * @brief Growable array of fixed-size elements (type-erased via element size).
 *
 * Elements are stored by value (memcpy'd in and out). For pointer elements,
 * push the pointer itself: `vec_push(&v, &ptr)`.
 *
 * Growth doubles capacity; pointers returned by vec_at are invalidated by
 * any push/reserve that reallocates.
 */
#ifndef CONTAINERS_VEC_H
#define CONTAINERS_VEC_H

#include "core/error.h"
#include <stddef.h>

/** @brief Growable array. Initialize with vec_init, free with vec_destroy. */
typedef struct Vec {
    unsigned char *data;      /**< Element storage. */
    size_t         len;       /**< Element count. */
    size_t         cap;       /**< Allocated element capacity. */
    size_t         elem_size; /**< Size of one element in bytes. */
} Vec;

/**
 * @brief Initialize an empty vector for elements of elem_size bytes.
 * @param v         Vector to initialize.
 * @param elem_size Element size (> 0), e.g. sizeof(int).
 */
ErrorCode vec_init(Vec *v, size_t elem_size);

/** @brief Free storage. Safe on NULL/already-destroyed. */
void vec_destroy(Vec *v);

/**
 * @brief Append a copy of *elem.
 * @return ERR_OK, ERR_INVALID_ARG, ERR_OVERFLOW on capacity overflow, ERR_NOMEM.
 */
ErrorCode vec_push(Vec *v, const void *elem);

/**
 * @brief Remove the last element, optionally copying it out.
 * @param v   Vector.
 * @param out Receives the removed element (may be NULL to discard).
 * @return ERR_OK, ERR_INVALID_ARG, or ERR_NOT_FOUND if empty.
 */
ErrorCode vec_pop(Vec *v, void *out);

/**
 * @brief Pointer to element i, or NULL if out of bounds.
 *        Invalidated by any operation that grows the vector.
 */
void *vec_at(const Vec *v, size_t i);

/** @brief Ensure capacity for at least cap elements (never shrinks). */
ErrorCode vec_reserve(Vec *v, size_t cap);

/** @brief Reset length to zero without releasing storage. */
void vec_clear(Vec *v);

#endif /* CONTAINERS_VEC_H */
