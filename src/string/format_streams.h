#pragma once

#include "format8.h"

typedef enum
{
    FMT_NONE = 0,
    FMT_INT,
    FMT_FLT,
    FMT_EXP,
    FMT_STR,
    FMT_S8,
    FMT_CUSTOM,
} fmt_type_t;

typedef size_t (*format_length_function)(void *param);
typedef int (*format_write_function)(void *param, size_t size, char8_t CUTL_ARRAY_ARG(buffer, size));

typedef struct
{
    fmt_type_t type;
    union {
        struct
        {
            intmax_t value;
            const integer_spec_c8_t *spec;
        } integer;
        struct
        {
            double value;
            const float_spec_c8_t *spec;
        } floating;
        struct
        {
            double value;
            const exponential_spec_c8_t *spec;
        } exponential;
        string8_t s8;
        const char *cstr;
        struct
        {
            format_length_function length_function;
            format_write_function write_function;
            void *param;
        } custom;
    };
} fmt_arg_t;
