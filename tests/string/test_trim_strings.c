#include "../common/common.h"
#include <cutl/strings.h>

static void test_string_trimming(const string8_t original, const string8_t expected)
{
    auto const trimmed = string8_trim_whitespace_before(string8_trim_whitespace_after(original));
    TEST_ASSERTION(string8_compare(trimmed, expected) == 0,
                   "String \"%*s\" was not properly trimmed (got \"%*s\" instead of \"%*s\")", (int)original.length,
                   (char *)original.data, (int)trimmed.length, (char *)trimmed.data, (int)expected.length,
                   (char *)expected.data);
}

int main(void)
{
    // very basic
    auto const string_to_trim1 = string8_from_literal("  hello world  ");
    auto const string_trimmed1 = string8_from_literal("hello world");
    test_string_trimming(string_to_trim1, string_trimmed1);

    // when no whitespace, we do nothing
    auto const string_to_trim2 = string8_from_literal("hello world");
    auto const string_trimmed2 = string8_from_literal("hello world");
    test_string_trimming(string_to_trim2, string_trimmed2);

    // When we only have space, we end with an empty string
    auto const string_to_trim3 = string8_from_literal("  ");
    auto const string_trimmed3 = string8_from_literal("");
    test_string_trimming(string_to_trim3, string_trimmed3);

    // Some Japanese characters, since their space is actually U+3000 (also throw in some other random whitespace)
    auto const string_to_trim4 = string8_from_literal("　　こんにちは　　\n　\t");
    auto const string_trimmed4 = string8_from_literal("こんにちは");
    test_string_trimming(string_to_trim4, string_trimmed4);

    return 0;
}