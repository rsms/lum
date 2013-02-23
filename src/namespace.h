#ifndef _LUM_NAMESPACE_H_
#define _LUM_NAMESPACE_H_

#include <lum/common.h>
#include <lum/var.h>
#include <lum/sym.h>
#include <unordered_map>

namespace lum {

struct Cell;

struct SymVarMap {
  typedef std::unordered_map<const Str*,Var,Str::Hasher,Str::Comparator> SMap;
  SMap _map;
  Spinlock _lock = LUM_SPINLOCK_INIT;

  SymVarMap() {}
  SymVarMap(SMap&& m) : _map(m) {}

  bool get_or_put(Var*& var, const Str* ns, const Str* name, Cell* value) {
    spinlock_lock(_lock);
    auto P = _map.emplace(name, (Sym*)0, value);
    var = &P.first->second;
    if (P.second) {
      // We just caused this var to be created. Now we need to assign a
      // qualified symbol name to the var.
      var->_symbol = intern_sym(ns, name);
    }
    spinlock_unlock(_lock);
    return P.second;
  }

  Var* get(const Str* name) {
    spinlock_lock(_lock);
    SMap::iterator I = _map.find(name);
    Var* v = (I == _map.end()) ? 0 : &I->second;
    spinlock_unlock(_lock);
    return v;
  }
};

// Namespaces are mappings from (unqualified) symbols to Vars
struct Namespace {
  const Str* _name;
  SymVarMap mappings; // symbol -> var

  Namespace(const Str* name);
  Namespace(const Str* name, SymVarMap&& map) : _name(name), mappings(map) {}
  const Str* name() const { return _name; }

  // void import(Namespace* other) {
  //   // HACKY!
  //   SymVarMap::SMap::iterator I = other->mappings._map.begin();
  //   SymVarMap::SMap::iterator E = other->mappings._map.end();
  //   for (; I != E; ++I) {
  //     mappings._map.insert(*I);
  //   }
  // }

  //static Namespace core;
};

} // namespace lum
#endif // _LUM_NAMESPACE_H_
