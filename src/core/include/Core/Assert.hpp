#ifndef CORE_ASSERT_HPP
#define CORE_ASSERT_HPP

#include "Core/core_export.h"
#include <Core/Compiler.hpp>

// Check if we're on windows and have access to __debugbreak().
#if !defined(CORE_DEBUGBREAK)
#if defined(_WIN32) && defined(__has_include)
#if __has_include(<intrin.h>)
#define CORE_DEBUGBREAK 1
#endif
#endif
#endif

#if !defined(CORE_DEBUGBREAK)
#define CORE_DEBUGBREAK 0
#endif

// Check if we're on a compiler that supports __builtin_trap().
#if !defined(CORE_BUILTIN_TRAP)
#if defined(__GNUC__) || defined(__clang__)
#define CORE_BUILTIN_TRAP 1
#else
#define CORE_BUILTIN_TRAP 0
#endif
#endif

#if !defined(CORE_BUILTIN_TRAP)
#define CORE_BUILTIN_TRAP 0
#endif

#if CORE_DEBUGBREAK
#include <intrin.h> // Provides access to the __debugbreak() function on windows.
#define CORE_ASSERT_TRAP() ::__debugbreak()
#elif CORE_BUILTIN_TRAP
#define CORE_ASSERT_TRAP() __builtin_trap()
#else
#define CORE_ASSERT_TRAP() ::std::abort()
#endif

namespace core {

CORE_EXPORT void assertion(const char* fileName, int line, const char* funcName, const char* message);

} // namespace core

#if defined(NDEBUG) && NDEBUG
#define CORE_ASSERT(...)
#else
#define CORE_ASSERT(...)                                                                                               \
    do {                                                                                                               \
        if (CORE_UNLIKELY(!(__VA_ARGS__))) {                                                                           \
            ::core::assertion(__FILE__, __LINE__, __func__, #__VA_ARGS__);                                             \
            CORE_ASSERT_TRAP();                                                                                        \
        }                                                                                                              \
    } while (false)
#endif

#endif // CORE_ASSERT_HPP
