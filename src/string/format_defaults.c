#include "format_defaults.h"

static const padding_spec_c8_t PADDING_NONE = {.padding_max = 0};

const integer_spec_c8_t DEFAULT_INT_SPECS = {
    .digit_spec = &DIGIT_SPEC_DECIMAL,
    .sign_spec = &SIGN_SPEC_BASIC,
    .separator_spec = &SEPARATOR_SPEC_NONE,
    .padding_spec = &PADDING_NONE,
    .minimum_digits = 1,
    .always_sign = false,
};

const float_spec_c8_t DEFAULT_FLT_SPECS = {
    .digit_spec = &DIGIT_SPEC_DECIMAL,
    .sign_spec = &SIGN_SPEC_BASIC,
    .separator_spec = &SEPARATOR_SPEC_NONE,
    .padding_spec = &PADDING_NONE,
    .minimum_digits = 1,
    .fractional_digits = 6,
    .always_sign = false,
};

static const exponent_spec_c8_t DEFAULT_EXPONENT_SPECS = {
    .exponent_prefix = string8_from_literal(u8"E"),
    .use_separators = false,
};
const exponential_spec_c8_t DEFAULT_EXP_SPECS = {
    .digit_spec = &DIGIT_SPEC_DECIMAL,
    .sign_spec = &SIGN_SPEC_BASIC,
    .separator_spec = &SEPARATOR_SPEC_NONE,
    .padding_spec = &PADDING_NONE,
    .exponent_spec = &DEFAULT_EXPONENT_SPECS,
    .fractional_digits = 6,
    .minimum_exponent_digits = 3,
    .always_sign_value = false,
    .always_exponent_sign = true,
    .skip_zero_exponent = false,
    .use_exponent_suffix = false,
};
