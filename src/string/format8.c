#include "format8.h"

#include "../common_defs.h"
#include <math.h>

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
    if (*v == 0)
        return 0;

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

typedef struct
{
    size_t length;
    size_t digit_count;
    unsigned padded_zeros;
    number_sign_t sign;
} number_counts_t;

static struct
{
    size_t length;
    size_t digit_count;
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
    else if (spec->always_sign)
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
    if (digit_count < spec->minimum_digits)
    {
        padded_zeros = spec->minimum_digits - digit_count;
        len += spec->digit_spec->digits[0].length * padded_zeros;
    }

    // Separators (integers only have minor separators)
    auto const total_digits = digit_count + padded_zeros;
    auto const separator_cnt = (spec->separator_spec->separator_distance && total_digits)
                                   ? (total_digits - 1) / spec->separator_spec->separator_distance
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

static cutl_result_t pad_output(string8_t *const p_output, const padding_direction_t direction,
                                const string8_t padding_unit, const unsigned pads)
{
    auto output = *p_output;
    switch (direction)
    {
    case PADDING_LEFT:
        output = write_output_left_n(output, pads, padding_unit);
        break;

    case PADDING_RIGHT:
        output = write_output_right_n(output, pads, padding_unit);
        break;

    case PADDING_BOTH_L:
    {
        auto const padding_right = pads / 2;
        auto const padding_left = pads - padding_right; // This is GEQ padding_right
        output = write_output_left_n(output, padding_left, padding_unit);
        output = write_output_right_n(output, padding_right, padding_unit);
        break;
    }

    case PADDING_BOTH_R:
    {
        auto const padding_left = pads / 2;
        auto const padding_right = pads - padding_left; // This is GEQ padding_left
        output = write_output_right_n(output, padding_right, padding_unit);
        output = write_output_left_n(output, padding_left, padding_unit);
        break;
    }

    default:
        // How the fuck!?
        CUTL_ASSUME(0);
        return CUTL_RESULT_FAILURE;
    }
    *p_output = output;
    return CUTL_SUCCESS;
}

static void write_sign(string8_t *const p_output, const number_sign_t sign, const sign_spec_c8_t *const spec)
{
    switch (sign)
    {
    case SIGN_POSITIVE:
        *p_output = write_output_left(*p_output, spec->plus);
        break;
    case SIGN_NEGATIVE:
        *p_output = write_output_left(*p_output, spec->minus);
        break;
    case SIGN_NONE:
        break;
    }
}

string8_t write_integer_with_separators_rtl(string8_t output, uintmax_t abs_value, const digit_spec_c8_t *digit_spec,
                                            const separator_spec_c8_t *separator_spec, const unsigned minimum_digits)
{
    unsigned minor_separator_counter = 0, written_digits = 0;
    while (abs_value)
    {
        // Write the next digit
        auto const next_digit = extract_next_digit(&abs_value, digit_spec->base);
        output = write_output_right(output, digit_spec->digits[next_digit]);
        minor_separator_counter += 1;
        // Write every `separator_distance` units, except if this is the last place
        if (abs_value && minor_separator_counter == separator_spec->separator_distance)
        {
            output = write_output_right(output, separator_spec->minor_separator);
            minor_separator_counter = 0;
        }
        written_digits += 1;
    }

    auto const leading_zeros = minimum_digits <= written_digits ? 0 : minimum_digits - written_digits;
    if (leading_zeros != 0)
    {
        if (separator_spec->separator_distance == 0 ||
            separator_spec->separator_distance >= minor_separator_counter + leading_zeros)
        {
            // We don't have to deal with separators
            output = write_output_right_n(output, leading_zeros, digit_spec->digits[0]);
        }
        else
        {
            // We do have to deal with separators
            for (unsigned i = 0; i < leading_zeros; ++i)
            {
                output = write_output_right(output, digit_spec->digits[0]);
                // Write every `separator_distance` units, except if this is the last place
                minor_separator_counter += 1;
                if (i + 1 != leading_zeros && minor_separator_counter == separator_spec->separator_distance)
                {
                    output = write_output_right(output, separator_spec->minor_separator);
                    minor_separator_counter = 0;
                }
            }
        }
    }

    return output;
}

string8_t write_integer_with_separators_ltr(string8_t output, uintmax_t abs_value, const digit_spec_c8_t *digit_spec,
                                            const separator_spec_c8_t *separator_spec, const unsigned maximum_digits)
{
    // Find the number of digits in the abs value
    unsigned digit_count = 0;
    uintmax_t divisor = 1;
    auto const base = digit_spec->base;
    while (divisor <= abs_value)
    {
        divisor *= base;
        digit_count += 1;
    }
    CUTL_ASSERT(digit_count <= maximum_digits, "Digit count exceeds maximum_digits.");

    unsigned minor_separator_counter = 0;

    for (unsigned i = 0; i < digit_count; ++i)
    {
        divisor /= base;
        auto const digit = abs_value / divisor;
        abs_value %= divisor;
        output = write_output_left(output, digit_spec->digits[digit]);
        minor_separator_counter += 1;
        if (minor_separator_counter == separator_spec->separator_distance && i + 1 != digit_count)
        {
            minor_separator_counter = 0;
            output = write_output_left(output, separator_spec->minor_separator);
        }
    }

    auto const trailing_zeros = maximum_digits <= digit_count ? 0 : maximum_digits - digit_count;
    if (trailing_zeros != 0)
    {
        if (separator_spec->separator_distance == 0 ||
            separator_spec->separator_distance >= minor_separator_counter + trailing_zeros)
        {
            // We don't have to deal with separators
            output = write_output_left_n(output, trailing_zeros, digit_spec->digits[0]);
        }
        else
        {
            // We do have to deal with separators
            for (unsigned i = 0; i < trailing_zeros; ++i)
            {
                output = write_output_left(output, digit_spec->digits[0]);
                // Write every `separator_distance` units, except if this is the last place
                minor_separator_counter += 1;
                if (i + 1 != trailing_zeros && minor_separator_counter == separator_spec->separator_distance)
                {
                    output = write_output_left(output, separator_spec->minor_separator);
                    minor_separator_counter = 0;
                }
            }
        }
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
        auto const res = pad_output(&output, spec.padding_spec->direction, padding_unit, len_info.pads);
        if (res != CUTL_SUCCESS)
            return res;
    }

    // Next, we do the sign
    write_sign(&output, len_info.sign, spec.sign_spec);

    // We can now write digits
    // Get the absolute value
    auto const abs_value = (uintmax_t)(value < 0 ? -value : value);
    output =
        write_integer_with_separators_rtl(output, abs_value, spec.digit_spec, spec.separator_spec, spec.minimum_digits);

    CUTL_ASSERT(output.length == 0, "Output length mismatch.");
    return CUTL_SUCCESS;
}

static void float_spec_fill_defaults(float_spec_c8_t *spec)
{
    if (!spec->separator_spec)
        spec->separator_spec = &SEPARATOR_SPEC_NONE;
    if (!spec->digit_spec)
        spec->digit_spec = &DIGIT_SPEC_DECIMAL;
    if (!spec->sign_spec)
        spec->sign_spec = &SIGN_SPEC_BASIC;
    if (!spec->padding_spec)
        spec->padding_spec = &PADDING_SPEC_NONE;
}

typedef struct
{
    unsigned bytes; // Number of bytes to store all character units
    unsigned units; // Number of character units
} character_requirements_t;

static character_requirements_t compute_digit_requirements(uintmax_t value, const digit_spec_c8_t *const digit_spec,
                                                           const unsigned min_digits)
{
    character_requirements_t reqs = {};

    while (value)
    {
        auto const digit = extract_next_digit(&value, digit_spec->base);
        reqs.bytes += digit_spec->digits[digit].length;
        reqs.units += 1;
    }
    if (reqs.units < min_digits)
    {
        reqs.bytes += digit_spec->digits[0].length * (min_digits - reqs.units);
        reqs.units += min_digits - reqs.units;
    }

    return reqs;
}

typedef struct
{
    uintmax_t integer_part;
    uintmax_t fraction_part;
    number_sign_t sign;
} float_info_t;

static character_requirements_t compute_float_requirements(const float_info_t number, const float_spec_c8_t *const spec)
{
    auto const integer_v = number.integer_part;
    auto const fraction_v = number.fraction_part;
    auto const sign = number.sign;
    // Deal with the sign
    const character_requirements_t integer_reqs =
        compute_digit_requirements(integer_v, spec->digit_spec, spec->minimum_digits);
    const character_requirements_t fraction_reqs =
        compute_digit_requirements(fraction_v, spec->digit_spec, spec->fractional_digits);
    // Combine the requirements and add the major separator
    auto total_requirements = (character_requirements_t){
        .bytes = integer_reqs.bytes + fraction_reqs.bytes + spec->separator_spec->major_separator.length,
        .units = fraction_reqs.units + integer_reqs.units + 1,
    };
    // Add the minor separators for both the integer and the fraction parts
    if (spec->separator_spec->separator_distance)
    {
        if (integer_reqs.units)
        {
            auto const integer_separators = (integer_reqs.units - 1) / spec->separator_spec->separator_distance;
            total_requirements.units += integer_separators;
            total_requirements.bytes += spec->separator_spec->minor_separator.length * integer_separators;
        }
        if (fraction_reqs.units)
        {
            auto const fraction_separators = (fraction_reqs.units - 1) / spec->separator_spec->separator_distance;
            total_requirements.units += fraction_separators;
            total_requirements.bytes += spec->separator_spec->minor_separator.length * fraction_separators;
        }
    }

    // Deal with the sign
    if (sign == SIGN_NEGATIVE)
    {
        total_requirements.bytes += spec->sign_spec->minus.length;
        total_requirements.units += 1;
    }
    else if (spec->always_sign)
    {
        total_requirements.bytes += spec->sign_spec->plus.length;
        total_requirements.units += 1;
    }

    // Write the

    return total_requirements;
}

static float_info_t parse_float(const double value, const float_spec_c8_t *const spec)
{
    auto const base = spec->digit_spec->base;
    uintmax_t frac_scaling_factor = 1;
    for (unsigned i = 0; i < spec->fractional_digits; ++i)
    {
        frac_scaling_factor *= base;
    }

    // Split the value into whole and fractional parts
    double whole_d;
    auto const frac = modf(value, &whole_d);
    // Whole integer part (absolute value)
    auto const whole = (uintmax_t)(whole_d < 0 ? -whole_d : whole_d);
    // Fraction part digits

    auto const frac_d = (uintmax_t)round(frac * (double)frac_scaling_factor);
    number_sign_t sign = SIGN_NONE;
    if (whole_d < 0)
        sign = SIGN_NEGATIVE;
    else if (spec->always_sign)
        sign = SIGN_POSITIVE;

    return (float_info_t){.integer_part = whole, .fraction_part = frac_d, .sign = sign};
}

size_t format8_float_length(const double value, float_spec_c8_t spec)
{
    // Fill in the defaults
    float_spec_fill_defaults(&spec);

    auto const info = parse_float(value, &spec);

    // Get the number requirements
    auto reqs = compute_float_requirements(info, &spec);

    // Check for padding
    if (reqs.units < spec.padding_spec->padding_max)
    {
        auto const padding_units = spec.padding_spec->padding_max - reqs.units;
        reqs.bytes += spec.padding_spec->padding.length * padding_units;
        reqs.units += padding_units;
    }

    return reqs.bytes;
}

cutl_result_t format8_float(const double value, string8_t output, float_spec_c8_t spec)
{
    float_spec_fill_defaults(&spec);
    auto const info = parse_float(value, &spec);
    // Check the length
    auto reqs = compute_float_requirements(info, &spec);

    auto const padding_units =
        (reqs.units < spec.padding_spec->padding_max) ? spec.padding_spec->padding_max - reqs.units : 0;
    reqs.bytes += spec.padding_spec->padding.length * padding_units;

    if (reqs.bytes != output.length)
        return CUTL_RESULT_INSUFFICIENT_BUFFER;

    // Deal with padding
    if (padding_units)
    {
        auto const res = pad_output(&output, spec.padding_spec->direction, spec.padding_spec->padding, padding_units);
        if (res != CUTL_SUCCESS)
            return res;
    }
    // Next, we do the sign
    write_sign(&output, info.sign, spec.sign_spec);

    // Fraction part (left to right)
    {
        auto const remaining_output = write_integer_with_separators_ltr(output, info.fraction_part, spec.digit_spec,
                                                                        spec.separator_spec, spec.fractional_digits);
        auto const written_bytes = output.length - remaining_output.length;
        // Move the fraction part to the end of the output buffer
        memmove(output.data + output.length - written_bytes, output.data, written_bytes);
        output.length -= written_bytes;
    }
    // Major separator
    output = write_output_right(output, spec.separator_spec->major_separator);

    // Integer part
    output = write_integer_with_separators_rtl(output, info.integer_part, spec.digit_spec, spec.separator_spec,
                                               spec.minimum_digits);

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
};

const separator_spec_c8_t SEPARATOR_SPEC_NONE = {
    .major_separator = string8_from_literal(u8"."),
};

const separator_spec_c8_t SEPARATOR_SPEC_ISO = {
    .minor_separator = string8_from_literal(u8" "),
    .major_separator = string8_from_literal(u8"."),
};

const sign_spec_c8_t SIGN_SPEC_BASIC = {
    .plus = string8_from_literal(u8"+"),
    .minus = string8_from_literal(u8"-"),
};
