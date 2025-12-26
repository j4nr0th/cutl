#pragma once

typedef enum
{
    CUTL_SUCCESS,
    CUTL_RESULT_FAILURE,
    CUTL_RESULT_OUT_OF_MEMORY,
    CUTL_RESULT_INSUFFICIENT_ALIGNMENT,
    CUTL_RESULT_INSUFFICIENT_BUFFER,
    CUTL_RESULT_MISMATCHED_ALLOCATOR,
    CUTL_RESULT_CORRUPTED_POINTER,
    CUTL_RESULT_DOUBLE_DEALLOCATION,
    CUTL_RESULT_INDEX_OUT_OF_BOUNDS,
} cutl_result_t;

const char *cutl_result_to_string(cutl_result_t result);

const char *cutl_result_message(cutl_result_t result);
