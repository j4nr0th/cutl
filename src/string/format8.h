#pragma once
#include "string8.h"

/**
 * Specification of how digits of a number are formatted.
 */
typedef struct
{
    unsigned base;           // Numeric base of digits
    const string8_t *digits; // Individual digits in increasing value.
} digit_spec_c8_t;

typedef struct
{
    string8_t plus;  // Sign to use for "+"
    string8_t minus; // Sign to use for "-"
} sign_spec_c8_t;

typedef struct
{
    string8_t minor_separator;   // Serves the purpose of thousands separators and appears every `separator_distance`
    string8_t major_separator;   // Serves for the separator of the integer and fractional part (like decimal dot)
    unsigned separator_distance; // How ofter to place the minor separators.
} separator_spec_c8_t;

typedef enum
{
    PADDING_LEFT,   // Pad on the left only
    PADDING_RIGHT,  // Pad on the right only
    PADDING_BOTH_L, // Pad on both sizes, with preference for left
    PADDING_BOTH_R, // Pad on both sides, with preference for right
} padding_direction_t;

typedef struct
{
    string8_t padding;             // What to pad the digits with
    unsigned padding_max;          // What size to pad the number to
    padding_direction_t direction; // Which direction to apply the padding to
} padding_spec_c8_t;

/**
 * Specifications of how integers are formatted.
 */
typedef struct
{
    const digit_spec_c8_t *digit_spec;         // Specifications for formatting digits
    const sign_spec_c8_t *sign_spec;           // Specifications for formatting signs
    const separator_spec_c8_t *separator_spec; // Specifications for formatting separators
    const padding_spec_c8_t *padding_spec;     // Specifications for formatting padding
    unsigned minimum_digits; // If fewer than this many digits are found, zeros are added ahead of the number
    bool always_sign;        // Always print the sign, even for positive numbers
} integer_spec_c8_t;

size_t format8_integer_length(intmax_t value, integer_spec_c8_t spec);

cutl_result_t format8_integer(intmax_t value, string8_t output, integer_spec_c8_t spec);

/**
 * Specifications of how floating point numbers are formatted.
 */
typedef struct
{
    const digit_spec_c8_t *digit_spec;         // Specifications for formatting digits
    const sign_spec_c8_t *sign_spec;           // Specifications for formatting signs
    const separator_spec_c8_t *separator_spec; // Specifications for formatting separators
    const padding_spec_c8_t *padding_spec;     // Specifications for formatting padding
    unsigned minimum_digits;    // If fewer than this many digits are found, zeros are added ahead of the number
    unsigned fractional_digits; // These many digits are shown for behind the decimal point
    bool always_sign;           // Always print the sign, even for positive numbers
} float_spec_c8_t;

size_t format8_float_length(double value, float_spec_c8_t spec);

cutl_result_t format8_float(double value, string8_t output, float_spec_c8_t spec);

typedef struct
{
    string8_t exponent_prefix; // To put in front of the exponent value
    string8_t exponent_suffix; // To put after the exponent value
    bool use_separators;       // Should we put minor separators in the value?
} exponent_spec_c8_t;

/**
 * Specifications of how floating point numbers are formatted.
 */
typedef struct
{
    const digit_spec_c8_t *digit_spec;         // Specifications for formatting digits
    const sign_spec_c8_t *sign_spec;           // Specifications for formatting signs
    const separator_spec_c8_t *separator_spec; // Specifications for formatting separators
    const padding_spec_c8_t *padding_spec;     // Specifications for formatting padding
    const exponent_spec_c8_t *exponent_spec;   // Specifications for formatting the exponent part
    unsigned fractional_digits;                // These many digits are shown for behind the decimal point
    unsigned minimum_exponent_digits;          // Always print at least this many digits for the exponent
    bool always_sign_value;                    // Always print the sign, even for positive numbers
    bool always_exponent_sign;                 // Always print the sign for the exponent, even for positive numbers
    bool skip_zero_exponent;                   // Do not print the exponent when it is 0
    bool use_exponent_suffix;                  // Print the exponent suffix
} exponential_spec_c8_t;

size_t format8_exponential_length(double value, exponential_spec_c8_t spec);

cutl_result_t format8_exponential(double value, string8_t output, exponential_spec_c8_t spec);

// Some common specs
extern const digit_spec_c8_t DIGIT_SPEC_DECIMAL;
extern const digit_spec_c8_t DIGIT_SPEC_HEXADECIMAL;
extern const digit_spec_c8_t DIGIT_SPEC_HEXADECIMAL_LOWER;

extern const separator_spec_c8_t SEPARATOR_SPEC_NONE;
extern const separator_spec_c8_t SEPARATOR_SPEC_ISO;

extern const sign_spec_c8_t SIGN_SPEC_BASIC;
