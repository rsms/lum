#ifndef _LUM_COMMON_H_
#define _LUM_COMMON_H_

// The classic stringifier macro
#define LUM_STR1(str) #str
#define LUM_STR(str) LUM_STR1(str)
#define LUM_MCAT(...) __VA_ARGS__

// Version
#define LUM_VERSION_MAJOR 0
#define LUM_VERSION_MINOR 1
#define LUM_VERSION_BUILD 0
#define LUM_VERSION_STRING \
  LUM_STR(LUM_VERSION_MAJOR) "." \
  LUM_STR(LUM_VERSION_MINOR) "." \
  LUM_STR(LUM_VERSION_BUILD)

// Configuration
#include <lum/common/config.h>

// Defines the host target
#include <lum/common/target.h>

// LUM_FILENAME expands to the current translation unit's filename
#ifdef __BASE_FILE__
  #define LUM_FILENAME __BASE_FILE__
#else
  #define LUM_FILENAME ((strrchr(__FILE__, '/') ?: __FILE__ - 1) + 1)
#endif

// Number of items in a constant array
#define LUM_COUNTOF(a) (sizeof(a) / sizeof(*(a)))

#include <lum/common/assert.h>

// Terminate process with status 70 (EX_SOFTWARE), writing `fmt` with optional
// arguments to stderr.
#define LUM_FATAL(fmt, ...) \
  errx(70, "FATAL: " fmt " at " __FILE__ ":" LUM_STR(__LINE__), ##__VA_ARGS__)

// Print `fmt` with optional arguments to stderr, and abort() process causing
// EXC_CRASH/SIGABRT.
#define LUM_CRASH(fmt, ...) do { \
    fprintf(stderr, "CRASH: " fmt " at " __FILE__ ":" LUM_STR(__LINE__) "\n", \
      ##__VA_ARGS__); \
    abort(); \
  } while (0)

#include <lum/common/attributes.h>

#define LUM_MAX(a,b) \
  ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define LUM_MIN(a,b) \
  ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#include <lum/common/types.h> // .. include <std{io,int,def,bool}>

// libc
#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <err.h>
#if LUM_HOST_OS_POSIX
  #include <unistd.h>
#endif

// common c++ standard library headers
#ifdef __cplusplus
  #include <string>
  #include <map>
  #include <vector>
  #include <list>
  #include <forward_list>
  #include <exception>
  #include <stdexcept>
  #include <utility>
  #include <iostream>
  #include <memory>
  #include <functional>
#endif // __cplusplus

// Atomic operations
#include <lum/common/atomic.h>

// C++ utils
#ifdef __cplusplus
  #include <lum/common/cxxdetail.h>
#endif

#endif // _LUM_COMMON_H_
