#pragma once

#include "../error.h"

#include <stdint.h>
#include <string.h>
#include <uchar.h>

typedef struct
{
    size_t length;
    char8_t *data;
} string8_t;

/**
 * Wrap a typical null-terminated string as a string.
 *
 * @param str Pointer to a null-terminated string to wrap as a string.
 * @return Wrapping struct.
 */
static inline string8_t string8_from_cstr(const char *str)
{
    return (string8_t){.length = strlen(str), .data = (char8_t *)str};
}

/**
 * Wrap first `n` characters from a string.
 *
 * @param str Pointer to the string to wrap.
 * @param size Number of elements to wrap.
 * @return Wrapping struct.
 */
static inline string8_t string8_from_cstr_n(const char *str, const size_t size)
{
    return (string8_t){.length = size, .data = (char8_t *)str};
}

/**
 * Wrap a single character as a string (equivalent to calling `string_utf8_from_cstr_n`).
 *
 * @param c Pointer to the character to wrap.
 * @return Wrapping struct.
 */
static inline string8_t string8_from_char(const char *c)
{
    return (string8_t){.length = 1, .data = (char8_t *)c};
}

#define string8_from_literal(str)                                                                                      \
    (string8_t)                                                                                                        \
    {                                                                                                                  \
        .length = sizeof(str) - 1, .data = (char8_t *)str                                                              \
    }

/**
 * Extract a substring from a string.
 *
 * @param str String to extract the substring from.
 * @param start Position of the start of the substring.
 * @param end Position of the first element to not be in the substring.
 * @param substr Substring which receives the information about the substring.
 * @return CUTL_SUCCESS if successful and CUTL_RESULT_INDEX_OUT_OF_BOUNDS if either `start` or `end` are too large.
 */
cutl_result_t string8_create_substring(const string8_t *str, size_t start, size_t end, string8_t *substr);

/**
 * Concatenates the two strings and stores the concatenated strings in `result`.
 *
 * @param str1 First string to concatenate.
 * @param str2 Second string to concatenate.
 * @param result String which the result is written to. Must be the same size as `str1` and `str2` combined.
 * @return CUTL_SUCCESS if successful, CUTL_RESULT_INSUFFICIENT_BUFFER if `result` is of incorrect size.
 */
cutl_result_t string8_concatenate(const string8_t *str1, const string8_t *str2, string8_t *result);

/**
 * Compare the two strings.
 *
 * @param str1 First string to compare.
 * @param str2 Second string to compare.
 * @return When both are equal 0, otherwise either -1 or +1, respective to the first or the second one being shorter
 * or their first character which differs being less.
 */
int32_t string8_compare(string8_t str1, string8_t str2);

/**
 * Equivalent to `string8_compare`, but considering at most first `n` characters.
 *
 * @param str1 First string to compare.
 * @param str2 Second string to compare.
 * @param n The maximum number of characters to compare.
 * @return Result of comparing at most first `n` characters using `string_utf8_compare`.
 */
int32_t string8_compare_n(const string8_t *str1, const string8_t *str2, size_t n);

/**
 * Search for the first occurrence of a substring within another string.
 *
 * @param string String to search through.
 * @param substr Substring to try and find.
 * @return When `substr` does not appear in `string` -1, otherwise the position where `substr` starts in `string`.
 */
int32_t string8_has_substring(const string8_t *string, const string8_t *substr);

/**
 * Search for the last occurrence of a substring within another string.
 *
 * @param string String to search through.
 * @param substr Substring to try and find.
 * @return When `substr` does not appear in `string` -1, otherwise the after where `substr` ends in `string`.
 */
int32_t string8_has_substring_reverse(const string8_t *string, const string8_t *substr);

/**
 * Find the maximum length of a substring appearing within a string.
 *
 * @param string String to search through.
 * @param substr Substring to try and find.
 * @return The maximum number of elements of `substr` which appear in `string`.
 */
size_t string8_subspan(const string8_t *string, const string8_t *substr);

/**
 * Advance the string by at most `n` bytes.
 *
 * @param str String to advance.
 * @param n The highest number of characters to advance.
 * @return String without at most first `n` bytes.
 */
string8_t string8_advance(string8_t str, size_t n);

/**
 * Remove at most the last `n` bytes.
 *
 * @param str Sting to shrink.
 * @param n Number of bytes to shrink.
 * @return String without the last `n` bytes.
 */
string8_t string8_shrink(string8_t str, size_t n);

/**
 *
 * @param str String to reorder the elements of.
 * @param pos1 Start of the first segment.
 * @param pos2 Start of the second segment.
 * @param len Length of the segments to reorder.
 * @return CUTL_SUCCESS if successful, CUTL_RESULT_INDEX_OUT_OF_BOUNDS if the segments would be out of bounds.
 */
cutl_result_t string8_reorder(const string8_t *str, unsigned pos1, unsigned pos2, size_t len);

/**
 * Get the string representation of a ``cutl_result_t`` value.
 *
 * @param result Value to convert to a string.
 * @return Statically allocated string representation of ``result``.
 */
string8_t cutl_result_to_string8(cutl_result_t result);

/**
 * Get the meaning of the ``cutl_result_t`` value.
 *
 * @param result Value to get the meaning of.
 * @return Statically allocated string representation of the ``result``.
 */
string8_t cutl_result_message_s8(cutl_result_t result);
