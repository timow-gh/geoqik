#ifndef CORE_COMPILER_HPP
#define CORE_COMPILER_HPP

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#ifndef __has_feature
#define __has_feature(x) 0
#endif

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#if __has_attribute(noreturn)
#define CORE_NORETURN __attribute__((noreturn))
#else
#define CORE_NORETURN
#endif

#if __has_attribute(packed)
#define CORE_PACKED __attribute__((packed))
#else
#define CORE_PACKED
#endif

#if __has_builtin(__builtin_expect)
#define CORE_LIKELY(exp) (__builtin_expect(!!(exp), true))
#define CORE_UNLIKELY(exp) (__builtin_expect(!!(exp), false))
#else
#define CORE_LIKELY(exp) (!!(exp))
#define CORE_UNLIKELY(exp) (!!(exp))
#endif

#if __has_builtin(__builtin_prefetch)
#define CORE_PREFETCH(exp) (__builtin_prefetch(exp))
#else
#define CORE_PREFETCH(exp)
#endif

#if __has_builtin(__builtin_assume)
#define CORE_ASSUME(exp) (__builtin_assume(exp))
#else
#define UTILS_ASSUME(exp)
#endif

#if __has_attribute(always_inline)
#define CORE_ALWAYS_INLINE __attribute__((always_inline))
#else
#define CORE_ALWAYS_INLINE
#endif

#if __has_attribute(noinline)
#define CORE_NOINLINE __attribute__((noinline))
#else
#define CORE_NOINLINE
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1900
#define CORE_RESTRICT __restrict
#elif (defined(__clang__) || defined(__GNUC__))
#define CORE_RESTRICT __restrict__
#else
#define CORE_RESTRICT
#endif

#endif // CORE_COMPILER_HPP
