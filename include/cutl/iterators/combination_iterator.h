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
 * The total number of iterations is given by :math:`\frac{n!}{(n - r)! \cdot r!}`
 *
 * @param this Iterator to get the total combination count for.
 * @return Total number of combinations for the iterator.
 */
unsigned combination_iterator_total_count(const combination_iterator_t *this);

/**
 * Compute the total number of combinations for the values of ``n`` and ``r`` of this iterator.
 *
 * The total number of iterations is given by :math:`\frac{n!}{(n - r)! \cdot r!}`
 *
 * @param n Number of elements that the selection can be made from.
 * @param r Number of elements taken per selection.
 * @return Total number of combinations for the iterator.
 */
unsigned combination_total_count(uint8_t n, uint8_t r);

/**
 * Get the index at which the specified iteration would appear.
 *
 * @param n Number of elements that the selection can be made from.
 * @param r Number of elements taken per selection.
 * @param vals Selection of the combination to get the index for.
 * @return Index of the combination.
 */
unsigned combination_get_index(unsigned n, unsigned r, const uint8_t CUTL_ARRAY_ARG(vals, static r));

/**
 * Get the number of combinations that were processed between the first and second one.
 *
 * @param n Number of elements that the selection can be made from.
 * @param r Number of elements taken per selection.
 * @param vals_1 Selection of the first combination.
 * @param vals_2 Selection of the first combination.
 * @return Number of combinations between the first and second one. Positive value means that the first combination
 *         occurs before the second one, while the negative value means the opposite. Zero is returned when the two
 *         are identical.
 */
signed combination_get_index_difference(unsigned n, unsigned r, const uint8_t CUTL_ARRAY_ARG(vals_1, static r),
                                        const uint8_t CUTL_ARRAY_ARG(vals_2, static r));

/**
 * Set the iterator to a combination based on its lexicographical ordering index.
 *
 * Lexicographical ordering means that for combination ``A < B``, it means that for
 * the first non-equal pair of entries of ``a`` and ``b``, it holds that ``a < b``.
 *
 * @param iter Combination iterator to set.
 * @param index Index of the combination to set.
 */
void combination_iterator_set_to_index(combination_iterator_t *iter, unsigned index);

/**
 * Set the array to a combination based on its lexicographical ordering index.
 *
 * Lexicographical ordering means that for combination ``A < B``, it means that for
 * the first non-equal pair of entries of ``a`` and ``b``, it holds that ``a < b``.
 *
 * @param n Number of elements that the selection can be made from.
 * @param r Number of elements taken per selection.
 * @param vals Array which will receive the combination.
 * @param index Index of the combination to set.
 */
void combination_set_to_index(uint8_t n, uint8_t r, uint8_t CUTL_ARRAY_ARG(vals, r), unsigned index);
