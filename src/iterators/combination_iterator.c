#include "../../include/cutl/iterators/combination_iterator.h"

#include "../../include/cutl/common_defs.h"

struct combination_iterator_t
{
    uint8_t n;
    uint8_t r;
    uint8_t counters[];
};

size_t combination_iterator_required_memory(uint8_t r)
{
    if (r == 0)
        r = 1; // We need at least one element
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
    // Special case when r is 0
    if (this->r == 0)
    {
        this->counters[0] = 0;
        return;
    }

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
    // Special case when r is 0
    if (this->r == 0)
        return this->counters[0] == 1;
    return this->counters[this->r - 1] == this->n;
}

void combination_iterator_next(combination_iterator_t *this)
{
    if (combination_iterator_is_done(this))
        return;

    if (this->r == 0)
    {
        // Special case when r is 0
        this->counters[0] = 1;
        return;
    }

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

unsigned combination_total_count(const uint8_t n, const uint8_t r)
{
    return calculate_combination_count(n, r);
}

unsigned combination_get_index(const unsigned n, const unsigned r, const uint8_t CUTL_ARRAY_ARG(vals, const static r))
{
    // Check that N and R are sensible
    CUTL_ASSERT(n >= r, "Number of elements must be greater than or equal to the number of elements per permutation.");
    // Special case when r is 0
    if (r == 0)
        return 0;

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
    unsigned index = 0;
    for (unsigned i = 0, // Position in the combination array
         min_v = 0;      // Minimal value the current counter could take
         i < r; ++i)
    {
        // Get the current counter
        auto const ci = vals[i];
        // Start the counter for the current contribution
        unsigned p = 0;
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
signed combination_get_index_difference(const unsigned n, const unsigned r,
                                        const uint8_t CUTL_ARRAY_ARG(vals_1, static r),
                                        const uint8_t CUTL_ARRAY_ARG(vals_2, static r))
{

    // Check that N and R are sensible
    CUTL_ASSERT(n >= r, "Number of elements must be greater than or equal to the number of elements per permutation.");
    // Check indices are not out of bounds
    for (unsigned i = 0; i < r; ++i)
    {
        CUTL_ASSERT(vals_1[i] < n, "Values must be less than N.");
        CUTL_ASSERT(vals_2[i] < n, "Values must be less than N.");
    }
    // Check values are sorted
    for (unsigned i = 1; i < r; ++i)
    {
        CUTL_ASSERT(vals_1[i] > vals_1[i - 1], "Values must be in ascending order.");
        CUTL_ASSERT(vals_2[i] > vals_2[i - 1], "Values must be in ascending order.");
    }

    const uint8_t *first, *second;
    bool negate;
    unsigned matching = 0;
    for (unsigned i = 0; i < r; ++i)
    {
        auto const v1 = vals_1[i];
        auto const v2 = vals_2[i];
        if (v1 < v2)
        {
            first = vals_1;
            second = vals_2;
            negate = false;
            break;
        }
        if (v2 < v1)
        {
            first = vals_2;
            second = vals_1;
            negate = true;
            break;
        }
        matching += 1;
    }
    // They are the same
    if (matching == r)
        return 0;

    // We know which is the first and which is the second.
    // Recursive combination counter rewritten into loop
    signed dif = 0;
    for (unsigned i = matching,    // Position in the combination array
         min_v_1 = 0, min_v_2 = 0; // Minimal value the current counter could take
         i < r; ++i)
    {
        auto const c1 = first[i];
        auto const c2 = second[i];
        signed dp = 0;

        for (unsigned j = min_v_2; j < c2; ++j)
        {
            dp += (signed)calculate_combination_count(n - 1 - j, r - 1 - i);
        }
        for (unsigned j = min_v_1; j < c1; ++j)
        {
            dp -= (signed)calculate_combination_count(n - 1 - j, r - 1 - i);
        }

        dif += dp;
        min_v_1 = c1 + 1;
        min_v_2 = c2 + 1;
    }

    return negate ? -dif : dif;
}

void combination_iterator_set_to_index(combination_iterator_t *const iter, const unsigned index)
{
    combination_set_to_index(iter->n, iter->r, iter->counters, index);
}

void combination_set_to_index(const uint8_t n, const uint8_t r, uint8_t CUTL_ARRAY_ARG(vals, r), const unsigned index)
{
    // Handle the exception
    if (r == 0)
        return;

    unsigned remaining = index;
    unsigned min_val = 0;
    for (unsigned i = 0; i < r - 1; ++i)
    {
        unsigned j;
        for (j = min_val; j < n - r + i; ++j)
        {
            auto const count = calculate_combination_count(n - 1 - j, r - 1 - i);
            if (remaining < count)
            {
                break;
            }
            remaining -= count;
        }
        vals[i] = j;
        min_val = j + 1;
    }

    vals[r - 1] = min_val + remaining;
}
