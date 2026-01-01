#pragma once

/**
 * Values used to indicate the reason for a function's return.
 *
 * To get a more detailed explanation of what each of these values means, call ``cutl_result_message``.
 */
enum cutl_result_t
{
    /**
     * Success
     */
    CUTL_SUCCESS,
    /**
     * Failure
     */
    CUTL_RESULT_FAILURE,
    /**
     * Out of memory
     */
    CUTL_RESULT_OUT_OF_MEMORY,
    /**
     * Memory pointer did not have adequate alignment
     */
    CUTL_RESULT_INSUFFICIENT_ALIGNMENT,
    /**
     *  Buffer passed to the function was not large enough
     */
    CUTL_RESULT_INSUFFICIENT_BUFFER,
    /**
     *   This allocator did not allocate the memory pointer which was attempted to be deallocated or reallocated
     */
    CUTL_RESULT_MISMATCHED_ALLOCATOR,
    /**
     *  Memory was either already deallocated or the allocator state was corrupted
     */
    CUTL_RESULT_CORRUPTED_POINTER,
    /**
     * Memory was already deallocated
     */
    CUTL_RESULT_DOUBLE_DEALLOCATION,
    /**
     *  Specified index was out of bounds
     */
    CUTL_RESULT_INDEX_OUT_OF_BOUNDS,
    /**
     *  Callback passed to the function indicated failure
     */
    CUTL_RESULT_CALLBACK_FAILURE,
    /**
     * Invalid value of enum used to indicate a type was passed to the function
     */
    CUTL_RESULT_INVALID_TYPE_ENUM,
    /**
     *  File read or write operation failed
     */
    CUTL_RESULT_FILE_IO_FAILURE,
};

typedef enum cutl_result_t cutl_result_t;

/**
 * Get the string representation of a ``cutl_result_t`` value.
 *
 * @param result Value to convert to a string.
 * @return Statically allocated null-terminated string representation of ``result``.
 */
const char *cutl_result_to_string(cutl_result_t result);

/**
 * Get the meaning of the ``cutl_result_t`` value.
 *
 * @param result Value to get the meaning of.
 * @return Statically allocated null-terminated string representation of the ``result``.
 */
const char *cutl_result_message(cutl_result_t result);
