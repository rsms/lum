#ifndef _LUM_CELL_H_
#define _LUM_CELL_H_

#include <lum/common.h>
#include <lum/print.h> // for operator<<Cell*
#include <lum/sym.h>

namespace lum {

enum class Type : size_t {
  UNKNOWN,
  BOOL,
  INT,
  FLOAT,
  BIF,
  FN,
  SYM,
  KEYWORD,
  VAR,
  LOCAL,
  QUOTE,
  LIST,  // (x ...)
  NS,
};

struct Namespace;
struct Var;
struct Cell;
struct Env;
struct Sym;
struct Fn;
struct Bif;

struct Cell {
  Type type;
  union { void* p; int64_t i; double f; } value;
  Cell* rest;

  constexpr Cell(const Sym const* p, Cell* rest=0)
    : type(Type::SYM), value{.p=(void*)(p)}, rest(rest) {}
  constexpr Cell(bool p, Cell* rest=0)
    : type(Type::BOOL), value{.i=int64_t(p)}, rest(rest) {}

  static Cell* alloc();
  static void free(Cell* c);

  static Cell* create(Type t) {
    Cell* c = alloc(); c->type = t; return c;
  }
  static Cell* createBool(bool v, Cell* rest=0) {
    Cell* c = create(Type::BOOL);
    c->value.i = (int64_t)v;
    c->rest = rest;
    return c;
  }
  static Cell* createInt(int64_t v, Cell* rest=0, Type t=Type::INT) {
    Cell* c = create(t);
    c->value.i = v;
    c->rest = rest;
    return c;
  }
  static Cell* createFloat(double v, Cell* rest=0) {
    Cell* c = create(Type::FLOAT);
    c->value.f = v;
    c->rest = rest;
    return c;
  }
  static Cell* createPtr(Type t, void* v, Cell* rest) {
    Cell* c = create(t);
    c->value.p = v;
    c->rest = rest;
    return c;
  }

  static Cell* createBif(const Bif* v) {
    return createPtr(Type::BIF, (void*)v, 0);
  }
  static const Bif* getBif(const Cell* c) {
    assert(c->type == Type::BIF);
    return ((const Bif*)c->value.p);
  }

  static Cell* createSym(const Sym const* v, Cell* rest=0) {
    return createPtr(Type::SYM, (void*)v, rest);
  }
  static Cell* createKeyword(const Sym const* v, Cell* rest=0) {
    return createPtr(Type::KEYWORD, (void*)v, rest);
  }
  static Cell* createVar(Var* v, Cell* rest=0) {
    return createPtr(Type::VAR, (void*)v, rest);
  }
  static Cell* createLocal(uint64_t stack_offset, Cell* rest=0) {
    return createInt((int64_t)stack_offset, rest, Type::LOCAL);
  }
  static Cell* createQuote(Cell* v, Cell* rest=0) {
    return createPtr(Type::QUOTE, (void*)v, rest);
  }
  static Cell* createList(Cell* v, Cell* rest=0) {
    return createPtr(Type::LIST, (void*)v, rest);
  }
  static Cell* createNS(Namespace* v, Cell* rest=0) {
    return createPtr(Type::NS, (void*)v, rest);
  }
  static Cell* createFn(Fn* v, Cell* rest=0) {
    return createPtr(Type::FN, (void*)v, rest);
  }
  static Cell* createNil(Cell* rest=0) {
    return createSym(kSym_nil, rest);
  }
  static Cell* copy(Cell* other) {
    // creates an exact but shallow copy of `other`
    Cell* c = alloc();
    memcpy((void*)c, (const void*)other, sizeof(Cell));
    return c;
  }
  static Cell* copy(Cell* other, Cell* rest) {
    // creates a shallow copy of `other` with a different `rest`
    Cell* c = alloc();
    memcpy((void*)c, (const void*)other, sizeof(Cell));
    c->rest = rest;
    return c;
  }

  static const char* type_name(Type type);
  const char* type_name() const { return type_name(type); }
};

} // namespace lum
#endif // _LUM_CELL_H_
