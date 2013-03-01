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
  #define print(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#endif

#endif  // _LUM_TEST_H_
