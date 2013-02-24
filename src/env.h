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
  T at(size_t i) const { assert(i < depth); return items[i]; }
  T at_top(size_t offset) const {
    assert(offset < depth);
    return at((depth-1) - offset);
  }
  T top() const { return depth ? items[depth-1] : 0; }
  void push(T v) { assert(depth+1 != Size); items[depth++] = v; }
  T pop() { return items[--depth]; }
  size_t depth = 0;
  T items[Size];
};

template <typename T, size_t N>
inline std::ostream& operator<< (std::ostream& s, const Stack<T,N>& p) {
  #if 0
  s << "[";
  size_t i = 0;
  while (i != p.depth) {
    if (i != 0) { s << ", "; } s << p.at(i++);
  }
  return s << "]";
  #else
  size_t i = p.depth;
  while (i--) {
    s << "\n  [" << i << "] => " << p.at(i);
  }
  return s;
  #endif
}

struct Env {
  struct Results {
    void push(Cell* c) {
      #if LUM_DEBUG_RESULT_STACK
      std::cout << "results" << cell_stack << " ← " << c << '\n';
      #endif
      cell_stack.push(c);
    }
    size_t index() { return cell_stack.depth; }
    Cell* top() { return cell_stack.top(); }
    Cell* pop() { return cell_stack.pop(); }
    void unwind(size_t end_depth) {
      while (cell_stack.depth != end_depth) {
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
  Stack<Cell*,1024> apply_stack;

  // Get the local which is at position `offset_from_top` from the top of the
  // locals stack.
  Cell* get_local(size_t offset_from_top) {
    return locals.at_top(offset_from_top);
  }

  void unwind_locals(size_t end_depth) {
    while (locals.depth != end_depth) {
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
