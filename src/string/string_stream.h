#pragma once

#include <stdio.h>
#include "../common_defs.h"
#include "../error.h"
#include "format_streams.h"

typedef struct
{
    size_t buffer_size;
    size_t buffer_pos;
    char8_t buffer[];
} string_stream_t;

cutl_result_t string_stream_init(size_t size, unsigned char CUTL_ARRAY_ARG(memory, size), string_stream_t **p_out);

cutl_result_t string_stream_write_s8(string_stream_t *this, string8_t str);

cutl_result_t string_stream_write_cstr(string_stream_t *this, const char *str);

cutl_result_t string_stream_write_integer(string_stream_t *this, intmax_t value, const integer_spec_c8_t *spec);

cutl_result_t string_stream_write_float(string_stream_t *this, double value, const float_spec_c8_t *spec);

cutl_result_t string_stream_write_exponential(string_stream_t *this, double value, const exponential_spec_c8_t *spec);

cutl_result_t string_stream_write_custom(string_stream_t *this, format_length_function length_function,
                                         format_write_function write_function, void *param);

string8_t string_stream_get_string(string_stream_t *this);

cutl_result_t string_stream_format(string_stream_t *this, const fmt_arg_t args[]);

void string_stream_clear(string_stream_t *this);

size_t string_stream_get_pos(const string_stream_t *this);

cutl_result_t string_stream_set_pos(string_stream_t *this, size_t pos);
