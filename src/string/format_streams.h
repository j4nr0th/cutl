#pragma once

#include "format8.h"

/**
 * Enum specifying the type of the format argument.
 */
typedef enum
{
    FMT_END = 0, // This marks the last argument.
    FMT_INT,     // Integer value.
    FMT_FLT,     // Floating point value.
    FMT_EXP,     // Floating point value in exponential notation.
    FMT_STR,     // C-style null-terminated string.
    FMT_S8,      // string8_t.
    FMT_CUSTOM,  // Custom length and print functions are provided.
} fmt_type_t;

/**
 * Function that returns the size of the buffer needed to format the custom value.
 */
typedef size_t (*format_length_function)(void *param);

/**
 * Function that formats the custom value to the buffer, which was previously sized.
 */
typedef int (*format_write_function)(void *param, size_t size, char8_t CUTL_ARRAY_ARG(buffer, size));

/**
 * Type used to pass arguments for formatting streams.
 */
typedef struct
{
    fmt_type_t type; // Type of the value in this struct.
    union {
        struct
        {
            intmax_t value;
            const integer_spec_c8_t *spec;
        } integer; // FMT_INT
        struct
        {
            double value;
            const float_spec_c8_t *spec;
        } floating; // FMT_FLT
        struct
        {
            double value;
            const exponential_spec_c8_t *spec;
        } exponential;    // FMT_EXP
        string8_t s8;     // FMT_S8
        const char *cstr; //  FMT_STR
        struct
        {
            format_length_function length_function;
            format_write_function write_function;
            void *param;
        } custom; // FMT_CUSTOM
    };
} fmt_arg_t;
