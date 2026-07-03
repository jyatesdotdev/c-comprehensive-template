/**
 * @file rng.h
 * @brief Seedable pseudo-random number generator (PCG32).
 *
 * Deterministic given the same seed — reproducible simulations, ML weight
 * init, and tests. Not cryptographically secure; for security-sensitive
 * randomness use the OS CSPRNG (getrandom/arc4random).
 */
#ifndef MATH_RNG_H
#define MATH_RNG_H

#include <stdint.h>

/** @brief PCG32 generator state. Initialize with rng_seed before use. */
typedef struct Rng {
    uint64_t state; /**< Internal LCG state. */
    uint64_t inc;   /**< Stream selector (odd). */
} Rng;

/**
 * @brief Seed the generator.
 * @param r    Generator to initialize.
 * @param seed Any value; same seed + seq → same sequence.
 * @param seq  Stream selector; different seq values give independent streams.
 */
void rng_seed(Rng *r, uint64_t seed, uint64_t seq);

/** @brief Next uniformly distributed 32-bit value. */
uint32_t rng_next_u32(Rng *r);

/**
 * @brief Uniform integer in [0, bound) without modulo bias.
 * @param r     Generator.
 * @param bound Exclusive upper bound (must be > 0; returns 0 if bound is 0).
 */
uint32_t rng_range_u32(Rng *r, uint32_t bound);

/** @brief Uniform float in [0, 1). */
float rng_next_float(Rng *r);

/** @brief Uniform float in [lo, hi). */
float rng_range_float(Rng *r, float lo, float hi);

/** @brief Standard normal deviate (mean 0, stddev 1), via the polar method. */
float rng_normal(Rng *r);

#endif /* MATH_RNG_H */
