// Helpers for tests
#ifndef _LUM_TEST_H_
#define _LUM_TEST_H_

#include <lum/common.h>

#if !LUM_DEBUG
  #warning "Running test in release mode"
#endif

#if LUM_TEST_SUIT_RUNNING
  #define print(...) ((void)0)
#else
  #define print(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#endif

// ------------------------------
#ifdef __cplusplus
#include <iostream>

namespace lum {

template <typename A, typename B>
inline void _assert_eq(
    A value1, const char* name1,
    B value2, const char* name2,
    const char* source_name, int source_line)
{
  if (!(value1 == value2)) {
    std::cerr << "\nAssertion failure at "
              << source_name << ":" << source_line << "\n"
              << "  " << name1 << " == " << name2 << "\n"
              << "  " << value1 << " == " << value2 << "\n"
              ;
    abort();
  }
}

template <typename A, typename B>
inline void _assert_not_eq(
    A value1, const char* name1,
    B value2, const char* name2,
    const char* source_name, int source_line)
{
  if (!(value1 != value2)) {
    std::cerr << "\nAssertion failure at "
              << source_name << ":" << source_line << "\n"
              << "  " << name1 << " != " << name2 << "\n"
              << "  " << value1 << " != " << value2 << "\n"
              ;
    abort();
  }
}

inline void _assert_eq_cstr(
    const char* cstr1, const char* name1,
    const char* cstr2, const char* name2,
    const char* source_name, int source_line)
{
  if (std::strcmp(cstr1, cstr2) != 0) {
    std::cerr << "\nAssertion failure at "
              << source_name << ":" << source_line << "\n"
              << "  " << name1 << " == " << name2 << "\n"
              << "  \"" << cstr1 << "\" == \"" << cstr2 << "\"\n"
              ;
    abort();
  }
}

#define assert_eq(a, b) \
  ::lum::_assert_eq((a), #a, (b), #b, LUM_FILENAME, __LINE__)

#define assert_not_eq(a, b) \
  ::lum::_assert_not_eq((a), #a, (b), #b, LUM_FILENAME, __LINE__)

#define assert_eq_cstr(cstr, b) \
  ::lum::_assert_eq_cstr((cstr), #cstr, (b), #b, LUM_FILENAME, __LINE__)

#define assert_true(a) \
  ::lum::_assert_eq((a), #a, true, "true", LUM_FILENAME, __LINE__)

#define assert_false(a) \
  ::lum::_assert_eq((a), #a, false, "false", LUM_FILENAME, __LINE__)

#define assert_null(a) \
  ::lum::_assert_eq((a), #a, (void*)NULL, "NULL", LUM_FILENAME, __LINE__)

} // namespace lum
#endif // __cplusplus


#endif  // _LUM_TEST_H_
