#include <lum/print.h>
#include <lum/cell.h>
#include <lum/fn.h>
#include <lum/sym.h>
#include <lum/namespace.h>
#include <iomanip>

namespace lum {

inline void _print_rest(std::ostream& s, const Cell* c, bool rest) {
  if (rest && c->rest != 0) { s << ' '; _print(s, c->rest, rest); }
}

std::ostream& _print(std::ostream& s, const Cell* c, bool rest) {
  if (c != 0) {
    switch (c->type) {
      case Type::BOOL: { s << std::dec << (c->value.i ? "true" : "false");
        _print_rest(s, c, rest); break; }
      case Type::INT: { s << std::dec << c->value.i;
        _print_rest(s, c, rest); break; }
      case Type::FLOAT: { s << std::dec << c->value.f;
        _print_rest(s, c, rest); break; }
      case Type::BIF: { s << "#<bif " << c->value.p << ">";
        _print_rest(s, c, rest); break; }
      case Type::FN: { s << ((Fn*)c->value.p);
        _print_rest(s, c, rest); break; }
      case Type::KEYWORD: { s << ':' << ((Sym*)c->value.p)->c_str();
        _print_rest(s, c, rest); break; }
      case Type::SYM: { s << (Sym*)c->value.p;
        _print_rest(s, c, rest); break; }
      case Type::VAR: { s << (Var*)c->value.p;
        _print_rest(s, c, rest); break; }
      case Type::LOCAL: { s << "#<local " << (uint64_t)c->value.i << ">";
        _print_rest(s, c, rest); break; }
      case Type::QUOTE: { s << '\''; _print(s, (const Cell*)c->value.p, true);
        _print_rest(s, c, rest); break; }
      case Type::CONS: {
        s << '('; _print(s, (const Cell*)c->value.p, true) << ')';
        _print_rest(s, c, rest);
        break;
      }
      case Type::NS: {
        s << "#<ns " << ((Namespace*)c->value.p)->name()->c_str() << '>';
        _print_rest(s, c, rest);
        break;
      }
      case Type::UNKNOWN: { s << "#<unknown>"; break; }
    }
  }
  return s;
}

} // namespace lum
