#ifndef _LUM_COMMON_ATOMIC_H_
#define _LUM_COMMON_ATOMIC_H_
/*

T LumAtomicSwap(T *ptr, T value)
  Atomically swap integers or pointers in memory.
  E.g: int old_value = LumAtomicSwap(&value, new_value);

bool LumAtomicBoolCmpSwap(T* ptr, T oldval, T newval)
  If the current value of *ptr is oldval, then write newval into *ptr.
  Returns true if the operation is successful and newval was written.

T LumAtomicCmpSwap(T* ptr, T oldval, T newval)
  If the current value of *ptr is oldval, then write newval into *ptr.
  Returns the contents of *ptr before the operation.

void LumAtomicAdd32(int32_t* operand, int32_t delta)
  Increment a 32-bit integer `operand` by `delta`. There's no return value.

T LumAtomicSubAndFetch(T* operand, T delta)
  Subtract `delta` from `operand` and return the resulting value of `operand`

-----------------------------------------------------------------------------*/

// T LumAtomicSwap(T *ptr, T value)
#if LUM_WITHOUT_SMP
  #define LumAtomicSwap(ptr, value)  \
    ({ __typeof__ (value) oldval = *(ptr); \
       *(ptr) = (value); \
       oldval; })
#elif defined(__clang__)
  // This is more efficient than the below fallback
  #define LumAtomicSwap __sync_swap
#elif defined(__GNUC__) && (__GNUC__ >= 4)
  static inline void* LUM_UNUSED _LumAtomicSwap(void* volatile* ptr, void* value) {
    void* oldval;
    do {
      oldval = *ptr;
    } while (__sync_val_compare_and_swap(ptr, oldval, value) != oldval);
    return oldval;
  }
  #define LumAtomicSwap(ptr, value) \
    _LumAtomicSwap((void* volatile*)(ptr), (void*)(value))
#else
  #error "Unsupported compiler: Missing support for atomic operations"
#endif


// bool LumAtomicBoolCmpSwap(T* ptr, T oldval, T newval)
#if LUM_WITHOUT_SMP
  #define LumAtomicBoolCmpSwap(ptr, oldval, newval)  \
    (*(ptr) == (oldval) && (*(ptr) = (newval)))
#elif defined(__GNUC__) && (__GNUC__ >= 4)
  #define LumAtomicBoolCmpSwap(ptr, oldval, newval) \
    __sync_bool_compare_and_swap((ptr), (oldval), (newval))
#else
  #error "Unsupported compiler: Missing support for atomic operations"
#endif


// T LumAtomicCmpSwap(T* ptr, T oldval, T newval)
#if LUM_WITHOUT_SMP
  #define LumAtomicCmpSwap(ptr, oldval, newval)  \
    ({ __typeof__(oldval) prevv = *(ptr); \
       if (*(ptr) == (oldval)) { *(ptr) = (newval); } \
       prevv; })
#elif defined(__GNUC__) && (__GNUC__ >= 4)
  #define LumAtomicCmpSwap(ptr, oldval, newval) \
    __sync_val_compare_and_swap((ptr), (oldval), (newval))
#else
  #error "Unsupported compiler: Missing support for atomic operations"
#endif


// void LumAtomicAdd32(T* operand, T delta)
#if LUM_WITHOUT_SMP
  #define LumAtomicAdd32(operand, delta) (*(operand) += (delta))
#elif LUM_TARGET_ARCH_X64 || LUM_TARGET_ARCH_X86
  inline static void LUM_UNUSED LumAtomicAdd32(int32_t* operand, int32_t delta) {
    // From http://www.memoryhole.net/kyle/2007/05/atomic_incrementing.html
    __asm__ __volatile__ (
      "lock xaddl %1, %0\n" // add delta to operand
      : // no output
      : "m" (*operand), "r" (delta)
    );
  }
#elif defined(__clang__) || (defined(__GNUC__) && (__GNUC__ >= 4))
  #define LumAtomicAdd32 __sync_add_and_fetch
#else
  #error "Unsupported compiler: Missing support for atomic operations"
#endif


// T LumAtomicSubAndFetch(T* operand, T delta)
#if LUM_WITHOUT_SMP
  #define LumAtomicSubAndFetch(operand, delta) (*(operand) -= (delta))
#elif defined(__clang__) || (defined(__GNUC__) && (__GNUC__ >= 4))
  #define LumAtomicSubAndFetch __sync_sub_and_fetch
#else
  #error "Unsupported compiler: Missing support for atomic operations"
#endif


// Spinlock
#ifdef __cplusplus
namespace lum {
typedef volatile int32_t Spinlock;

#define LUM_SPINLOCK_INIT 0  // usage: Spinlock lock = LUM_SPINLOCK_INIT;

bool spinlock_try_lock(Spinlock& lock);
void spinlock_lock(Spinlock& lock);
void spinlock_unlock(Spinlock& lock);

bool spinlock_try_lock(Spinlock* lock);
void spinlock_lock(Spinlock* lock);
void spinlock_unlock(Spinlock* lock);

struct ScopedSpinlock {
  ScopedSpinlock(Spinlock& lock) : _lock(lock) { spinlock_lock(_lock); }
  ~ScopedSpinlock() { spinlock_unlock(_lock); }
private:
  Spinlock& _lock;
};

inline bool LUM_UNUSED spinlock_try_lock(Spinlock& lock) {
  return LumAtomicBoolCmpSwap(&lock, (int32_t)0, (int32_t)1); }
inline void LUM_UNUSED spinlock_lock(Spinlock& lock) {
  while (!spinlock_try_lock(lock)); }
inline void LUM_UNUSED spinlock_unlock(Spinlock& lock) {
  lock = 0; }

inline bool LUM_UNUSED spinlock_try_lock(Spinlock* lock) {
  return LumAtomicBoolCmpSwap(lock, (int32_t)0, (int32_t)1); }
inline void LUM_UNUSED spinlock_lock(Spinlock* lock) {
  while (!spinlock_try_lock(lock)); }
inline void LUM_UNUSED spinlock_unlock(Spinlock* lock) {
  *lock = 0; }

} // namespace lum
#endif // __cplusplus


#endif // _LUM_COMMON_ATOMIC_H_
