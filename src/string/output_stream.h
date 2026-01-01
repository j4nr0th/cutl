#pragma once

#include <stdio.h>

#include "../common_defs.h"
#include "../error.h"
#include "format_streams.h"

/**
 * Stream which uses (buffered) writing to a file stream.
 */
typedef struct
{
    FILE *stream;
    size_t buffer_size;
    size_t buffer_pos;
    char8_t buffer[];
} output_stream_t;

/**
 * Create a new output file stream in the allocated memory region.
 *
 * @param stream File to write to with the stream.
 * @param size Size of the buffer to create the file stream in.
 * @param memory Pointer to the allocated memory in which to create the output file stream.
 * @param p_out Pointer which receives the output file stream.
 * @return CUTL_SUCCESS on success, otherwise an error code.
 */
cutl_result_t output_stream_init(FILE *stream, size_t size, unsigned char CUTL_ARRAY_ARG(memory, size),
                                 output_stream_t **p_out);

/**
 * Flush the output stream and ensure all data in the stream's buffer is written to a file.
 *
 * @param this Output stream to flush.
 * @return CUTL_SUCCESS if successful, CUTL_RESULT_FILE_IO_FAILURE if the write or flush operations fail, and
 *         CUTL_RESULT_FAILURE if an unknown failure occurs.
 */
cutl_result_t output_stream_flush(output_stream_t *this);

/**
 * Write a string to an output stream.
 *
 * @param this Output stream to write to.
 * @param str String to write.
 * @return CUTL_SUCCESS on success, otherwise an error code.
 */
cutl_result_t output_stream_write_s8(output_stream_t *this, string8_t str);

/**
 * Write a null-terminated string to an output stream.
 *
 * @param this Output stream to write to.
 * @param str Null-terminated string to write.
 * @return CUTL_SUCCESS on success, otherwise an error code.
 */
cutl_result_t output_stream_write_cstr(output_stream_t *this, const char *str);

/**
 * Write an integer to an output stream.
 *
 * @param this Output stream to write to.
 * @param value Integer to write to the stream.
 * @param spec Specifications for the formatting of the value.
 * @return CUTL_SUCCESS on success, otherwise an error code.
 */
cutl_result_t output_stream_write_integer(output_stream_t *this, intmax_t value, const integer_spec_c8_t *spec);

/**
 * Write a float to an output stream.
 *
 * @param this Output stream to write to.
 * @param value Float to write to the stream.
 * @param spec Specifications for the formatting of the value.
 * @return CUTL_SUCCESS on success, otherwise an error code.
 */
cutl_result_t output_stream_write_float(output_stream_t *this, double value, const float_spec_c8_t *spec);

/**
 * Write an exponential float to an output stream.
 *
 * @param this Output stream to write to.
 * @param value Float to write to the stream.
 * @param spec Specifications for the formatting of the value.
 * @return CUTL_SUCCESS on success, otherwise an error code.
 */
cutl_result_t output_stream_write_exponential(output_stream_t *this, double value, const exponential_spec_c8_t *spec);

/**
 * Write a value with custom formatting to an output stream.
 *
 * @param this Output stream to write to.
 * @param length_function Function used to determine the bytes needed.
 * @param write_function Function used to write to the string.
 * @param param Value passed to the two functions.
 * @return CUTL_SUCCESS on success, otherwise an error code.
 */
cutl_result_t output_stream_write_custom(output_stream_t *this, format_length_function length_function,
                                         format_write_function write_function, void *param);

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
