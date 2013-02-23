#ifndef _LUM_VAR_H_
#define _LUM_VAR_H_

#include <lum/common.h>
#include <lum/sym.h>

namespace lum {

struct Cell;

struct Var {
  Var(const Sym* symbol, const Cell* v) : _symbol(symbol), _value(v) {}
  Var(const Var& other) : _symbol(other._symbol), _value(other._value) {}
  Cell* set(const Cell* c) {
    return const_cast<Cell*>(LumAtomicSwap(&_value, c));
  }
  Cell* get() const { return const_cast<Cell*>(_value); }
  const Sym* symbol() const { return _symbol; }
private:
  friend std::ostream& operator<< (std::ostream&,const Var*);
  friend struct SymVarMap;
  const Sym* _symbol;
  const Cell* _value;
};

inline std::ostream& operator<< (std::ostream& os, const Var* p) {
  return ((p->_value == 0)
    ? (os << "<unbound #'" << p->symbol()->c_str() << ">")
    : (os << "#'" << p->symbol()->c_str()));
}

} // namespace lum
#endif // _LUM_VAR_H_
