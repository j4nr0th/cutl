#include "permutations.h"

#include "../common_defs.h"

struct permutation_iterator_t
{
    const uint8_t n;
    const uint8_t r;
    uint8_t _data[];
};

size_t permutation_iterator_required_memory(const uint8_t n, const uint8_t r)
{
    return (size_t)(n + 2 * r) * sizeof(*((permutation_iterator_t *)0xB00000B5)->_data) +
           sizeof(permutation_iterator_t);
}

static uint8_t *perm_indices_ptr(permutation_iterator_t *const this)
{
    return this->_data;
}

static uint8_t *perm_cycles_ptr(permutation_iterator_t *const this)
{
    return this->_data + this->n;
}

static uint8_t *perm_value_ptr(permutation_iterator_t *const this)
{
    return this->_data + this->n + this->r;
}

// static const uint8_t *perm_indices_ptr_const(const permutation_iterator_t *const this)
// {
//     return this->_data;
// }

static const uint8_t *perm_cycles_ptr_const(const permutation_iterator_t *const this)
{
    return this->_data + this->n;
}

static const uint8_t *perm_value_ptr_const(const permutation_iterator_t *const this)
{
    return this->_data + this->n + this->r;
}

void permutation_iterator_init(permutation_iterator_t *const this, const uint8_t n, const uint8_t r)
{
    CUTL_ASSERT(n >= r, "Number of elements must be greater than or equal to the number of elements per permutation.");
    *(uint8_t *)&this->n = n;
    *(uint8_t *)&this->r = r;
    permutation_iterator_reset(this);
}

void permutation_iterator_reset(permutation_iterator_t *this)
{
    uint8_t *const indices = perm_indices_ptr(this);
    for (unsigned i = 0; i < this->n; ++i)
    {
        indices[i] = i;
    }
    uint8_t *const cycles = perm_cycles_ptr(this);
    for (unsigned i = 0; i < this->r; ++i)
    {
        cycles[i] = this->n - i;
    }
    uint8_t *const value = perm_value_ptr(this);
    for (unsigned i = 0; i < this->r; ++i)
    {
        value[i] = i;
    }
}

const uint8_t *permutation_iterator_current(const permutation_iterator_t *this)
{
    return perm_value_ptr_const(this);
}

int permutation_iterator_is_done(const permutation_iterator_t *this)
{
    const uint8_t *const cycles = perm_cycles_ptr_const(this);
    return cycles[0] == 0;
}

void permutation_iterator_next(permutation_iterator_t *const this)
{
    uint8_t *const cycles = perm_cycles_ptr(this);
    uint8_t *const indices = perm_indices_ptr(this);
    uint8_t *const value = perm_value_ptr(this);
    const uint8_t n = this->n;
    const uint8_t r = this->r;

    for (unsigned kv = r; kv > 0; --kv)
    {
        const unsigned k = kv - 1;
        cycles[k] -= 1;
        const unsigned ck = cycles[k];
        if (ck == 0)
        {
            // Rotate all indices from k to n
            const unsigned tmp = indices[k];
            for (unsigned i = k; i < n; ++i)
            {
                indices[i] = indices[i + 1];
            }
            indices[n - 1] = tmp;
            cycles[k] = n - k;
        }
        else
        {
            // Swap indices k with that at n - ck
            const unsigned k1 = k;
            const unsigned k2 = n - ck;
            const unsigned tmp = indices[k1];
            indices[k1] = indices[k2];
            indices[k2] = tmp;
            // Update output
            for (unsigned i = k; i < r; ++i)
                value[i] = indices[i];
            // State was updated, we can return.
            return;
        }
    }

    // We're done, make the iterator to the finished state
    cycles[0] = 0;
}

int permutation_iterator_current_sign(const permutation_iterator_t *this)
{
    const uint8_t r = this->r;
    const uint8_t *const value = perm_value_ptr_const(this);
    // Sign is flipped whenever the value below is higher.
    int sign = 0;
    for (unsigned i = 1; i < r; ++i)
    {
        const uint8_t v = value[i];
        for (unsigned j = 0; j < i; ++j)
        {
            sign += v < value[j];
        }
    }

    // An even number of flips means no flip.
    return sign & 1;
}

unsigned permutation_iterator_total_count(const permutation_iterator_t *this)
{
    const unsigned n = this->n;
    const unsigned r = this->r;

    unsigned count = 1;
    for (unsigned i = n; i > (n - r); --i)
        count *= i;

    return count;
}
