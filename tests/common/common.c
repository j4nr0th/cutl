#include "common.h"

// Initialize the PRNG state with a seed (non-zero recommended)
void test_prng_seed(test_prng_t *rng, uint32_t seed)
{
    if (seed == 0)
        seed = 1; // avoid zero seed which can degenerate sequence
    rng->state = seed;
}

// Generate the next random unsigned 32-bit integer
uint32_t test_prng_next_uint(test_prng_t *rng)
{
    // Constants from Numerical Recipes LCG
    rng->state = 1664525 * rng->state + 1013904223;
    return rng->state;
}

// Generate the next random double in [0, 1)
double test_prng_next_double(test_prng_t *rng)
{
    uint32_t val = test_prng_next_uint(rng);
    // Divide by 2^32 to get uniform double in [0,1)
    return (double)val / 4294967296.0;
}
