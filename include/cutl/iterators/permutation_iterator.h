#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * A set of permutations is specified by the total number of elements :math:`n`
 * an the size of the selection from these elements :math:`r`. These are denoted as :math:`P^n_r`.
 *
 * What makes permutations different from combinations is that permutations are considered equal
 * if and only if the same elements appear them in the same order, while for combinations the order is not important.
 */
typedef struct permutation_iterator_t permutation_iterator_t;

/**
 * Compute the amount of memory needed to store the iterator.
 *
 * @param n Number of elements to make the selection from.
 * @param r Number of elements selected per permutation.
 * @return Number of bytes needed to store the iterator.
 */
size_t permutation_iterator_required_memory(uint8_t n, uint8_t r);

/**
 * Initializes the permutation iterator in given memory.
 *
 * @param this Memory which will be used for the iterator.
 * @param n Number of elements that the selection can be made from.
 * @param r Number of elements taken per selection.
 */
void permutation_iterator_init(permutation_iterator_t *this, uint8_t n, uint8_t r);

/**
 * Set the iterator to the beginning.
 *
 * @param this Iterator to reset.
 */
void permutation_iterator_reset(permutation_iterator_t *this);

/**
 * Get the current permutation the iterator is at.
 *
 * @param this Iterator to get the current iteration from.
 * @return Array of ``r`` entries with indices of elements in the current permutation.
 */
const uint8_t *permutation_iterator_current(const permutation_iterator_t *this);

/**
 * Check if the iterator went beyond the last permutation.
 *
 * @param this Iterator to check.
 * @return Non-zero if the iterator is finished and zero if it is not.
 */
int permutation_iterator_is_done(const permutation_iterator_t *this);

/**
 * Advance the iterator to the next permutation.
 *
 * @param this Iterator to advance to the next permutation.
 */
void permutation_iterator_next(permutation_iterator_t *this);

/**
 * Determine the sign of a permutation.
 *
 * For some operators the order of elements is important. For example, the wedge product :math:`\wedge`
 * is defined such that:
 *
 * .. math::
 *     a \wedge b = - b \wedge a
 *
 * As such, when permutation iterator is used to track these operators, the sign is used to find
 * what sign the sorted sequence of these operators would have.
 *
 * @param this Iterator from which the current permutation is taken.
 * @return Non-zero if the sign should be flipped and zero if it does not need to be flipped.
 */
int permutation_iterator_current_sign(const permutation_iterator_t *this);

/**
 * Compute the total number of permutations for the values of ``n`` and ``r`` of this iterator.
 *
 * The total number of iterations is given by :math:`\frac{n!}{r!}`
 *
 * @param this Iterator to get the total permutation count for.
 * @return Total number of permutations for the iterator.
 */
unsigned permutation_iterator_total_count(const permutation_iterator_t *this);

/**
 * Compute the total number of permutations for the values of ``n`` and ``r`` of this iterator.
 *
 * The total number of iterations is given by :math:`\frac{n!}{r!}`
 *
 * @param n Number of elements that the selection can be made from.
 * @param r Number of elements taken per selection.
 * @return Total number of permutations for the iterator.
 */
unsigned permutation_total_count(uint8_t n, uint8_t r);