#pragma once

#include <stdio.h>

#include "../common_defs.h"
#include "../error.h"
#include "format_streams.h"

typedef struct
{
    FILE *stream;
    size_t buffer_size;
    size_t buffer_pos;
    char8_t buffer[];
} output_stream_t;

cutl_result_t output_stream_init(FILE *stream, size_t size, unsigned char CUTL_ARRAY_ARG(memory, size),
                                 output_stream_t **p_out);

void output_stream_flush(output_stream_t *this);

void output_stream_write_s8(output_stream_t *this, string8_t str);

void output_stream_write_cstr(output_stream_t *this, const char *str);

cutl_result_t output_stream_write_integer(output_stream_t *this, intmax_t value, const integer_spec_c8_t *spec);

cutl_result_t output_stream_write_float(output_stream_t *this, double value, const float_spec_c8_t *spec);

cutl_result_t output_stream_write_exponential(output_stream_t *this, double value, const exponential_spec_c8_t *spec);

cutl_result_t output_stream_write_custom(output_stream_t *this, format_length_function length_function,
                                         format_write_function write_function, void *param);

cutl_result_t output_stream_format(output_stream_t *this, const fmt_arg_t args[]);
