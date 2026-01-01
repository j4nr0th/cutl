#pragma once

// GCC definitions for parts of other macros
#ifdef __GNUC__
#    define CUTL_ARRAY_ARG(array, attrib) array[attrib]
#    define CUTL_DEBUG_BREAK __builtin_trap()
#    define CUTL_ASSUME(x) __attribute__((assume(!(!(x)))))
#endif

// Fallback definitions
#ifndef CUTL_ARRAY_ARG
#    define CUTL_ARRAY_ARG(array, attrib) array[]
#endif

#ifndef CUTL_DEBUG_BREAK
#    define CUTL_DEBUG_BREAK (void)0
#endif
#ifndef CUTL_ASSUME
#    define CUTL_ASSUME(x) (void)(0)
#endif

// Assert used to either validate assumptions at runtime or otherwise specify them at compile time.
#ifdef CUTL_ENABLE_ASSERTS
#    include <stdio.h>
#    include <stdlib.h>
#    define CUTL_ASSERT(x, fmt, ...)                                                                                   \
        (!(x) ?                                                                                                        \
                                                                                                                       \
              (fprintf(stderr, "%s:%d - %s - Assertion failed %s: " fmt "\n", __FILE__, __LINE__, __func__,            \
                       #x __VA_OPT__(, ) __VA_ARGS__),                                                                 \
               CUTL_DEBUG_BREAK, exit(EXIT_FAILURE))                                                                   \
              : (void)0)
#else
#    define CUTL_ASSERT(x, fmt, ...) CUTL_ASSUME(x)
#endif
