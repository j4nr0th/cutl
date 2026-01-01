#pragma once

#include "../format.h"
#include "string_stream.h"
#include "output_stream.h"

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

/**
 * Format multiple values into the output stream.
 *
 * If failure in formatting occurs during formatting, previous values are still written to the stream.
 *
 * @param this Output stream which to write the format to.
 * @param args Format specifications.
 * @return CUTL_SUCCESS if successful, otherwise an error code.
 */
cutl_result_t output_stream_format(output_stream_t *this, const fmt_arg_t args[]);


/**
 * Format multiple values into the string stream.
 *
 * If failure in formatting occurs during formatting, previous values are still written to the stream.
 *
 * @param this String stream which to write the format to.
 * @param args Format specifications.
 * @return CUTL_SUCCESS if successful, otherwise an error code.
 */
cutl_result_t string_stream_format(string_stream_t *this, const fmt_arg_t args[]);

