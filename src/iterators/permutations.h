#pragma once

#include <stddef.h>

typedef struct
{
    const unsigned char n;
    const unsigned char r;
    unsigned char _data[];
} permutation_iterator_t;

size_t permutation_iterator_required_memory(unsigned char n, unsigned char r);

void permutation_iterator_init(permutation_iterator_t *this, unsigned char n, unsigned char r);

void permutation_iterator_reset(permutation_iterator_t *this);

const unsigned char *permutation_iterator_current(const permutation_iterator_t *this);

int permutation_iterator_is_done(const permutation_iterator_t *this);

void permutation_iterator_next(permutation_iterator_t *this);

int permutation_iterator_run_callback(permutation_iterator_t *this, void *ptr,
                                      int (*callback)(const unsigned char *permutation, void *ptr));

int permutation_iterator_current_sign(const permutation_iterator_t *this);

unsigned permutation_iterator_total_count(const permutation_iterator_t *this);
