#include "../../src/string/format8.h"
#include "../common/common.h"

#include <stdlib.h>

static void test_integer(const intmax_t value, const string8_t expected_str, integer_spec_c8_t specs)
{
    const string8_t test_1 = {.length = expected_str.length, .data = (char8_t *)malloc(expected_str.length)};
    TEST_ASSERTION(test_1.data, "Failed to allocate memory for test_1");
    memset(test_1.data, 'c', expected_str.length);

    auto const len_1 = format8_integer_length(value, specs);
    TEST_ASSERTION(len_1 == expected_str.length, "Mismatched length (expected %zu, but got %zu)", expected_str.length,
                   len_1);

    auto const res = format8_integer(value, test_1, specs);
    TEST_ASSERTION(res == CUTL_SUCCESS, "Failed to format integer: (%s) - %s.", cutl_result_to_string(res),
                   cutl_result_message(res));

    TEST_ASSERTION(string8_compare(test_1, expected_str) == 0, "Mismatched formatted string (\"%*s\" and \"%*s\" .",
                   (int)test_1.length, (char *)test_1.data, (int)expected_str.length, (char *)expected_str.data);

    char buffer[128] = {};
    snprintf(buffer, sizeof(buffer), "Correctly formatted %jd as %*s\n", value, (int)test_1.length,
             (char *)test_1.data);
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
    digit_spec_c8_t hex_digits = DIGIT_SPEC_HEXADECIMAL;
    const separator_spec_c8_t no_separator = {.separator_distance = 0};
    const separator_spec_c8_t hex_separator = {.separator_distance = 2, .minor_separator = string8_from_literal(u8":")};

    // Default formatting
    test_integer(152890, string8_from_literal(u8"152890"), (integer_spec_c8_t){});
    test_integer(0, string8_from_literal(u8"0"), (integer_spec_c8_t){.minimum_digits = 1});
    // Japanese numbers (fixed-width) with no separator
    test_integer(1982570, string8_from_literal(u8"１９８２５７０"),
                 (integer_spec_c8_t){.digit_spec = &jpn_digits, .separator_spec = &no_separator});

    // Hexadecimal integer (signed)
    test_integer(-0x123456789abcdef, string8_from_literal(u8"-1:23:45:67:89:ab:cd:ef"),
                 (integer_spec_c8_t){.digit_spec = &DIGIT_SPEC_HEXADECIMAL_LOWER, .separator_spec = &hex_separator});
    test_integer(+0x123456789abcdef, string8_from_literal(u8"+1:23:45:67:89:ab:cd:ef"),
                 (integer_spec_c8_t){.digit_spec = &DIGIT_SPEC_HEXADECIMAL_LOWER,
                                     .separator_spec = &hex_separator,
                                     .always_sign = true});
    // Preceding zeros
    test_integer(+0x123456789abcdef, string8_from_literal(u8"+00:01:23:45:67:89:AB:CD:EF"),
                 (integer_spec_c8_t){
                     .digit_spec = &hex_digits,
                     .separator_spec = &hex_separator,
                     .always_sign = true,
                     .minimum_digits = 18,
                 });
    // MOAR Preceding zeros
    test_integer(-0x123456789abcdef, string8_from_literal(u8"-0:00:00:01:23:45:67:89:AB:CD:EF"),
                 (integer_spec_c8_t){
                     .digit_spec = &hex_digits,
                     .separator_spec = &hex_separator,
                     .always_sign = true,
                     .minimum_digits = 21,
                 });

    // Pad far left
    const padding_spec_c8_t pad_left = {
        .padding_max = 35, .padding = string8_from_literal("*"), .direction = PADDING_LEFT};
    test_integer(-0x123456789abcdef, string8_from_literal(u8"***-0:00:00:01:23:45:67:89:AB:CD:EF"),
                 (integer_spec_c8_t){
                     .digit_spec = &hex_digits,
                     .separator_spec = &hex_separator,
                     .always_sign = true,
                     .minimum_digits = 21,
                     .padding_spec = &pad_left,
                 });

    // Pad far right
    const padding_spec_c8_t pad_right = {
        .padding_max = 36, .padding = string8_from_literal("*"), .direction = PADDING_RIGHT};
    test_integer(-0x123456789abcdef, string8_from_literal(u8"-0:00:00:01:23:45:67:89:AB:CD:EF****"),
                 (integer_spec_c8_t){
                     .digit_spec = &hex_digits,
                     .separator_spec = &hex_separator,
                     .always_sign = true,
                     .minimum_digits = 21,
                     .padding_spec = &pad_right,
                 });

    // Pad far right bias
    const padding_spec_c8_t pad_right_c = {
        .padding_max = 37, .padding = string8_from_literal("*"), .direction = PADDING_BOTH_R};
    test_integer(-0x123456789abcdef, string8_from_literal(u8"**-0:00:00:01:23:45:67:89:AB:CD:EF***"),
                 (integer_spec_c8_t){
                     .digit_spec = &hex_digits,
                     .separator_spec = &hex_separator,
                     .always_sign = true,
                     .minimum_digits = 21,
                     .padding_spec = &pad_right_c,
                 });
    // Pad far left bias
    const padding_spec_c8_t pad_left_c = {
        .padding_max = 37, .padding = string8_from_literal("*"), .direction = PADDING_BOTH_L};
    test_integer(-0x123456789abcdef, string8_from_literal(u8"***-0:00:00:01:23:45:67:89:AB:CD:EF**"),
                 (integer_spec_c8_t){
                     .digit_spec = &hex_digits,
                     .separator_spec = &hex_separator,
                     .always_sign = true,
                     .minimum_digits = 21,
                     .padding_spec = &pad_left_c,
                 });

    // Very positive :)
    const sign_spec_c8_t very_positive_sign = {
        .plus = string8_from_literal(u8"++++++++"),
        .minus = string8_from_literal(u8"-"),
    };
    test_integer(0x123456789abcdef, string8_from_literal(u8"***++++++++0:00:00:01:23:45:67:89:AB:CD:EF**"),
                 (integer_spec_c8_t){
                     .digit_spec = &hex_digits,
                     .separator_spec = &hex_separator,
                     .sign_spec = &very_positive_sign,
                     .padding_spec = &pad_left_c,
                     .always_sign = true,
                     .minimum_digits = 21,

                 });

    return 0;
}
