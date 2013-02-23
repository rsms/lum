#ifndef _LUM_FN_H_
#define _LUM_FN_H_

#include <lum/common.h>
#include <lum/cell.h>

namespace lum {

struct Fn {
  struct Param {
    const Str* name;
    Type type;
    bool is_variable; // if ... args should be consumed rather than just one
  };

  static Fn* create(Env* env, Cell* params_and_bodies);
  static void free(Fn*);

  bool compile(Env*);
  Cell* apply(Env* env, Cell* args);

  uint32_t param_count() const { return _param_count; }
  const Param& param(uint32_t i) const { return _params[i]; }
  const Cell* body() const { return _body; }

  Cell* _body;
  uint32_t _param_count;
  Param _params[];
};

std::ostream& operator<< (std::ostream& os, const Fn const* fn);

} // namespace lum

#endif // _LUM_FN_H_
