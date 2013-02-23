#ifndef _LUM_SYM_H_
#define _LUM_SYM_H_

#include <lum/common.h>
#include <lum/hash.h>
#include <lum/str.h>

namespace lum {

struct Sym;

// Exported global strings and symbols, like "+", "def" and "core"
#define LUM_SYM_APPLY(Name, ...) \
  extern const Str const* kStr_##Name; \
  extern const Sym const* kSym_##Name;
#include <lum/sym-defs.h>
#undef LUM_SYM_APPLY

// Exported "core/Name" symbols
#define LUM_SYM_APPLY_ONLY_CORE 1
#define LUM_SYM_APPLY(Name, ...) \
  extern const Str const* kCoreStr_##Name; \
  extern const Sym const* kCoreSym_##Name;
#include <lum/sym-defs.h>
#undef LUM_SYM_APPLY
#undef LUM_SYM_APPLY_ONLY_CORE

const Str* intern_str(const char* cstr);
const Sym* intern_sym(const Str* ns, const Str* name);
const Sym* intern_sym(const Str* name);
const Sym* intern_sym(const char* cstr);

struct Sym {
  uint32_t hash = 0;
  const Str* ns = 0;
  const Str* name = 0;
  const Str* _str = 0;

  static Sym* create(const Str* ns, const Str* name);
  static Sym* create(const char* name) { return create(0, intern_str(name)); }
  static void free(Sym* sym);

  const char* c_str();
  constexpr const char* c_str() const;
  constexpr bool equals(const Sym* other) const;
  
  struct Comparator {
    inline bool operator()(const Sym* const a, const Sym* const b) const {
      return a->equals(b); }};
  struct Hasher {
    inline size_t operator()(const Sym* const a) const { return a->hash; }};
};

constexpr inline bool Sym::equals(const Sym* other) const {
  return (other != 0) &&
         (hash == other->hash) &&
         name->equals(other->name) &&
         (ns == other->ns || ns->equals(other->ns));
}

constexpr inline const char* Sym::c_str() const {
  return _str == 0 ? (ns == 0 ? name->c_str() : "?/?") : _str->c_str();
}

inline const Sym* intern_sym(const char* cstr) {
  return intern_sym(intern_str(cstr));
}

// inline std::ostream& operator<< (std::ostream& os, Sym* p) {
//   return os << p->c_str();
// }

inline std::ostream& operator<< (std::ostream& os, const Sym const* p) {
  return (p->ns ? (os << p->ns->c_str() << '/' << p->name->c_str())
                : os << p->name->c_str());
}

template <size_t N>
struct ConstSym {
  uint32_t hash;
  void* _ns;
  ConstStr<N> const* name;
  ConstStr<N> const* _str;
};

template<size_t N>
constexpr inline ConstSym<N> ConstSymCreate(ConstStr<N> const* name) {
  return ConstSym<N> {hash::combine(name->hash, 0), 0, name, name};
}

template <size_t NamespaceSize, size_t NameSize>
struct ConstSym2 {
  uint32_t hash;
  ConstStr<NamespaceSize> const* ns;
  ConstStr<NameSize> const* name;
  // Note: (NamespaceSize + NameSize) will equal the size needed because
  // ConstStr size is the size of the bytes contained, including the
  // terminating NUL byte. Therefore e.g. a namespace "foo" has the size 4
  // and a name "hello" has the size 6. The ns_and_name should represent
  // "foo/hello" and have a size of 9. Think of it as replacing the NUL byte
  // of the name with a '/' byte, for the separator.
  ConstStr<NamespaceSize + NameSize> const* ns_and_name;
};

template<size_t NamespaceSize, size_t NameSize>
constexpr inline ConstSym2<NamespaceSize,NameSize>
ConstSym2Create(
    ConstStr<NamespaceSize> const* ns,
    ConstStr<NameSize> const* name,
    ConstStr<NamespaceSize + NameSize> const* ns_and_name)
{
  return ConstSym2<NamespaceSize,NameSize> {
    hash::combine(name->hash, ns->hash), ns, name, ns_and_name};
}

} // namespace lum
#endif // _LUM_SYM_H_
