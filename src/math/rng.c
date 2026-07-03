/**
 * @file rng.c
 * @brief PCG32 implementation (Melissa O'Neill, www.pcg-random.org, Apache-2.0).
 */
#include "math/rng.h"

#include <math.h>

void rng_seed(Rng *r, uint64_t seed, uint64_t seq) {
    if (!r) return;
    r->state = 0u;
    r->inc = (seq << 1u) | 1u;
    (void)rng_next_u32(r);
    r->state += seed;
    (void)rng_next_u32(r);
}

uint32_t rng_next_u32(Rng *r) {
    uint64_t old = r->state;
    r->state = old * 6364136223846793005ULL + r->inc;
    uint32_t xorshifted = (uint32_t)(((old >> 18u) ^ old) >> 27u);
    uint32_t rot = (uint32_t)(old >> 59u);
    return (xorshifted >> rot) | (xorshifted << ((32u - rot) & 31u));
}

uint32_t rng_range_u32(Rng *r, uint32_t bound) {
    if (bound == 0) return 0;
    /* Rejection sampling: discard the biased low region. */
    uint32_t threshold = (0u - bound) % bound;
    for (;;) {
        uint32_t v = rng_next_u32(r);
        if (v >= threshold) return v % bound;
    }
}

float rng_next_float(Rng *r) {
    /* 24 high bits → float in [0, 1) with full float precision. */
    return (float)(rng_next_u32(r) >> 8) * (1.0f / 16777216.0f);
}

float rng_range_float(Rng *r, float lo, float hi) {
    return lo + (hi - lo) * rng_next_float(r);
}

float rng_normal(Rng *r) {
    /* Marsaglia polar method: sample the unit disk, transform to N(0, 1). */
    for (;;) {
        float u = 2.0f * rng_next_float(r) - 1.0f;
        float v = 2.0f * rng_next_float(r) - 1.0f;
        float s = u * u + v * v;
        if (s > 0.0f && s < 1.0f) return u * sqrtf(-2.0f * logf(s) / s);
    }
}
