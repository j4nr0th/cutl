#include <cutl/format.h>
#include "../common/common.h"

#include <stdlib.h>

static void test_exponential(const double value, const string8_t expected_str, const exponential_spec_c8_t specs)
{
    const string8_t test_1 = {.length = expected_str.length, .data = (char8_t *)malloc(expected_str.length)};
    TEST_ASSERTION(test_1.data, "Failed to allocate memory for test_1");
    memset(test_1.data, 'c', expected_str.length);

    auto const len_1 = format8_exponential_length(value, specs);
    TEST_ASSERTION(len_1 == expected_str.length, "Mismatched length (expected %zu, but got %zu)", expected_str.length,
                   len_1);

    auto const res = format8_exponential(value, test_1, specs);
    TEST_ASSERTION(res == CUTL_SUCCESS, "Failed to format integer: (%s) - %s.", cutl_result_to_string(res),
                   cutl_result_message(res));

    TEST_ASSERTION(string8_compare(test_1, expected_str) == 0, "Mismatched formatted string (\"%*s\" and \"%*s\" .",
                   (int)test_1.length, (char *)test_1.data, (int)expected_str.length, (char *)expected_str.data);

    char buffer[128] = {};
    snprintf(buffer, sizeof(buffer), "Correctly formatted %g as %*s\n", value, (int)test_1.length, (char *)test_1.data);
    buffer[127] = 0;
    printf("%s", buffer);
    free(test_1.data);
}

int main()
{
    const digit_spec_c8_t jpn_digits = {.base = 10,
                                        .digits = (const string8_t[]){
                                            [0] = string8_from_literal(u8"０"),
                                            [1] = string8_from_literal(u8"１"),
                                            [2] = string8_from_literal(u8"２"),
                                            [3] = string8_from_literal(u8"３"),
                                            [4] = string8_from_literal(u8"４"),
                                            [5] = string8_from_literal(u8"５"),
                                            [6] = string8_from_literal(u8"６"),
                                            [7] = string8_from_literal(u8"７"),
                                            [8] = string8_from_literal(u8"８"),
                                            [9] = string8_from_literal(u8"９"),
                                        }};
    const separator_spec_c8_t test_separator = {.separator_distance = 3,
                                                .major_separator = string8_from_literal(u8"."),
                                                .minor_separator = string8_from_literal(u8"'")};
    const padding_spec_c8_t pad_left_c = {
        .padding_max = 15, .padding = string8_from_literal("?"), .direction = PADDING_BOTH_L};
    const exponent_spec_c8_t latex_exponent_hex = {
        .exponent_prefix = string8_from_literal(u8"16^{"),
        .exponent_suffix = string8_from_literal(u8"}"),
        .use_separators = false,
    };

    // Default formatting
    test_exponential(1.52890e10, string8_from_literal(u8"1.52890E10"), (exponential_spec_c8_t){.fractional_digits = 5});

    // Different digits
    test_exponential(1.52890e10, string8_from_literal(u8"１.５２８９０E１０"),
                     (exponential_spec_c8_t){.fractional_digits = 5, .digit_spec = &jpn_digits});

    // Default, no fractional digits
    test_exponential(1.52890e10, string8_from_literal(u8"2.E10"), (exponential_spec_c8_t){.fractional_digits = 0});
    // Default, 1-1 digits
    test_exponential(0, string8_from_literal(u8"0.0"),
                     (exponential_spec_c8_t){.skip_zero_exponent = true, .fractional_digits = 1});

    // With a separator
    test_exponential(1.781'515e+29, string8_from_literal(u8"1.781'515E+29"),
                     (exponential_spec_c8_t){
                         .fractional_digits = 6, .separator_spec = &test_separator, .always_exponent_sign = true});

    // Pad with separator
    test_exponential(1.781'515e-29, string8_from_literal(u8"?1.781'515E-29?"),
                     (exponential_spec_c8_t){
                         .fractional_digits = 6, .separator_spec = &test_separator, .padding_spec = &pad_left_c});

    return 0;
}
