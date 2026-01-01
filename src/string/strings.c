#include "../../include/cutl/strings.h"

#include "../../include/cutl/common_defs.h"

#include <stdatomic.h>

cutl_result_t string8_create_substring(const string8_t *const str, const size_t start, const size_t end,
                                       string8_t *const substr)
{
    if (str->length < start || end > str->length)
        return CUTL_RESULT_INDEX_OUT_OF_BOUNDS;

    *substr = (string8_t){.length = end - start, .data = str->data + start};
    return CUTL_SUCCESS;
}

cutl_result_t string8_concatenate(const string8_t *str1, const string8_t *str2, string8_t *result)
{
    auto const total_size = str1->length + str2->length;
    if (result->length != total_size)
        return CUTL_RESULT_INSUFFICIENT_BUFFER;

    if (result != str1 && str1->length)
        memcpy(result->data, str1->data, str1->length);

    if (str2->length)
        memcpy(result->data + str1->length, str2->data, str2->length);

    return CUTL_SUCCESS;
}

int32_t string8_compare(const string8_t str1, const string8_t str2)
{
    if (str1.length < str2.length)
    {
        return -1;
    }
    if (str1.length > str2.length)
    {
        return 1;
    }

    return memcmp(str1.data, str2.data, str1.length);
}

int32_t string8_compare_n(const string8_t *str1, const string8_t *str2, const size_t n)
{
    auto const size1 = str1->length < n ? str1->length : n;
    auto const size2 = str2->length < n ? str2->length : n;
    if (size1 < size2)
    {
        return -1;
    }
    if (size1 > size2)
    {
        return 1;
    }

    return memcmp(str1->data, str2->data, size1);
}

int32_t string8_has_substring(const string8_t *string, const string8_t *substr)
{
    if (string->length < substr->length)
        return -1;

    // Find the first position where `substr` appears inside `string`
    for (size_t pos = 0; substr->length + pos < string->length; ++pos)
    {
        size_t match;
        for (match = 0; match < substr->length; ++match)
        {
            if (string->data[pos] != substr->data[match])
                break;
        }
        if (match == substr->length)
            return (int32_t)pos;
    }
    return -1;
}

int32_t string8_has_substring_reverse(const string8_t *string, const string8_t *substr)
{
    if (string->length < substr->length)
        return -1;

    // Go from the end to the front
    for (size_t pos = string->length; pos > substr->length; --pos)
    {
        size_t match;
        for (match = substr->length; match > 0; --match)
        {
            if (string->data[pos - 1] != substr->data[match - 1])
                break;
        }
        if (match == 0)
            return (int32_t)pos;
    }

    return -1;
}

size_t string8_subspan(const string8_t *string, const string8_t *substr)
{
    size_t max_len = 0;
    for (size_t pos = 0; pos + max_len < string->length; ++pos)
    {
        size_t match;
        for (match = 0; match < substr->length; ++match)
        {
            if (string->data[pos + match] != substr->data[match])
                break;
        }

        if (match == substr->length)
            return substr->length;

        if (match > max_len)
            max_len = match;
    }

    return max_len;
}

string8_t string8_advance(const string8_t str, size_t n)
{
    if (n >= str.length)
    {
        return (string8_t){.length = 0, .data = nullptr};
    }

    return (string8_t){.length = str.length - n, .data = str.data + n};
}

string8_t string8_shrink(string8_t str, size_t n)
{
    if (str.length < n)
        return (string8_t){.length = 0, .data = nullptr};

    return (string8_t){.length = str.length - n, .data = str.data};
}

cutl_result_t string8_reorder(const string8_t *str, const unsigned pos1, const unsigned pos2, const size_t len)
{
    if (pos1 + len > str->length || pos2 + len > str->length)
        return CUTL_RESULT_INDEX_OUT_OF_BOUNDS;
    memmove(str->data + pos2, str->data + pos1, len);
    return CUTL_SUCCESS;
}
