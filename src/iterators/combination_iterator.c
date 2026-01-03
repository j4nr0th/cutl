#include "../../include/cutl/iterators/combination_iterator.h"

#include "../../include/cutl/common_defs.h"

struct combination_iterator_t
{
    uint8_t n;
    uint8_t r;
    uint8_t counters[];
};

size_t combination_iterator_required_memory(const uint8_t r)
{
    return sizeof(combination_iterator_t) + r * sizeof(uint8_t);
}

void combination_iterator_init(combination_iterator_t *this, const uint8_t n, const uint8_t r)
{
    CUTL_ASSERT(n >= r, "Number of elements must be greater than or equal to the number of elements per permutation.");
    this->n = n;
    this->r = r;
    combination_iterator_reset(this);
}

void combination_iterator_reset(combination_iterator_t *this)
{
    for (unsigned i = 0; i < this->r; ++i)
    {
        this->counters[i] = i;
    }
}

const uint8_t *combination_iterator_current(const combination_iterator_t *this)
{
    return this->counters;
}

int combination_iterator_is_done(const combination_iterator_t *this)
{
    return this->counters[this->r - 1] == this->n;
}

void combination_iterator_next(combination_iterator_t *this)
{
    if (combination_iterator_is_done(this))
        return;

    for (unsigned i = this->r; i > 0; --i)
    {
        auto const idx = i - 1;
        // Advance the counter
        auto const cnt = this->counters[idx] += 1;

        if (cnt + (this->r - i) != this->n)
        {
            // Roll over indices
            for (unsigned j = idx + 1; j < this->r; ++j)
            {
                this->counters[j] = this->counters[j - 1] + 1;
            }
            return;
        }
    }

    // We went through all combinations already
}

static uintmax_t calculate_combination_count(uint8_t const n, uint8_t const r)
{
    // Handle special cases first
    if (n == r || r == 0)
        return 1;
    if (r == 1)
        return n;

    uintmax_t count = 1;
    for (unsigned i = n; i + r > n; --i)
        count *= i;

    for (unsigned i = r; i > 1; --i)
        count /= i;
    return count;
}

unsigned combination_iterator_total_count(const combination_iterator_t *this)
{
    return calculate_combination_count(this->n, this->r);
}

size_t combination_get_index(const unsigned n, const unsigned r, const uint8_t CUTL_ARRAY_ARG(vals, const static r))
{
    // Check that N and R are sensible
    CUTL_ASSERT(n >= r, "Number of elements must be greater than or equal to the number of elements per permutation.");
    // Check indices are not out of bounds
    for (unsigned i = 0; i < r; ++i)
    {
        CUTL_ASSERT(vals[i] < n, "Values must be less than N.");
    }
    // Check values are sorted
    for (unsigned i = 1; i < r; ++i)
    {
        CUTL_ASSERT(vals[i] > vals[i - 1], "Values must be in ascending order.");
    }

    // Recursive combination counter rewritten into loop
    size_t index = 0;
    for (unsigned i = 0, // Position in the combination array
         min_v = 0;      // Minimal value the current counter could take
         i < r; ++i)
    {
        // Get the current counter
        auto const ci = vals[i];
        // Start the counter for the current contribution
        size_t p = 0;
        for (unsigned j = min_v; j < ci; ++j)
        {
            p += calculate_combination_count(n - 1 - j, r - 1 - i);
        }
        // Add the contribution of the current index to the global one
        index += p;
        // The next counter cannot be less than this value
        min_v = ci + 1;
    }

    return index;
}
