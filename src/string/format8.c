#include "format8.h"

#include "../common_defs.h"

#include <float.h>
#include <math.h>

/**
 * Extracts the next least significant digit from the given value based on the specified numeric base.
 * The input value is modified in-place by dividing it by the base.
 *
 * @param v A pointer to the numeric value from which the next digit will be extracted.
 *          The value is updated in-place by integer division with the base.
 * @param base The numeric base used to determine the digit (e.g., 10 for decimal, 16 for hexadecimal).
 * @return The extracted digit, corresponding to the remainder of the division of *v by base.
 */
static unsigned extract_next_digit(uintmax_t *v, const unsigned base)
{
    if (*v == 0)
        return 0;

    auto const digit = *v % base;
    *v /= base;
    return digit;
}

/**
 * Enum specifying the sign that is to be used for representing the number.
 */
typedef enum : unsigned char
{
    SIGN_NONE = 0,     // No sign is to be printed (implicitly the number was positive)
    SIGN_POSITIVE = 1, // Print the positive sign (explicitly positive)
    SIGN_NEGATIVE = 2, // Print the negative sign (explicitly negative)
} number_sign_t;

/**
 * Type used to return internal sizing information.
 */
typedef struct
{
    unsigned bytes; // Number of bytes to store all character units
    unsigned units; // Number of character units
} character_requirements_t;

/**
 * Computes the character requirements for representing a numeric value as a string based on the provided digit
 * specification and minimum digit requirements. The character requirements include the total number of bytes required
 * for storage and the number of character units used.
 *
 * @param value The numeric value for which the character requirements will be computed.
 * @param digit_spec A pointer to the digit specification that defines the numeric base and the properties of the
 * individual digits.
 * @param min_digits The minimum number of digit units required in the output representation.
 * @return A structure containing the total number of bytes and the total number of character units required to
 * represent the numeric value.
 */
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

/**
 * Computes the character requirements (number of bytes and character units) for formatting an integer
 * based on the provided specification.
 *
 * @param spec A pointer to the integer specification. This includes details for formatting such as digit,
 *             sign, separator, padding specifications, and minimum digit count.
 * @param value The integer value to compute character requirements for.
 * @return A structure containing the total number of bytes and character units required to format the integer.
 */
static character_requirements_t compute_integer_requirements(const integer_spec_c8_t *const spec, const intmax_t value)
{
    size_t len = 0;
    size_t units = 0;
    // Sign
    if (value < 0)
    {
        len += spec->sign_spec->minus.length;
        units += 1;
    }
    else if (spec->always_sign)
    {
        len += spec->sign_spec->plus.length;
        units += 1;
    }

    auto const abs_value = (uintmax_t)(value < 0 ? -value : value);
    // Digits
    auto const req = compute_digit_requirements(abs_value, spec->digit_spec, spec->minimum_digits);
    len += req.bytes;
    units += req.units;

    // Separators (integers only have minor separators)

    auto const separator_cnt = (spec->separator_spec->separator_distance && req.units)
                                   ? (req.units - 1) / spec->separator_spec->separator_distance
                                   : 0;

    units += separator_cnt;
    len += separator_cnt * spec->separator_spec->minor_separator.length;

    return (character_requirements_t){.bytes = len, .units = units};
}

/**
 * Padding specification, which does not pad.
 */
static const padding_spec_c8_t PADDING_SPEC_NONE = {.padding_max = 0};

/**
 * Fill the integer specifications with default values if none are used.
 *
 * @param spec Pointer to the specification struct to fill out.
 */
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
    auto const requirements = compute_integer_requirements(&spec, value);
    if (requirements.units < spec.padding_spec->padding_max)
    {
        auto const padding_units = spec.padding_spec->padding_max - requirements.units;
        return requirements.bytes + spec.padding_spec->padding.length * padding_units;
    }

    return requirements.bytes;
}

/**
 * Write a string to the output on the left side, then return the remaining part of the destination string.
 *
 * @param output String to write to.
 * @param str String to write.
 * @return Remaining part of the string.
 */
static string8_t write_output_left(const string8_t output, const string8_t str)
{
    memcpy(output.data, str.data, str.length);
    return string8_advance(output, str.length);
}

/**
 * Write a string to the output on the left side `repeats` times, then return the remaining part of the destination
 * string.
 *
 * @param output String to write to.
 * @param repeats How times to repeat the writing.
 * @param str String to write.
 * @return Remaining part of the string.
 */
static string8_t write_output_left_n(string8_t output, const unsigned repeats, string8_t const str)
{
    for (unsigned i = 0; i < repeats; ++i)
    {
        output = write_output_left(output, str);
    }
    return output;
}

/**
 * Write a string to the output on the right side, then return the remaining part of the destination string.
 *
 * @param output String to write to.
 * @param str String to write.
 * @return Remaining part of the string.
 */
static string8_t write_output_right(const string8_t output, const string8_t str)
{
    memcpy(output.data + output.length - str.length, str.data, str.length);
    return string8_shrink(output, str.length);
}

/**
 * Write a string to the output on the right side `repeats` times, then return the remaining part of the destination
 * string.
 *
 * @param output String to write to.
 * @param repeats How times to repeat the writing.
 * @param str String to write.
 * @return Remaining part of the string.
 */
static string8_t write_output_right_n(string8_t output, const unsigned repeats, string8_t const str)
{
    for (unsigned i = 0; i < repeats; ++i)
    {
        output = write_output_right(output, str);
    }
    return output;
}

/**
 * Pad the string with a specified padding unit.
 *
 * @param p_output Pointer to the output string to pad.
 * @param direction Padding direction.
 * @param padding_unit What to use for padding.
 * @param pads Number of padding units to write in total.
 * @return CUTL_RESULT_FAILURE if the enum value was not correct.
 */
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

/**
 * Write a sign to a string on the left side.
 *
 * @param p_output String to write to.
 * @param sign Sign to write to the string.
 * @param spec Specifications for the signs.
 */
static void write_sign_ltr(string8_t *const p_output, const number_sign_t sign, const sign_spec_c8_t *const spec)
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

/**
 * Write a sign to a string on the right side.
 *
 * @param p_output String to write to.
 * @param sign Sign to write to the string.
 * @param spec Specifications for the signs.
 */
static void write_sign_rtl(string8_t *const p_output, const number_sign_t sign, const sign_spec_c8_t *const spec)
{
    switch (sign)
    {
    case SIGN_POSITIVE:
        *p_output = write_output_right(*p_output, spec->plus);
        break;
    case SIGN_NEGATIVE:
        *p_output = write_output_right(*p_output, spec->minus);
        break;
    case SIGN_NONE:
        break;
    }
}

/**
 * Write an integer value right-to-left.
 *
 * @param output String to write to.
 * @param abs_value Absolute value of the integer to write.
 * @param digit_spec Specifications of the digits.
 * @param separator_spec Specifications of the separators.
 * @param minimum_digits Minimum number of digits to be written, with zeros being added on the left if needed.
 * @return Remainder of the string that was written to.
 */
static string8_t write_integer_with_separators_rtl(string8_t output, uintmax_t abs_value,
                                                   const digit_spec_c8_t *digit_spec,
                                                   const separator_spec_c8_t *separator_spec,
                                                   const unsigned minimum_digits)
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

/**
 * Write an integer value left-to-right.
 *
 * @param output String to write to.
 * @param abs_value Absolute value of the integer to write.
 * @param digit_spec Specifications of the digits.
 * @param separator_spec Specifications of the separators.
 * @param minimum_digits Minimum number of digits to be written, with zeros being added on the right if needed.
 * @return Remainder of the string that was written to.
 */
static string8_t write_integer_with_separators_ltr(string8_t output, uintmax_t abs_value,
                                                   const digit_spec_c8_t *digit_spec,
                                                   const separator_spec_c8_t *separator_spec,
                                                   const unsigned minimum_digits)
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
    CUTL_ASSERT(digit_count <= minimum_digits, "Digit count exceeds maximum_digits.");

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

    auto const trailing_zeros = minimum_digits <= digit_count ? 0 : minimum_digits - digit_count;
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
    auto len_info = compute_integer_requirements(&spec, value);
    auto const padding_units =
        (len_info.units < spec.padding_spec->padding_max) ? spec.padding_spec->padding_max - len_info.units : 0;
    len_info.bytes += spec.padding_spec->padding.length * padding_units;

    if (len_info.bytes != output.length)
    {
        return CUTL_RESULT_INSUFFICIENT_BUFFER;
    }
    number_sign_t sign = SIGN_NONE;
    if (value < 0)
    {
        sign = SIGN_NEGATIVE;
    }
    else if (spec.always_sign)
    {
        sign = SIGN_POSITIVE;
    }

    // First, write the padding
    if (padding_units)
    {
        auto const padding_unit = spec.padding_spec->padding;
        auto const res = pad_output(&output, spec.padding_spec->direction, padding_unit, padding_units);
        if (res != CUTL_SUCCESS)
            return res;
    }

    // Next, we do the sign
    write_sign_ltr(&output, sign, spec.sign_spec);

    // We can now write digits
    // Get the absolute value
    auto const abs_value = (uintmax_t)(value < 0 ? -value : value);
    output =
        write_integer_with_separators_rtl(output, abs_value, spec.digit_spec, spec.separator_spec, spec.minimum_digits);

    CUTL_ASSERT(output.length == 0, "Output length mismatch.");
    return CUTL_SUCCESS;
}

/**
 * Fill the float specifications with default values if none are used.
 *
 * @param spec Pointer to the specification struct to fill out.
 */
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

/**
 * Type to store the parsed float as integer part, fraction part, and the sign of the float.
 */
typedef struct
{
    uintmax_t integer_part;
    uintmax_t fraction_part;
    number_sign_t sign;
} float_info_t;

/**
 * Computes the character requirements (number of bytes and character units) for formatting a float
 * based on the provided specification.
 *
 * @param number Parsed float value.
 * @param spec A pointer to the float specification. This includes details for formatting such as digit,
 *             sign, separator, padding specifications, and minimum digit count.
 * @return A structure containing the total number of bytes and character units required to format the float.
 */
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

/**
 * Parse the floating point number into the integer part, fraction part, and sign to be written.
 *
 * @param value Value of the floating point number.
 * @param base Base in which the number will be represented.
 * @param fractional_digits Number of digits used for the fractional part.
 * @param always_sign True if the sign should be displayed for a positive number as well.
 * @return Parsed floating point number.
 */
static float_info_t parse_float(const double value, const unsigned base, const unsigned fractional_digits,
                                const bool always_sign)
{
    uintmax_t whole, frac_d;

    if (fractional_digits != 0)
    {
        uintmax_t frac_scaling_factor = 1;
        for (unsigned i = 0; i < fractional_digits; ++i)
        {
            frac_scaling_factor *= base;
        }
        // Split the value into whole and fractional parts
        double whole_d;
        auto const frac = modf(value, &whole_d);
        // Whole integer part (absolute value)
        whole = (uintmax_t)(whole_d < 0 ? -whole_d : whole_d);
        // Fraction part digits
        frac_d = (uintmax_t)round(frac * (double)frac_scaling_factor);
    }
    else
    {
        // We round to a whole number instead of splitting it into integer/fraction part
        frac_d = 0;
        whole = (uintmax_t)round(value);
    }

    number_sign_t sign = SIGN_NONE;
    if (value < 0)
        sign = SIGN_NEGATIVE;
    else if (always_sign)
        sign = SIGN_POSITIVE;

    return (float_info_t){.integer_part = whole, .fraction_part = frac_d, .sign = sign};
}

size_t format8_float_length(const double value, float_spec_c8_t spec)
{
    // Fill in the defaults
    float_spec_fill_defaults(&spec);

    auto const info = parse_float(value, spec.digit_spec->base, spec.fractional_digits, spec.always_sign);

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
    auto const info = parse_float(value, spec.digit_spec->base, spec.fractional_digits, spec.always_sign);
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
    write_sign_ltr(&output, info.sign, spec.sign_spec);

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

/**
 * Default exponent specifications.
 */
static const exponent_spec_c8_t EXPONENT_SPEC_DEFAULT = {
    .exponent_prefix = string8_from_literal(u8"E"),
    .exponent_suffix = (string8_t){},
    .use_separators = false,
};

/**
 * Fill the exponential specifications with default values if none are used.
 *
 * @param spec Pointer to the specification struct to fill out.
 */
static void exponential_spec_fill_defaults(exponential_spec_c8_t *spec)
{
    if (!spec->separator_spec)
        spec->separator_spec = &SEPARATOR_SPEC_NONE;
    if (!spec->digit_spec)
        spec->digit_spec = &DIGIT_SPEC_DECIMAL;
    if (!spec->sign_spec)
        spec->sign_spec = &SIGN_SPEC_BASIC;
    if (!spec->padding_spec)
        spec->padding_spec = &PADDING_SPEC_NONE;
    if (!spec->exponent_spec)
        spec->exponent_spec = &EXPONENT_SPEC_DEFAULT;
}

/**
 * Type used to store the information about the parsed exponential number.
 */
typedef struct
{
    uintmax_t integer_part;      // Integer part of the significant part
    uintmax_t fraction_part;     // Fraction part of the significant part
    number_sign_t sign;          // Sign of the number
    uintmax_t exponent;          // Absolute value of the exponent
    number_sign_t exponent_sign; // Sign of the exponent
} exponential_info_t;

/**
 * Parse the floating point number into the exponential representation.
 *
 * @param value Value of the floating point number.
 * @param base Base in which the number will be represented.
 * @param digits_fraction Number of digits used for the fractional part.
 * @param always_sign_value True if the sign should be displayed for a positive number as well.
 * @param always_sign_exponent True if the sign should be displayed for a positive exponent as well.
 * @return Parsed floating point number.
 */
static exponential_info_t parse_exponent(const double value, const unsigned base, const unsigned digits_fraction,
                                         const bool always_sign_value, const bool always_sign_exponent)
{
    intmax_t exponent;
    double significant;

    auto const exp_base_radix = ilogb(value);
    if (exp_base_radix != FP_ILOGB0)
    {
        auto const significant_base_radix = ldexp(value, -exp_base_radix);
        if (base == FLT_RADIX)
        {
            exponent = exp_base_radix;
            significant = significant_base_radix;
        }
        else
        {
            // Adjust the exponent to the new base
            auto const adjusted_exponent = exp_base_radix / log2(base);
            // Split it into the integer and fraction again
            double adjusted_exp;
            auto const frac = modf(adjusted_exponent, &adjusted_exp);
            exponent = (intmax_t)adjusted_exp;
            // Significant is multiplied by the noninteger part of the exponent
            significant = significant_base_radix * pow(base, frac);
            // Adjustment might have caused an overflow or an underflow in the significant
            if (significant > base)
            {
                exponent += 1;
                significant /= base;
            }
            else if (significant < 1)
            {
                exponent -= 1;
                significant *= base;
            }
        }
    }
    else
    {
        significant = 0.0;
        exponent = 0;
    }

    CUTL_ASSERT(significant < base, "Somehow the significant was not correctly adjusted!");

    auto const significant_info = parse_float(significant, base, digits_fraction, always_sign_value);
    uintmax_t exp_abs;
    number_sign_t exp_sign = SIGN_NONE;
    if (exponent < 0)
    {
        exp_abs = -exponent;
        exp_sign = SIGN_NEGATIVE;
    }
    else
    {
        exp_abs = exponent;
        if (always_sign_exponent)
            exp_sign = SIGN_POSITIVE;
    }

    return (exponential_info_t){
        .integer_part = significant_info.integer_part,
        .fraction_part = significant_info.fraction_part,
        .sign = significant_info.sign,
        .exponent = exp_abs,
        .exponent_sign = exp_sign,
    };
}

/**
 * Computes the character requirements (number of bytes and character units) for formatting an exponential float
 * based on the provided specification.
 *
 * @param number Parsed exponential float value.
 * @param spec A pointer to the exponential specification. This includes details for formatting such as digit,
 *             sign, separator, padding specifications, and minimum digit count.
 * @return A structure containing the total number of bytes and character units required to format the exponential
 * float.
 */
static character_requirements_t compute_exponential_requirements(const exponential_info_t number,
                                                                 const exponential_spec_c8_t *const spec)
{
    const float_spec_c8_t float_spec = {
        .digit_spec = spec->digit_spec,
        .sign_spec = spec->sign_spec,
        .separator_spec = spec->separator_spec,
        .padding_spec = spec->padding_spec,
        .minimum_digits = 1,
        .fractional_digits = spec->fractional_digits,
        .always_sign = spec->always_sign_value,
    };

    auto reqs = compute_float_requirements(
        (float_info_t){.integer_part = number.integer_part, .fraction_part = number.fraction_part, .sign = number.sign},
        &float_spec);

    if (number.exponent || !spec->skip_zero_exponent)
    {

        separator_spec_c8_t integer_separator_spec = *spec->separator_spec;
        if (!spec->exponent_spec->use_separators)
        {
            integer_separator_spec.separator_distance = 0;
        }
        const integer_spec_c8_t integer_spec = {
            .digit_spec = spec->digit_spec,
            .sign_spec = spec->sign_spec,
            .separator_spec = &integer_separator_spec,
            .padding_spec = spec->padding_spec,
            .minimum_digits = spec->minimum_exponent_digits,
            .always_sign = spec->always_exponent_sign,
        };

        auto const signed_exponent =
            number.exponent_sign == SIGN_NEGATIVE ? -(intmax_t)number.exponent : (intmax_t)number.exponent;
        auto const exponent_reqs = compute_integer_requirements(&integer_spec, signed_exponent);
        reqs.bytes += exponent_reqs.bytes + spec->exponent_spec->exponent_prefix.length;
        reqs.units += exponent_reqs.units + 1;
        if (spec->use_exponent_suffix)
        {
            reqs.bytes += spec->exponent_spec->exponent_suffix.length;
            reqs.units += 1;
        }
    }

    return reqs;
}

size_t format8_exponential_length(const double value, exponential_spec_c8_t spec)
{
    exponential_spec_fill_defaults(&spec);
    auto const info = parse_exponent(value, spec.digit_spec->base, spec.fractional_digits, spec.always_sign_value,
                                     spec.always_exponent_sign);

    auto const reqs = compute_exponential_requirements(info, &spec);

    // Deal with padding
    size_t bytes = reqs.bytes;
    if (reqs.units < spec.padding_spec->padding_max)
    {
        auto const padding_units = spec.padding_spec->padding_max - reqs.units;
        bytes += spec.padding_spec->padding.length * padding_units;
    }

    return bytes;
}

cutl_result_t format8_exponential(const double value, string8_t output, exponential_spec_c8_t spec)
{
    exponential_spec_fill_defaults(&spec);
    auto const info = parse_exponent(value, spec.digit_spec->base, spec.fractional_digits, spec.always_sign_value,
                                     spec.always_exponent_sign);

    auto reqs = compute_exponential_requirements(info, &spec);

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
    write_sign_ltr(&output, info.sign, spec.sign_spec);

    // Do the exponent
    if (info.exponent || !spec.skip_zero_exponent)
    {
        // We are doing the exponent (right to left)
        if (spec.use_exponent_suffix)
        {
            // We have a suffix
            output = write_output_right(output, spec.exponent_spec->exponent_suffix);
        }
        // Write the exponent
        auto separator_spec = *spec.separator_spec;
        if (!spec.exponent_spec->use_separators)
            separator_spec.separator_distance = 0;

        output = write_integer_with_separators_rtl(output, info.exponent, spec.digit_spec, &separator_spec,
                                                   spec.minimum_exponent_digits);

        // Sign
        write_sign_rtl(&output, info.exponent_sign, spec.sign_spec);

        // The prefix now
        output = write_output_right(output, spec.exponent_spec->exponent_prefix);
    }

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
    output = write_integer_with_separators_rtl(output, info.integer_part, spec.digit_spec, spec.separator_spec, 1);

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
