#ifndef LUM_COMMON_CONFIG_H
#define LUM_COMMON_CONFIG_H

// Enable debugging stuff, like assertions
#ifndef LUM_DEBUG
  #define LUM_DEBUG 0
#elif LUM_DEBUG && defined(NDEBUG)
  #undef NDEBUG
#endif

// Disable (symmetical) multiprocessing?
//#define LUM_WITHOUT_SMP 1

#endif // LUM_COMMON_CONFIG_H
