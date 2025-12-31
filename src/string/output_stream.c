#include "output_stream.h"

#include "format_defaults.h"
#include <errno.h>

cutl_result_t output_stream_init(FILE *const stream, const size_t size, unsigned char CUTL_ARRAY_ARG(memory, size),
                                 output_stream_t **const p_out)
{
    if (size < sizeof(output_stream_t))
        return CUTL_RESULT_INSUFFICIENT_BUFFER;

    auto const this = (output_stream_t *)memory;
    this->stream = stream;
    this->buffer_size = size - sizeof(output_stream_t);
    this->buffer_pos = 0;

    *p_out = this;
    return CUTL_SUCCESS;
}

void output_stream_flush(output_stream_t *this)
{
    if (this->buffer_pos == 0)
    {
        return;
    }

    auto const bytes_written = fwrite(this->buffer, sizeof(*this->buffer), this->buffer_pos, this->stream);
    (void)bytes_written;
    CUTL_ASSERT(bytes_written == this->buffer_pos, "Could not write all bytes to stream: %s", strerror(errno));
    fflush(this->stream);
    this->buffer_pos = 0;
}

static void output_stream_write_u8(output_stream_t *const this, size_t size,
                                   const unsigned char CUTL_ARRAY_ARG(bytes, restrict static size))
{
    auto const remaining_space = this->buffer_size - this->buffer_pos;
    if (size > remaining_space)
    {
        // Do as much as possible
        memcpy(this->buffer + this->buffer_pos, bytes, remaining_space);
        size -= remaining_space;
        bytes += remaining_space;
        output_stream_flush(this);
        // The remainder may still be too large
        if (size >= this->buffer_size)
        {
            // Just write the bytes directly, we cannot hold them in the buffer
            auto const bytes_written = fwrite(bytes, sizeof(*bytes), size, this->stream);
            (void)bytes_written;
            CUTL_ASSERT(bytes_written == size, "Could not write all bytes to stream: %s", strerror(errno));
            return;
        }
    }

    memcpy(this->buffer + this->buffer_pos, bytes, size);
    this->buffer_pos += size;
}

void output_stream_write_s8(output_stream_t *this, const string8_t str)
{
    output_stream_write_u8(this, str.length, str.data);
}

void output_stream_write_cstr(output_stream_t *this, const char *str)
{
    for (;;)
    {
        // Try filling up the remaining space
        size_t i;
        auto const remaining_space = this->buffer_size - this->buffer_pos;
        for (i = 0; str[i] && i < remaining_space; ++i)
        {
            this->buffer[this->buffer_pos++] = str[i];
        }
        // Did the loop finish because we reached the end of the string?
        if (str[i] == 0)
        {
            return;
        }
        str += remaining_space;
        // Nope, we still have ways to go
        output_stream_flush(this);
    }
}

static string8_t output_stream_get_output_string(output_stream_t *this, const size_t needed_size)
{
    CUTL_ASSERT(needed_size <= this->buffer_size, "Buffer too small for output string.");
    auto const remaining_space = this->buffer_size - this->buffer_pos;
    if (remaining_space < needed_size)
        output_stream_flush(this);
    const string8_t output = {.data = this->buffer + this->buffer_pos, .length = needed_size};
    // Already adjust the buffer position
    this->buffer_pos += needed_size;
    return output;
}

cutl_result_t output_stream_write_integer(output_stream_t *this, const intmax_t value, const integer_spec_c8_t *spec)
{
    if (!spec)
        spec = &DEFAULT_INT_SPECS;

    auto const required_memory = format8_integer_length(value, *spec);
    if (required_memory > this->buffer_size)
    {
        // We could never fit this
        return CUTL_RESULT_INSUFFICIENT_BUFFER;
    }

    auto const output = output_stream_get_output_string(this, required_memory);
    auto const res = format8_integer(value, output, *spec);
    if (res != CUTL_SUCCESS)
    {
        // Roll back the buffer
        this->buffer_pos -= output.length;
    }
    return res;
}

cutl_result_t output_stream_write_float(output_stream_t *const this, const double value, const float_spec_c8_t *spec)
{
    if (!spec)
        spec = &DEFAULT_FLT_SPECS;

    auto const required_memory = format8_float_length(value, *spec);
    if (required_memory > this->buffer_size)
        return CUTL_RESULT_INSUFFICIENT_BUFFER;

    auto const output = output_stream_get_output_string(this, required_memory);
    auto const res = format8_float(value, output, *spec);
    if (res != CUTL_SUCCESS)
    {
        this->buffer_pos -= output.length;
    }
    return res;
}

cutl_result_t output_stream_write_exponential(output_stream_t *const this, const double value,
                                              const exponential_spec_c8_t *spec)
{
    if (!spec)
        spec = &DEFAULT_EXP_SPECS;

    auto const required_memory = format8_exponential_length(value, *spec);
    if (required_memory > this->buffer_size)
        return CUTL_RESULT_INSUFFICIENT_BUFFER;

    auto const output = output_stream_get_output_string(this, required_memory);
    auto const res = format8_exponential(value, output, *spec);
    if (res != CUTL_SUCCESS)
    {
        this->buffer_pos -= output.length;
    }
    return res;
}

cutl_result_t output_stream_write_custom(output_stream_t *this, const format_length_function length_function,
                                         const format_write_function write_function, void *param)
{
    auto const required_memory = length_function(param);
    if (required_memory > this->buffer_size)
        return CUTL_RESULT_INSUFFICIENT_BUFFER;

    auto const output = output_stream_get_output_string(this, required_memory);
    auto const res = write_function(param, output.length, output.data);
    if (res)
    {
        this->buffer_pos -= output.length;
        return CUTL_RESULT_CALLBACK_FAILURE;
    }
    return CUTL_SUCCESS;
}

cutl_result_t output_stream_format(output_stream_t *this, const fmt_arg_t args[])
{
    cutl_result_t res;
    for (res = CUTL_SUCCESS; res == CUTL_SUCCESS && args->type != FMT_NONE; ++args)
    {
        switch (args->type)
        {
        case FMT_INT:
            res = output_stream_write_integer(this, args->integer.value, args->integer.spec);
            break;

        case FMT_FLT:
            res = output_stream_write_float(this, args->floating.value, args->floating.spec);
            break;

        case FMT_EXP:
            res = output_stream_write_exponential(this, args->exponential.value, args->exponential.spec);
            break;

        case FMT_S8:
            output_stream_write_s8(this, args->s8);
            break;

        case FMT_STR:
            output_stream_write_cstr(this, args->cstr);
            break;

        case FMT_CUSTOM:
            res = output_stream_write_custom(this, args->custom.length_function, args->custom.write_function,
                                             args->custom.param);
            break;

        default:
            return CUTL_RESULT_INVALID_TYPE_ENUM;
        }
    }
    return res;
}
