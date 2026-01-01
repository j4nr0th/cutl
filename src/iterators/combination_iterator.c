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

unsigned combination_iterator_total_count(const combination_iterator_t *this)
{
    uintmax_t count = 1;
    for (unsigned i = this->n; i + this->r > this->n; --i)
        count *= i;

    for (unsigned i = this->r; i > 1; --i)
        count /= i;

    return count;
}
