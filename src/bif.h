#ifndef _LUM_BIF_H_
#define _LUM_BIF_H_

#include <lum/common.h>
#include <math.h>

namespace lum {

struct Cell;
struct Env;
struct Str;

struct Bif {
  typedef Cell* (*Impl)(Env*,Cell*);
  constexpr Bif(Impl i, size_t pc, bool av)
      : apply(i)
      , _param_count(pc)
      , accepts_varargs(av)
      , name(0) {}

  size_t param_count() const { return _param_count; }

  Impl apply; // Cell* apply(Env* env, Cell* args) const
  size_t _param_count;
  bool accepts_varargs;
  const Str* name;
};

std::ostream& operator<< (std::ostream& os, const Bif const*);

#define LUM_BIF_APPLY(Name, ...) \
  extern const Bif const* kBif_##Name;
#include <lum/bif-defs.h>
#undef LUM_BIF_APPLY

} // namespace lum
#endif // _LUM_BIF_H_
