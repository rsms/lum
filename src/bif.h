#ifndef _LUM_BIF_H_
#define _LUM_BIF_H_

#include <lum/common.h>
#include <math.h>

namespace lum {

struct Cell;
struct Env;

struct Bif {
  typedef Cell* (*Impl)(Env*,Cell*);
  constexpr Bif(Impl impl, size_t param_count, bool accepts_varargs)
      : impl(impl)
      , param_count(param_count)
      , accepts_varargs(accepts_varargs) {}
  Impl impl;
  size_t param_count;
  bool accepts_varargs;
};

typedef void (*NumOpI)(int64_t& v, int64_t operand);
typedef void (*NumOpF)(double& v, double operand);

inline void BIF_sumI(int64_t& v, int64_t operand) { v += operand; }
inline void BIF_sumF(double& v, double operand)   { v += operand; }

inline void BIF_subI(int64_t& v, int64_t operand) { v -= operand; }
inline void BIF_subF(double& v, double operand)   { v -= operand; }

inline void BIF_mulI(int64_t& v, int64_t operand) { v *= operand; }
inline void BIF_mulF(double& v, double operand)   { v *= operand; }

inline void BIF_divI(int64_t& v, int64_t operand) { v /= operand; }
inline void BIF_divF(double& v, double operand)   { v /= operand; }

inline void BIF_remI(int64_t& v, int64_t operand) { v %= operand; }
inline void BIF_remF(double& v, double operand)   { v = fmod(v, operand); }

template <NumOpI IOP, NumOpF FOP>
Cell* BIF_numop(Env* env, Cell* args);

extern template Cell* BIF_numop<BIF_sumI, BIF_sumF>(Env* env, Cell* args);
extern template Cell* BIF_numop<BIF_subI, BIF_subF>(Env* env, Cell* args);
extern template Cell* BIF_numop<BIF_mulI, BIF_mulF>(Env* env, Cell* args);
extern template Cell* BIF_numop<BIF_divI, BIF_divF>(Env* env, Cell* args);
extern template Cell* BIF_numop<BIF_remI, BIF_remF>(Env* env, Cell* args);

Cell* BIF_eq(Env* env, Cell* args);
Cell* BIF_cons(Env* env, Cell* args);
Cell* BIF_in_ns(Env* env, Cell* arg);
Cell* BIF_def(Env* env, Cell* arg);
Cell* BIF_fn(Env* env, Cell* args);

} // namespace lum
#endif // _LUM_BIF_H_
