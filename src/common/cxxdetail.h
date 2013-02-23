#ifndef LUM_COMMON_CXXDETAIL_H
#define LUM_COMMON_CXXDETAIL_H

// Note: This header is supposed to be included by common.h and relies on stuff
// declared in that file.

#include <type_traits>

#define LUM_CXX_DISALLOW_COPY(T) \
  T(const T& other) = delete; \
  T& operator=(const T& other) = delete

namespace lum { namespace detail {

// Support for C++11 enum class comparison
template<typename T> using _UT = typename std::underlying_type<T>::type;
template<typename T> constexpr _UT<T> underlying_type(T t) { return _UT<T>(t); }
template<typename T> struct TruthValue {
  T t;
  constexpr TruthValue(T t): t(t) { }
  constexpr operator T() const { return t; }
  constexpr explicit operator bool() const { return underlying_type(t); }
};

}} // namespace lum::detail

// This would allow _any_ enum class to be bitwise operated
//template <typename T>
//constexpr lum::detail::TruthValue<T>
//operator&(T lhs, T rhs) {
//  return T(lum::detail::underlying_type(lhs) & lum::detail::underlying_type(rhs));
//}

// This allows explicit enum classes to be bitwise operated
#define LUM_CXX_DECL_ENUM_BITWISE_OPS(T) \
  namespace { \
    LUM_UNUSED \
    constexpr lum::detail::TruthValue<T> \
    operator&(T lhs, T rhs) { \
      return T(lum::detail::underlying_type(lhs) \
             & lum::detail::underlying_type(rhs)); \
    } \
  }

#endif // LUM_COMMON_CXXDETAIL_H
