#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    uint8_t n;
    uint8_t r;
    uint8_t counters[];
} combination_iterator_t;

size_t combination_iterator_required_memory(uint8_t r);

void combination_iterator_init(combination_iterator_t *this, uint8_t n, uint8_t r);

void combination_iterator_reset(combination_iterator_t *this);

const uint8_t *combination_iterator_current(const combination_iterator_t *this);

int combination_iterator_is_done(const combination_iterator_t *this);

void combination_iterator_next(combination_iterator_t *this);

unsigned combination_iterator_total_count(const combination_iterator_t *this);