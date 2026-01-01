#pragma once

#include "../common_defs.h"
#include "../error.h"
#include "format_streams.h"
#include <stdio.h>

/**
 * Stream which writes to an internal buffer.
 */
typedef struct string_stream_t string_stream_t;

/**
 * Create a new string stream in the allocated memory region.
 *
 * @param size Size of the buffer to create the string stream in.
 * @param memory Pointer to the allocated memory in which to create the string stream.
 * @param p_out Pointer which receives the string stream.
 * @return CUTL_SUCCESS on success, otherwise an error code.
 */
cutl_result_t string_stream_init(size_t size, unsigned char CUTL_ARRAY_ARG(memory, size), string_stream_t **p_out);

/**
 * Write a string to a string stream.
 *
 * @param this String stream to write to.
 * @param str String to write.
 * @return CUTL_SUCCESS on success, otherwise an error code.
 */
cutl_result_t string_stream_write_s8(string_stream_t *this, string8_t str);

/**
 * Write a null-terminated string to a string stream.
 *
 * @param this String stream to write to.
 * @param str Null-terminated string to write.
 * @return CUTL_SUCCESS on success, otherwise an error code.
 */
cutl_result_t string_stream_write_cstr(string_stream_t *this, const char *str);

/**
 * Write an integer to a string stream.
 *
 * @param this String stream to write to.
 * @param value Integer to write to the stream.
 * @param spec Specifications for the formatting of the value.
 * @return CUTL_SUCCESS on success, otherwise an error code.
 */
cutl_result_t string_stream_write_integer(string_stream_t *this, intmax_t value, const integer_spec_c8_t *spec);

/**
 * Write a float to a string stream.
 *
 * @param this String stream to write to.
 * @param value Float to write to the stream.
 * @param spec Specifications for the formatting of the value.
 * @return CUTL_SUCCESS on success, otherwise an error code.
 */
cutl_result_t string_stream_write_float(string_stream_t *this, double value, const float_spec_c8_t *spec);

/**
 * Write an exponential float to a string stream.
 *
 * @param this String stream to write to.
 * @param value Float to write to the stream.
 * @param spec Specifications for the formatting of the value.
 * @return CUTL_SUCCESS on success, otherwise an error code.
 */
cutl_result_t string_stream_write_exponential(string_stream_t *this, double value, const exponential_spec_c8_t *spec);

/**
 * Write a value with custom formatting to a string stream.
 *
 * @param this String stream to write to.
 * @param length_function Function used to determine the bytes needed.
 * @param write_function Function used to write to the string.
 * @param param Value passed to the two functions.
 * @return CUTL_SUCCESS on success, otherwise an error code.
 */
cutl_result_t string_stream_write_custom(string_stream_t *this, format_length_function length_function,
                                         format_write_function write_function, void *param);

/**
 * Return the string currently in the string stream.
 *
 * @param this String stream to get the current string from.
 * @return String currently written to the stream thus far.
 */
string8_t string_stream_get_string(string_stream_t *this);

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

/**
 * Clear the current state of the string stream, resetting it to an empty string.
 *
 * @param this String stream to clear.
 */
void string_stream_clear(string_stream_t *this);

/**
 * Get the position (state) of the string stream, which can be used to undo future write operations to the stream.
 *
 * @param this String stream to get the current position.
 * @return Position of the current string stream.
 */
size_t string_stream_get_pos(const string_stream_t *this);

/**
 * Roll back the string stream by setting its position.
 *
 * Calling ``string_stream_set_pos(this, 0)`` is equivalent to calling ``string_stream_clear(this)``. This function
 * can only be used to roll the state back, not move forward.
 *
 * @param this String stream to set the position of.
 * @param pos Position of the string stream.
 * @return CUTL_SUCCESS if successful, CUTL_RESULT_INDEX_OUT_OF_BOUNDS if the current position is less than the given
 * value.
 */
cutl_result_t string_stream_set_pos(string_stream_t *this, size_t pos);
