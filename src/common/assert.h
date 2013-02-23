#ifndef LUM_COMMON_ASSERT_H
#define LUM_COMMON_ASSERT_H

// Note: This header is supposed to be included by common.h and relies on stuff
// declared in that file.

#if !NDEBUG
  #define LumAssert(x) assert(x)
  // Important: The following macros _always_ evaluate `x`, even when compiling
  // w/ assertions disabled. The purpose is to allow wrapping like so:
  //    LumAssertNil(somelibcfunc(dostuff))
  //
  #define LumAssertTrue(x) LumAssert((x) == true)
  #define LumAssertFalse(x) LumAssert((x) == false)
  #define LumAssertNil(x) LumAssert((x) == 0)
  #define LumAssertNotNil(x) LumAssert((x) != 0)
#else
  #define LumAssert(x) ((void)0)
  #define LumAssertTrue(x) (x)
  #define LumAssertFalse(x) (x)
  #define LumAssertNil(x) (x)
  #define LumAssertNotNil(x) (x)
#endif

#endif // LUM_COMMON_ASSERT_H
