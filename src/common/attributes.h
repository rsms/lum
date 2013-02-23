#ifndef LUM_COMMON_ATTRIBUTES_H
#define LUM_COMMON_ATTRIBUTES_H

// Note: This header is supposed to be included by common.h and relies on stuff
// declared in that file.

// Attributes
#ifndef __has_attribute
  #define __has_attribute(x) 0
#endif
#ifndef __has_builtin
  #define __has_builtin(x) 0
#endif

#if __has_attribute(always_inline)
  #define LUM_ALWAYS_INLINE __attribute__((always_inline))
#else
  #define LUM_ALWAYS_INLINE
#endif
#if __has_attribute(unused)
  // Attached to a function, means that the function is meant to be possibly
  // unused. The compiler will not produce a warning for this function.
  #define LUM_UNUSED __attribute__((unused))
#else
  #define LUM_UNUSED
#endif
#if __has_attribute(warn_unused_result)
  #define LUM_WUNUSEDR __attribute__((warn_unused_result))
#else
  #define LUM_WUNUSEDR
#endif
#if __has_attribute(packed)
  #define LUM_PACKED __attribute((packed))
#else
  #define LUM_PACKED
#endif
#if __has_attribute(aligned)
  #define LUM_ALIGNED(bytes) __attribute__((aligned (bytes)))
#else
  #warning "No align attribute available. Things might break"
  #define LUM_ALIGNED
#endif

#if __has_builtin(__builtin_unreachable)
  #define LUM_UNREACHABLE do { \
    assert(!"Declared LUM_UNREACHABLE but was reached"); \
    __builtin_unreachable(); \
  } while(0)
#else
  #define LUM_UNREACHABLE \
    assert(!"Declared LUM_UNREACHABLE but was reached")
#endif

#define LUM_NOT_IMPLEMENTED \
  LUM_FATAL("NOT IMPLEMENTED in %s", __PRETTY_FUNCTION__)

#endif // LUM_COMMON_ATTRIBUTES_H