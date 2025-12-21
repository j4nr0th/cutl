//
// Created by jan on 2025-12-20.
//

#include "error.h"

const char *cutl_result_to_string(const cutl_result_t result)
{
#define VALUE_STRING(e)                                                                                                \
    case e:                                                                                                            \
        return #e
    switch (result)
    {
        VALUE_STRING(CUTL_SUCCESS);
        VALUE_STRING(CUTL_RESULT_FAILURE);
        VALUE_STRING(CUTL_RESULT_INSUFFICIENT_ALIGNMENT);
        VALUE_STRING(CUTL_RESULT_INSUFFICIENT_BUFFER);
        VALUE_STRING(CUTL_RESULT_MISMATCHED_ALLOCATOR);
        VALUE_STRING(CUTL_RESULT_CORRUPTED_POINTER);
        VALUE_STRING(CUTL_RESULT_DOUBLE_DEALLOCATION);
        VALUE_STRING(CUTL_RESULT_OUT_OF_MEMORY);
    }
#undef VALUE_STRING
    return "INVALID";
}
const char *cutl_result_message(const cutl_result_t result)
{
    switch (result)
    {
    case CUTL_SUCCESS:
        return "Success";
    case CUTL_RESULT_FAILURE:
        return "Failure";
    case CUTL_RESULT_INSUFFICIENT_ALIGNMENT:
        return "Memory pointer did not have adequate alignment";
    case CUTL_RESULT_INSUFFICIENT_BUFFER:
        return "Buffer passed to the function was not large enough";
    case CUTL_RESULT_MISMATCHED_ALLOCATOR:
        return "Memory pointer which was attempted to be deallocated or reallocated was not allocated by this "
               "allocator";
    case CUTL_RESULT_CORRUPTED_POINTER:
        return "Memory was either already deallocated or the allocator state was corrupted";
    case CUTL_RESULT_DOUBLE_DEALLOCATION:
        return "Memory was already deallocated";
    case CUTL_RESULT_OUT_OF_MEMORY:
        return "Out of memory";
    }
    return "Unknown result";
}