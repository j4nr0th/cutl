#include "../include/cutl/error.h"
#include "../include/cutl/strings.h"

string8_t cutl_result_to_string8(const cutl_result_t result)
{
#define VALUE_STRING(e)                                                                                                \
    case e:                                                                                                            \
        return string8_from_literal(#e)
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
        VALUE_STRING(CUTL_RESULT_INDEX_OUT_OF_BOUNDS);
        VALUE_STRING(CUTL_RESULT_CALLBACK_FAILURE);
        VALUE_STRING(CUTL_RESULT_INVALID_TYPE_ENUM);
        VALUE_STRING(CUTL_RESULT_FILE_IO_FAILURE);
    }
#undef VALUE_STRING
    return string8_from_literal("INVALID");
}

const char *cutl_result_to_string(const cutl_result_t result)
{
    // This cast works because these are all backed by null-terminated literals.
    return (const char *)cutl_result_to_string8(result).data;
}

string8_t cutl_result_message_s8(const cutl_result_t result)
{
    switch (result)
    {
    case CUTL_SUCCESS:
        return string8_from_literal(u8"Success");
    case CUTL_RESULT_FAILURE:
        return string8_from_literal(u8"Failure");
    case CUTL_RESULT_INSUFFICIENT_ALIGNMENT:
        return string8_from_literal(u8"Memory pointer did not have adequate alignment");
    case CUTL_RESULT_INSUFFICIENT_BUFFER:
        return string8_from_literal(u8"Buffer passed to the function was not large enough");
    case CUTL_RESULT_MISMATCHED_ALLOCATOR:
        return string8_from_literal(u8"This allocator did not allocate the memory pointer which was attempted to be "
                                    u8"deallocated or reallocated");
    case CUTL_RESULT_CORRUPTED_POINTER:
        return string8_from_literal(u8"Memory was either already deallocated or the allocator state was corrupted");
    case CUTL_RESULT_DOUBLE_DEALLOCATION:
        return string8_from_literal(u8"Memory was already deallocated");
    case CUTL_RESULT_OUT_OF_MEMORY:
        return string8_from_literal(u8"Out of memory");
    case CUTL_RESULT_INDEX_OUT_OF_BOUNDS:
        return string8_from_literal(u8"Specified index was out of bounds");
    case CUTL_RESULT_CALLBACK_FAILURE:
        return string8_from_literal(u8"Callback passed to the function indicated failure");
    case CUTL_RESULT_INVALID_TYPE_ENUM:
        return string8_from_literal(u8"Invalid value of enum used to indicate a type was passed to the function");
    case CUTL_RESULT_FILE_IO_FAILURE:
        return string8_from_literal(u8"File read or write operation failed");
    }
    return string8_from_literal(u8"Unknown result");
}

const char *cutl_result_message(const cutl_result_t result)
{
    // This cast works because these are all backed by null-terminated literals.
    return (const char *)cutl_result_message_s8(result).data;
}