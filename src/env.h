#ifndef _LUM_ENV_H_
#define _LUM_ENV_H_

#include <lum/common.h>
#include <lum/cell.h>
#include <lum/namespace.h>
#include <unordered_map>

namespace lum {

#define LUM_DEBUG_RESULT_STACK 1

template <typename T, size_t Size>
struct Stack {
  T at(size_t i) const { assert(i < index); return items[i]; }
  T top() const { return index ? items[index-1] : 0; }
  void push(T v) { assert(index+1 != Size); items[index++] = v; }
  T pop() { return items[--index]; }
  size_t index = 0;
  T items[Size];
};

template <size_t N>
inline std::ostream& operator<< (std::ostream& s, const Stack<Cell*,N>& p) {
  s << "[";
  size_t i = 0;
  while (i != p.index) {
    if (i != 0) { s << ", "; }
    print1(s, p.at(i++));
  }
  return s << "]";
}

struct Env {
  struct Results {
    void push(Cell* c) {
      #if LUM_DEBUG_RESULT_STACK
      std::cout << "results" << cell_stack << " ← " << c << '\n';
      #endif
      cell_stack.push(c);
    }
    size_t index() { return cell_stack.index; }
    Cell* top() { return cell_stack.top(); }
    Cell* pop() { return cell_stack.pop(); }
    void unwind(size_t end_index) {
      while (cell_stack.index != end_index) {
        Cell* c = cell_stack.pop();
        #if LUM_DEBUG_RESULT_STACK
        std::cout << "results" << cell_stack << " → " << c << '\n';
        #endif
        Cell::free(c);
      }
    }
    Stack<Cell*,128> cell_stack;
  } results;

  Namespace core_ns;
  Namespace* ns; // current

  Stack<Cell*,1024> locals;
  Stack<Fn*,128> compile_stack;

  void unwind_locals(size_t end_index) {
    while (locals.index != end_index) {
      locals.pop(); }}

  typedef
    std::unordered_map<const Str*,Namespace,Str::Hasher,Str::Comparator>
    NSMap;
  Spinlock _ns_map_lock = LUM_SPINLOCK_INIT;
  NSMap _ns_map;

  Env() : core_ns(kStr_core), ns(0) {
    ns = ns_get(intern_str("user"));
  }

  Namespace* ns_get(const Str* name) {
    if (name == kStr_core) {
      return &core_ns;
    }
    spinlock_lock(_ns_map_lock);
    NSMap::iterator I = _ns_map.find(name);
    if (I == _ns_map.end()) {
      // Newfound
      std::pair<NSMap::iterator,bool> P = _ns_map.emplace(name, name);
      spinlock_unlock(_ns_map_lock);
      Namespace* new_ns = &P.first->second;
      return new_ns;
    } else {
      // Existing
      spinlock_unlock(_ns_map_lock);
      return &I->second;
    }
  }

  Namespace* ns_set_current(const Str* name) {
    return ns = ns_get(name);
  }

  // Sets the `value` of the Var mapped to `name`, creating a mapping if
  // needed.
  Var* define(Sym* sym, Cell* value) {
    assert(sym->ns == 0); // must be an unqualified symbol
    Var* v;
    if (!ns->mappings.get_or_put(v, ns->name(), sym->name, value)) {
      // rebind var
      Cell* pc = v->set(value);
      std::cout << "[ns def] rebinding var " << sym << " from "
                << pc << " to "
                << value << "\n";
    }
    return v;
  }

  // Finds a mapping from `sym` to a Var. Returns NULL if not found.
  Var* resolve_symbol(Sym* sym) {
    assert(sym->ns == 0); // TODO support two-level qualified symbol lookups
    return ns->mappings.get(sym->name);
  }

  // Like resolve_symbol, but if there's no existing mapping from the symbol
  // to a var, a new mapping is created to an unbound var. This happens when a
  // function is compiled and it references an outside symbol.
  Var* map_symbol(Sym* sym) {
    assert(sym->ns == 0); // TODO support two-level qualified symbol lookups
    Var* v;
    ns->mappings.get_or_put(v, ns->name(), sym->name, 0);
    return v;
  }
};

} // namespace lum
#endif // _LUM_ENV_H_
