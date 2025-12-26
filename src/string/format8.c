#include "format8.h"

#include "../common_defs.h"

const digit_spec_c8_t DIGIT_SPEC_DECIMAL_JPN = {
    .base = 10,
    .digits =
        (const string8_t[10]){
            [0] = string8_from_literal(u8"一"),
            [1] = string8_from_literal(u8"二"),
            [2] = string8_from_literal(u8"三"),
            [3] = string8_from_literal(u8"四"),
            [4] = string8_from_literal(u8"五"),
            [5] = string8_from_literal(u8"六"),
            [6] = string8_from_literal(u8"七"),
            [7] = string8_from_literal(u8"八"),
            [8] = string8_from_literal(u8"九"),
            [9] = string8_from_literal(u8"０"),
        },
};

static unsigned count_digits(uintmax_t n, const unsigned base)
{
    unsigned count = 0;

    // Use a do-while loop to make sure that if we have 0, we have one digit at least.
    do
    {
        count += 1;
    } while (n /= base);

    return count;
}

static unsigned extract_next_digit(uintmax_t *v, const unsigned base)
{
    auto const digit = *v % base;
    *v /= base;
    return digit;
}

static unsigned extract_specific_digit(uintmax_t v, const unsigned base, const unsigned index)
{
    uintmax_t divisor = 1;
    for (unsigned i = 0; i < index; ++i)
    {
        divisor *= base;
    }
    return (v / divisor) % base;
}

static size_t natural_number_length(const uintmax_t n, const unsigned base)
{
    return count_digits(n, base);
}

typedef enum : unsigned char
{
    SIGN_NONE = 0,
    SIGN_POSITIVE = 1,
    SIGN_NEGATIVE = 2,
} number_sign_t;

static struct
{
    size_t length;
    size_t digit_count;
    unsigned padded_zeros;
    number_sign_t sign;
    size_t pads;
} integer_unit_counts(const integer_spec_c8_t *const spec, const intmax_t value)
{
    size_t len = 0, units = 0;
    number_sign_t sign = SIGN_NONE;
    // Sign
    if (value < 0)
    {
        len += spec->sign_spec->minus.length;
        units += 1;
        sign = SIGN_NEGATIVE;
    }
    else if (spec->sign_spec->always_sign)
    {
        len += spec->sign_spec->plus.length;
        units += 1;
        sign = SIGN_POSITIVE;
    }

    auto abs_value = (uintmax_t)(value < 0 ? -value : value);
    // Digits
    auto const digit_count = count_digits(abs_value, spec->digit_spec->base);
    units += digit_count;
    unsigned padded_zeros = 0;
    for (unsigned i = 0; i < digit_count; ++i)
    {
        len += spec->digit_spec->digits[extract_next_digit(&abs_value, spec->digit_spec->base)].length;
    }
    if (digit_count < spec->digit_spec->minimum_count)
    {
        padded_zeros = spec->digit_spec->minimum_count - digit_count;
        len += spec->digit_spec->digits[0].length * padded_zeros;
    }

    // Separators (integers only have minor separators)
    auto const total_digits = digit_count + padded_zeros;
    auto const separator_cnt = spec->separator_spec->separator_distance
                                   ? (total_digits ? (total_digits - 1) / spec->separator_spec->separator_distance : 0)
                                   : 0;
    units += separator_cnt + padded_zeros;
    len += separator_cnt * spec->separator_spec->minor_separator.length;

    // Finally, padding (it is based on units)
    size_t pads = 0;
    if (units < spec->padding_spec->padding_max)
    {
        pads = spec->padding_spec->padding_max - units;
        len += spec->padding_spec->padding.length * pads;
    }

    return (typeof(integer_unit_counts(spec, value))){
        .length = len,
        .digit_count = digit_count,
        .padded_zeros = padded_zeros,
        .sign = sign,
        .pads = pads,
    };
}

static const padding_spec_c8_t PADDING_SPEC_NONE = {.padding_max = 0};

static void integer_spec_fill_defaults(integer_spec_c8_t *spec)
{
    if (!spec->padding_spec)
    {
        // We add default padding specs
        spec->padding_spec = &PADDING_SPEC_NONE;
    }
    if (!spec->separator_spec)
    {
        // We add default separator spec
        spec->separator_spec = &SEPARATOR_SPEC_NONE;
    }
    if (!spec->sign_spec)
    {
        // We add default sign spec
        spec->sign_spec = &SIGN_SPEC_BASIC;
    }
    if (!spec->digit_spec)
    {
        // We add default digit spec
        spec->digit_spec = &DIGIT_SPEC_DECIMAL;
    }
}

size_t format8_integer_length(const intmax_t value, integer_spec_c8_t spec)
{
    integer_spec_fill_defaults(&spec);
    return integer_unit_counts(&spec, value).length;
}

static string8_t write_output_left(const string8_t output, const string8_t str)
{
    memcpy(output.data, str.data, str.length);
    return string8_advance(output, str.length);
}

static string8_t write_output_left_n(string8_t output, const unsigned repeats, string8_t const str)
{
    for (unsigned i = 0; i < repeats; ++i)
    {
        output = write_output_left(output, str);
    }
    return output;
}

static string8_t write_output_right(const string8_t output, const string8_t str)
{
    memcpy(output.data + output.length - str.length, str.data, str.length);
    return string8_shrink(output, str.length);
}

static string8_t write_output_right_n(string8_t output, const unsigned repeats, string8_t const str)
{
    for (unsigned i = 0; i < repeats; ++i)
    {
        output = write_output_right(output, str);
    }
    return output;
}

cutl_result_t format8_integer(const intmax_t value, string8_t output, integer_spec_c8_t spec)
{
    integer_spec_fill_defaults(&spec);
    auto const len_info = integer_unit_counts(&spec, value);
    if (len_info.length != output.length)
    {
        return CUTL_RESULT_INSUFFICIENT_BUFFER;
    }

    // First, write the padding
    if (len_info.pads)
    {
        auto const padding_unit = spec.padding_spec->padding;
        switch (spec.padding_spec->direction)
        {
        case PADDING_LEFT:
            output = write_output_left_n(output, len_info.pads, padding_unit);
            break;

        case PADDING_RIGHT:
            output = write_output_right_n(output, len_info.pads, padding_unit);
            break;

        case PADDING_BOTH_L:
        {
            auto const padding_right = len_info.pads / 2;
            auto const padding_left = len_info.pads - padding_right; // This is GEQ padding_right
            output = write_output_left_n(output, padding_left, padding_unit);
            output = write_output_right_n(output, padding_right, padding_unit);
            break;
        }

        case PADDING_BOTH_R:
        {
            auto const padding_left = len_info.pads / 2;
            auto const padding_right = len_info.pads - padding_left; // This is GEQ padding_left
            output = write_output_right_n(output, padding_right, padding_unit);
            output = write_output_left_n(output, padding_left, padding_unit);
            break;
        }

        default:
            // How the fuck!?
            CUTL_ASSUME(0);
            return CUTL_RESULT_FAILURE;
        }
    }

    // Next, we do the sign
    switch (len_info.sign)
    {
    case SIGN_POSITIVE:
        output = write_output_left(output, spec.sign_spec->plus);
        break;

    case SIGN_NEGATIVE:
        output = write_output_left(output, spec.sign_spec->minus);
        break;

    case SIGN_NONE:
        // No sign?
        break;
    }

    // We can now write digits
    // Get the absolute value
    auto abs_value = (uintmax_t)(value < 0 ? -value : value);
    unsigned minor_separator_counter = 0;
    do
    {
        // Write the next digit
        auto const next_digit = extract_next_digit(&abs_value, spec.digit_spec->base);
        output = write_output_right(output, spec.digit_spec->digits[next_digit]);
        minor_separator_counter += 1;
        // Write every `separator_distance` units, except if this is the last place
        if (abs_value && minor_separator_counter == spec.separator_spec->separator_distance)
        {
            output = write_output_right(output, spec.separator_spec->minor_separator);
            minor_separator_counter = 0;
        }
    } while (abs_value);

    if (len_info.padded_zeros != 0)
    {
        if (spec.separator_spec->separator_distance == 0 ||
            spec.separator_spec->separator_distance >= minor_separator_counter + len_info.padded_zeros)
        {
            // We don't have to deal with separators
            output = write_output_right_n(output, len_info.padded_zeros, spec.digit_spec->digits[0]);
        }
        else
        {
            // We do have to deal with separators
            for (unsigned i = 0; i < len_info.padded_zeros; ++i)
            {
                output = write_output_right(output, spec.digit_spec->digits[0]);
                // Write every `separator_distance` units, except if this is the last place
                minor_separator_counter += 1;
                if (i + 1 != len_info.padded_zeros &&
                    minor_separator_counter == spec.separator_spec->separator_distance)
                {
                    output = write_output_right(output, spec.separator_spec->minor_separator);
                    minor_separator_counter = 0;
                }
            }
        }
    }

    CUTL_ASSERT(output.length == 0, "Output length mismatch.");
    return CUTL_SUCCESS;
}

const digit_spec_c8_t DIGIT_SPEC_DECIMAL = {
    .base = 10,
    .digits =
        (const string8_t[10]){
            [0] = string8_from_literal(u8"0"),
            [1] = string8_from_literal(u8"1"),
            [2] = string8_from_literal(u8"2"),
            [3] = string8_from_literal(u8"3"),
            [4] = string8_from_literal(u8"4"),
            [5] = string8_from_literal(u8"5"),
            [6] = string8_from_literal(u8"6"),
            [7] = string8_from_literal(u8"7"),
            [8] = string8_from_literal(u8"8"),
            [9] = string8_from_literal(u8"9"),
        },
    .minimum_count = 1,
};

const digit_spec_c8_t DIGIT_SPEC_HEXADECIMAL = {
    .base = 16,
    .digits =
        (const string8_t[16]){
            [0] = string8_from_literal(u8"0"),
            [1] = string8_from_literal(u8"1"),
            [2] = string8_from_literal(u8"2"),
            [3] = string8_from_literal(u8"3"),
            [4] = string8_from_literal(u8"4"),
            [5] = string8_from_literal(u8"5"),
            [6] = string8_from_literal(u8"6"),
            [7] = string8_from_literal(u8"7"),
            [8] = string8_from_literal(u8"8"),
            [9] = string8_from_literal(u8"9"),
            [10] = string8_from_literal(u8"A"),
            [11] = string8_from_literal(u8"B"),
            [12] = string8_from_literal(u8"C"),
            [13] = string8_from_literal(u8"D"),
            [14] = string8_from_literal(u8"E"),
            [15] = string8_from_literal(u8"F"),
        },
    .minimum_count = 1,
};

const digit_spec_c8_t DIGIT_SPEC_HEXADECIMAL_LOWER = {
    .base = 16,
    .digits =
        (const string8_t[16]){
            [0] = string8_from_literal(u8"0"),
            [1] = string8_from_literal(u8"1"),
            [2] = string8_from_literal(u8"2"),
            [3] = string8_from_literal(u8"3"),
            [4] = string8_from_literal(u8"4"),
            [5] = string8_from_literal(u8"5"),
            [6] = string8_from_literal(u8"6"),
            [7] = string8_from_literal(u8"7"),
            [8] = string8_from_literal(u8"8"),
            [9] = string8_from_literal(u8"9"),
            [10] = string8_from_literal(u8"a"),
            [11] = string8_from_literal(u8"b"),
            [12] = string8_from_literal(u8"c"),
            [13] = string8_from_literal(u8"d"),
            [14] = string8_from_literal(u8"e"),
            [15] = string8_from_literal(u8"f"),
        },
    .minimum_count = 1,
};

const separator_spec_c8_t SEPARATOR_SPEC_NONE = {
    .separator_distance = 0,
    .major_separator = string8_from_literal(u8"."),
};

const separator_spec_c8_t SEPARATOR_SPEC_ISO = {
    .minor_separator = string8_from_literal(u8" "),
    .major_separator = string8_from_literal(u8"."),
    .separator_distance = 3,
};

const sign_spec_c8_t SIGN_SPEC_BASIC = {
    .plus = string8_from_literal(u8"+"),
    .minus = string8_from_literal(u8"-"),
    .always_sign = false,
};
