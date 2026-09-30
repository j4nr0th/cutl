#pragma once
#include <cutl/common_defs.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// The alignment the library was built with. It is only handed to us as a definition when a project
// asked for something stricter than the default.
#if defined(CUTL_ALLOCATOR_ALIGNMENT)
#define TEST_ALLOCATOR_ALIGNMENT CUTL_ALLOCATOR_ALIGNMENT
#else
#define TEST_ALLOCATOR_ALIGNMENT alignof(max_align_t)
#endif

/**
 * Allocate memory aligned the way the library hands out its own allocations. The allocators which
 * take the memory they manage from the caller insist on exactly this, and refuse anything looser.
 *
 * @param size Amount of memory to allocate, in bytes. Rounded up to the allocation alignment.
 * @return Aligned memory, to be released with `free`.
 */
static void *test_aligned_alloc(const size_t size)
{
    return aligned_alloc(TEST_ALLOCATOR_ALIGNMENT,
                         (size + TEST_ALLOCATOR_ALIGNMENT - 1) & ~(size_t)(TEST_ALLOCATOR_ALIGNMENT - 1));
}

#ifdef GCC_DETECTED
__attribute__((format(printf, 5, 6))) __attribute__((noreturn))
#endif
static void failed_assertion(const char *file, const int line, const char *function, const char *expr, const char *msg,
                             ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, msg);
    vsnprintf(buffer, sizeof(buffer), msg, args);
    va_end(args);
    fprintf(stderr, "%s:%d - %s (Assertion failed: %s): %s\n", file, line, function, expr, buffer);
#ifdef __GNUC__
    CUTL_DEBUG_BREAK;
#endif

    exit(EXIT_FAILURE);
}

#define TEST_ASSERTION(expr, msg, ...)                                                                                 \
    ((expr) ? (void)0 : failed_assertion(__FILE__, __LINE__, __func__, #expr, msg __VA_OPT__(, ) __VA_ARGS__))

#define TEST_CUTL_RESULT(res, expr, v)                                                                                 \
    TEST_ASSERTION((res = (expr)) == (v), "\"%s\" failed with an error: : (%s) - %s.", #expr,                          \
                   cutl_result_to_string(res), cutl_result_message(res))

static void test_numbers_close(const char *file, const int line, const char *function, const double x, const double y,
                               const double atol, const double rtol)
{
    TEST_ASSERTION(atol >= 0, "Absolute tolerance must be non-negative.");
    TEST_ASSERTION(rtol >= 0, "Relative tolerance must be non-negative.");
    const double mag_x = fabs(x);
    const double mag_y = fabs(y);
    const double max_mag = mag_x > mag_y ? mag_x : mag_y;
    const double relative_tol = rtol * max_mag;
    const double tol = relative_tol > atol ? relative_tol : atol;
    if (fabs(x - y) > tol)
        failed_assertion(file, line, function, "fabs(x - y) <= tol", "Numbers %g and %g are not close enough.", x, y);
}

#define TEST_NUMBERS_CLOSE(x, y, atol, rtol) test_numbers_close(__FILE__, __LINE__, __func__, x, y, atol, rtol)

#include <stdint.h>

typedef struct
{
    uint32_t state;
} test_prng_t;

void test_prng_seed(test_prng_t *rng, uint32_t seed);

uint32_t test_prng_next_uint(test_prng_t *rng);

double test_prng_next_double(test_prng_t *rng);
