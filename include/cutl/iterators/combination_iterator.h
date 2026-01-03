#pragma once

#include "../common_defs.h"
#include <stddef.h>
#include <stdint.h>

/**
 * A set of combinations is specified by the total number of elements :math:`n`
 * and the size of the selection from these elements :math:`r`. These are denoted as :math:`C^n_r`.
 *
 * What makes combinations different from permutations is that combinations are considered equal
 * if the same elements appear in them, while for permutations the order is also important.
 */
typedef struct combination_iterator_t combination_iterator_t;

/**
 * Compute the amount of memory needed to store the iterator.
 *
 * @param r Number of elements selected per combination.
 * @return Number of bytes needed to store the iterator.
 */
size_t combination_iterator_required_memory(uint8_t r);

/**
 * Initializes the combination iterator in given memory.
 *
 * @param this Memory which will be used for the iterator.
 * @param n Number of elements that the selection can be made from.
 * @param r Number of elements taken per selection.
 */
void combination_iterator_init(combination_iterator_t *this, uint8_t n, uint8_t r);

/**
 * Set the iterator to the beginning.
 *
 * @param this Iterator to reset.
 */
void combination_iterator_reset(combination_iterator_t *this);

/**
 * Get the current combination the iterator is at.
 *
 * @param this Iterator to get the current iteration from.
 * @return Array of ``r`` entries with indices of elements in the current combination.
 */
const uint8_t *combination_iterator_current(const combination_iterator_t *this);

/**
 * Check if the iterator went beyond the last combination.
 *
 * @param this Iterator to check.
 * @return Non-zero if the iterator is finished and zero if it is not.
 */
int combination_iterator_is_done(const combination_iterator_t *this);

/**
 * Advance the iterator to the next combination.
 *
 * @param this Iterator to advance to the next combination.
 */
void combination_iterator_next(combination_iterator_t *this);

/**
 * Compute the total number of combinations for the values of ``n`` and ``r`` of this iterator.
 *
 * Total number of iterations is given by :math:`\frac{n!}{(n - r)! \cdot r!}`
 *
 * @param this Allocator to get the total combination count for.
 * @return Total number of combinations for the allocator.
 */
unsigned combination_iterator_total_count(const combination_iterator_t *this);

/**
 * Get the index at which the specified iteration would appear.
 *
 * @param n Number of elements that the selection can be made from.
 * @param r Number of elements taken per selection.
 * @param vals Selection of the combination to get the index for
 * @return Index of the combination.
 */
size_t combination_get_index(unsigned n, unsigned r, const uint8_t CUTL_ARRAY_ARG(vals, static r));
