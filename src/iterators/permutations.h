#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    const uint8_t n;
    const uint8_t r;
    uint8_t _data[];
} permutation_iterator_t;

size_t permutation_iterator_required_memory(uint8_t n, uint8_t r);

void permutation_iterator_init(permutation_iterator_t *this, uint8_t n, uint8_t r);

void permutation_iterator_reset(permutation_iterator_t *this);

const uint8_t *permutation_iterator_current(const permutation_iterator_t *this);

int permutation_iterator_is_done(const permutation_iterator_t *this);

void permutation_iterator_next(permutation_iterator_t *this);

int permutation_iterator_run_callback(permutation_iterator_t *this, void *ptr,
                                      int (*callback)(const uint8_t *permutation, void *ptr));

int permutation_iterator_current_sign(const permutation_iterator_t *this);

unsigned permutation_iterator_total_count(const permutation_iterator_t *this);
