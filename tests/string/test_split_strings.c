#include "../common/common.h"
#include <cutl/strings.h>

static void test_string_splitting(const string8_t original, const string8_t delim, const size_t n_expected,
                                  const string8_t p_expected[static n_expected])
{
    constexpr size_t max_splits = 32;
    string8_t splits[max_splits];
    TEST_ASSERTION(n_expected <= max_splits, "Too many splits requested, change test code!");

    auto const split_count = string8_split(original, delim, max_splits, splits);
    TEST_ASSERTION(split_count == n_expected, "Wrong number of splits (expected %zu, got %zu)!", n_expected,
                   split_count);

    for (size_t i_split = 0; i_split < n_expected; ++i_split)
    {
        auto const expected = p_expected[i_split];
        auto const actual = splits[i_split];
        TEST_ASSERTION(string8_compare(expected, actual) == 0,
                       "Split %zu was incorrect (expected \"%*s\", got \"%*s\")!", i_split, (int)expected.length,
                       (char *)expected.data, (int)actual.length, (char *)actual.data);
    }
}

int main(void)
{
    // very basic, split using spaces
    auto const string_to_split1 = string8_from_literal("hello world ! Hi!");
    string8_t const string_split1[] = {
        string8_from_literal("hello"),
        string8_from_literal("world"),
        string8_from_literal("!"),
        string8_from_literal("Hi!"),
    };
    test_string_splitting(string_to_split1, string8_from_literal(" "), sizeof(string_split1) / sizeof(string8_t),
                          string_split1);

    // repeated spaces, this means we have some empty sections
    auto const string_to_split2 = string8_from_literal(" hello   world");
    string8_t const string_split2[] = {
        string8_from_literal(""), string8_from_literal("hello"), string8_from_literal(""),
        string8_from_literal(""), string8_from_literal("world"),
    };
    test_string_splitting(string_to_split2, string8_from_literal(" "), sizeof(string_split2) / sizeof(string8_t),
                          string_split2);

    // Only delimiters, this means we have only empty strings
    auto const string_to_split3 = string8_from_literal(";;;;;");
    string8_t const string_split3[] = {
        string8_from_literal(""), string8_from_literal(""), string8_from_literal(""),
        string8_from_literal(""), string8_from_literal(""), string8_from_literal(""),
    };
    test_string_splitting(string_to_split3, string8_from_literal(";"), sizeof(string_split3) / sizeof(string8_t),
                          string_split3);

    // Delimiters no longer only one byte
    auto const string_to_split4 = string8_from_literal("1: :2: :3: : :4");
    string8_t const string_split4[] = {
        string8_from_literal("1"),
        string8_from_literal("2"),
        string8_from_literal("3"),
        string8_from_literal(" :4"),
    };
    test_string_splitting(string_to_split4, string8_from_literal(": :"), sizeof(string_split4) / sizeof(string8_t),
                          string_split4);

    return 0;
}