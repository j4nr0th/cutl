#include "../../src/string/string_stream.h"
#include "../common/common.h"

int main()
{
    enum
    {
        TEST_BUFFER_SIZE = 1 << 11
    };
    unsigned char *const buffer = malloc(TEST_BUFFER_SIZE);

    string_stream_t *ss;
    cutl_result_t res;

    // Fail when there's not enough memory
    TEST_CUTL_RESULT(res, string_stream_init(1, buffer, &ss), CUTL_RESULT_INSUFFICIENT_BUFFER);
    // Succeed when we give enough memory
    TEST_CUTL_RESULT(res, string_stream_init(TEST_BUFFER_SIZE, buffer, &ss), CUTL_SUCCESS);

    // Fail when we try to write out of bounds
    TEST_CUTL_RESULT(res, string_stream_write_s8(ss, (string8_t){.data = nullptr, .length = TEST_BUFFER_SIZE}),
                     CUTL_RESULT_INSUFFICIENT_BUFFER);

    // Try and compose a formatted string
    TEST_CUTL_RESULT(res,
                     string_stream_format(ss,
                                          (const fmt_arg_t[]){
                                              {.type = FMT_INT, .integer = {.value = 67}},
                                              {.type = FMT_S8, .s8 = string8_from_literal(" on a Merry")},
                                              {.type = FMT_STR, .cstr = " Rizzmass! - "},
                                              {.type = FMT_EXP, .exponential = {.value = 420.69e3}},
                                          }),
                     CUTL_SUCCESS);

    auto const expected_s8 = string8_from_literal("67 on a Merry Rizzmass! - 4.206900E+005");
    auto const real_s8 = string_stream_get_string(ss);

    TEST_ASSERTION(string8_compare(real_s8, expected_s8) == 0,
                   "Composed string was not what was expected (expected \"%*s\", but got \"%*s\".",
                   (int)expected_s8.length, (char *)expected_s8.data, (int)real_s8.length, (char *)real_s8.data);

    free(buffer);
    return 0;
}