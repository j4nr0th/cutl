#pragma once

/**
 * Function that returns the size of the buffer needed to format the custom value.
 */
typedef size_t (*format_length_function)(void *param);

/**
 * Function that formats the custom value to the buffer, which was previously sized.
 */
typedef int (*format_write_function)(void *param, size_t size, char8_t CUTL_ARRAY_ARG(buffer, size));
