#include "string_stream.h"
#include "format_defaults.h"

cutl_result_t string_stream_init(const size_t size, unsigned char CUTL_ARRAY_ARG(memory, size),
                                 string_stream_t **const p_out)
{
    if (size < sizeof(string_stream_t))
        return CUTL_RESULT_INSUFFICIENT_BUFFER;

    auto const this = (string_stream_t *)memory;
    this->buffer_size = size - sizeof(string_stream_t);
    this->buffer_pos = 0;

    *p_out = this;
    return CUTL_SUCCESS;
}

cutl_result_t string_stream_write_s8(string_stream_t *this, const string8_t str)
{
    auto const remaining_space = this->buffer_size - this->buffer_pos;
    if (remaining_space < str.length)
        return CUTL_RESULT_INSUFFICIENT_BUFFER;

    memcpy(this->buffer + this->buffer_pos, str.data, str.length);
    this->buffer_pos += str.length;
    return CUTL_SUCCESS;
}

cutl_result_t string_stream_write_cstr(string_stream_t *this, const char *str)
{
    auto const len = strlen(str);
    if (len > this->buffer_size - this->buffer_pos)
        return CUTL_RESULT_INSUFFICIENT_BUFFER;
    memcpy(this->buffer + this->buffer_pos, str, len);
    this->buffer_pos += len;
    return CUTL_SUCCESS;
}

/**
 * Get a string of the desired length from a string stream, which will be written to, while also moving the position
 * forward.
 *
 * @param this String stream to get the string to write to.
 * @param required_memory Required size of the output string.
 * @return String which is to be used to write to.
 */
static string8_t string_stream_get_output_string(string_stream_t *this, const size_t required_memory)
{
    CUTL_ASSERT(required_memory <= this->buffer_size - this->buffer_pos, "Buffer too small for output string.");
    auto const output = (string8_t){required_memory, this->buffer + this->buffer_pos};
    this->buffer_pos += required_memory;
    return output;
}

cutl_result_t string_stream_write_integer(string_stream_t *this, const intmax_t value, const integer_spec_c8_t *spec)
{
    if (!spec)
        spec = &DEFAULT_INT_SPECS;

    auto const required_memory = format8_integer_length(value, *spec);
    if (required_memory > this->buffer_size)
        return CUTL_RESULT_INSUFFICIENT_BUFFER;

    auto const output = string_stream_get_output_string(this, required_memory);
    auto const res = format8_integer(value, output, *spec);
    if (res != CUTL_SUCCESS)
    {
        this->buffer_pos -= output.length;
    }
    return res;
}

cutl_result_t string_stream_write_float(string_stream_t *this, const double value, const float_spec_c8_t *spec)
{
    if (!spec)
        spec = &DEFAULT_FLT_SPECS;

    auto const required_memory = format8_float_length(value, *spec);
    if (required_memory > this->buffer_size)
        return CUTL_RESULT_INSUFFICIENT_BUFFER;

    auto const output = string_stream_get_output_string(this, required_memory);
    auto const res = format8_float(value, output, *spec);
    if (res != CUTL_SUCCESS)
    {
        this->buffer_pos -= output.length;
    }
    return res;
}

cutl_result_t string_stream_write_exponential(string_stream_t *this, const double value,
                                              const exponential_spec_c8_t *spec)
{
    if (!spec)
        spec = &DEFAULT_EXP_SPECS;

    auto const required_memory = format8_exponential_length(value, *spec);
    if (required_memory > this->buffer_size)
        return CUTL_RESULT_INSUFFICIENT_BUFFER;

    auto const output = string_stream_get_output_string(this, required_memory);
    auto const res = format8_exponential(value, output, *spec);
    if (res != CUTL_SUCCESS)
    {
        this->buffer_pos -= output.length;
    }
    return res;
}

cutl_result_t string_stream_write_custom(string_stream_t *this, const format_length_function length_function,
                                         const format_write_function write_function, void *param)
{
    auto const required_memory = length_function(param);
    if (required_memory > this->buffer_size)
        return CUTL_RESULT_INSUFFICIENT_BUFFER;

    auto const output = string_stream_get_output_string(this, required_memory);
    if (write_function(param, output.length, output.data))
    {
        this->buffer_pos -= output.length;
        return CUTL_RESULT_CALLBACK_FAILURE;
    }
    return CUTL_SUCCESS;
}

string8_t string_stream_get_string(string_stream_t *this)
{
    return (string8_t){this->buffer_pos, this->buffer};
}

cutl_result_t string_stream_format(string_stream_t *this, const fmt_arg_t args[])
{
    cutl_result_t res;
    for (res = CUTL_SUCCESS; res == CUTL_SUCCESS && args->type != FMT_END; ++args)
    {
        switch (args->type)
        {
        case FMT_INT:
            res = string_stream_write_integer(this, args->integer.value, args->integer.spec);
            break;

        case FMT_FLT:
            res = string_stream_write_float(this, args->floating.value, args->floating.spec);
            break;

        case FMT_EXP:
            res = string_stream_write_exponential(this, args->exponential.value, args->exponential.spec);
            break;

        case FMT_S8:
            string_stream_write_s8(this, args->s8);
            break;

        case FMT_STR:
            string_stream_write_cstr(this, args->cstr);
            break;

        case FMT_CUSTOM:
            res = string_stream_write_custom(this, args->custom.length_function, args->custom.write_function,
                                             args->custom.param);
            break;

        default:
            return CUTL_RESULT_INVALID_TYPE_ENUM;
        }
    }
    return res;
}

void string_stream_clear(string_stream_t *this)
{
    this->buffer_pos = 0;
}

size_t string_stream_get_pos(const string_stream_t *this)
{
    return this->buffer_pos;
}

cutl_result_t string_stream_set_pos(string_stream_t *this, const size_t pos)
{
    // Check that we are only doing rollback
    if (pos > this->buffer_pos)
        return CUTL_RESULT_INDEX_OUT_OF_BOUNDS;
    this->buffer_pos = pos;
    return CUTL_SUCCESS;
}