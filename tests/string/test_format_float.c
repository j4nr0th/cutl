#include "../../src/string/format8.h"
#include "../common/common.h"

#include <stdlib.h>

static void test_float(const double value, const string8_t expected_str, const float_spec_c8_t specs)
{
    const string8_t test_1 = {.length = expected_str.length, .data = (char8_t *)malloc(expected_str.length)};
    TEST_ASSERTION(test_1.data, "Failed to allocate memory for test_1");
    memset(test_1.data, 'c', expected_str.length);

    auto const len_1 = format8_float_length(value, specs);
    TEST_ASSERTION(len_1 == expected_str.length, "Mismatched length (expected %zu, but got %zu)", expected_str.length,
                   len_1);

    auto const res = format8_float(value, test_1, specs);
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
                                                .minor_separator = string8_from_literal(u8"_")};
    const padding_spec_c8_t pad_left_c = {
        .padding_max = 15, .padding = string8_from_literal("?"), .direction = PADDING_BOTH_L};

    // Default formatting
    test_float(152890.00, string8_from_literal(u8"152890.00"), (float_spec_c8_t){.fractional_digits = 2});

    // Different digits
    test_float(152890.00, string8_from_literal(u8"１５２８９０.００"),
               (float_spec_c8_t){.fractional_digits = 2, .digit_spec = &jpn_digits});

    // Default, no fractional digits
    test_float(152890., string8_from_literal(u8"152890."), (float_spec_c8_t){.fractional_digits = 0});
    // Default, no only digits
    test_float(.152890, string8_from_literal(u8".152890"), (float_spec_c8_t){.fractional_digits = 6});
    // Default, 1-1 digits
    test_float(0, string8_from_literal(u8"0.0"), (float_spec_c8_t){.minimum_digits = 1, .fractional_digits = 1});

    // With a separator
    test_float(17815.15290, string8_from_literal(u8"17_815.152_9"),
               (float_spec_c8_t){.fractional_digits = 4, .separator_spec = &test_separator});

    // Pad with separator
    test_float(
        17815.15290, string8_from_literal(u8"??17_815.152_9?"),
        (float_spec_c8_t){.fractional_digits = 4, .separator_spec = &test_separator, .padding_spec = &pad_left_c});

    // Add the sign as well
    test_float(17815.15290, string8_from_literal(u8"?+17_815.152_9?"),
               (float_spec_c8_t){.fractional_digits = 4,
                                 .separator_spec = &test_separator,
                                 .padding_spec = &pad_left_c,
                                 .always_sign = true});

    // Do a hex number
    test_float(0x.A4112p3, string8_from_literal(u8"???+5.208_900??"),
               (float_spec_c8_t){.fractional_digits = 6,
                                 .separator_spec = &test_separator,
                                 .padding_spec = &pad_left_c,
                                 .always_sign = true,
                                 .digit_spec = &DIGIT_SPEC_HEXADECIMAL});

    // Check we round correctly
    test_float(0x.A4112p3, string8_from_literal(u8"?????+5.209????"),
               (float_spec_c8_t){.fractional_digits = 3,
                                 .separator_spec = &test_separator,
                                 .padding_spec = &pad_left_c,
                                 .always_sign = true,
                                 .digit_spec = &DIGIT_SPEC_HEXADECIMAL});

    return 0;
}
