#ifndef __JIVAN_MACRO_H__
#define __JIVAN_MACRO_H__

#include <string.h>
#include <assert.h>
#include "log.h"
#include "util.h"

#if defined __GNUC__ || defined __llvm__
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
#define JIVAN_LIKELY(x) __builtin_expect(!!(x), 1)
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立
#define JIVAN_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define JIVAN_LIKELY(x) (x)
#define JIVAN_UNLIKELY(x) (x)
#endif

/// 断言宏封装
#define JIVAN_ASSERT(x)                                                                \
    if (JIVAN_UNLIKELY(!(x))) {                                                        \
        JIVAN_LOG_ERROR(JIVAN_LOG_ROOT()) << "ASSERTION: " #x                          \
                                          << "\nbacktrace:\n"                          \
                                          << jivan::BacktraceToString(100, 2, "    "); \
        assert(x);                                                                     \
    }

/// 断言宏封装
#define JIVAN_ASSERT2(x, w)                                                            \
    if (JIVAN_UNLIKELY(!(x))) {                                                        \
        JIVAN_LOG_ERROR(JIVAN_LOG_ROOT()) << "ASSERTION: " #x                          \
                                          << "\n"                                      \
                                          << w                                         \
                                          << "\nbacktrace:\n"                          \
                                          << jivan::BacktraceToString(100, 2, "    "); \
        assert(x);                                                                     \
    }

#endif
