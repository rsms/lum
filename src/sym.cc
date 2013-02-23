#include <lum/sym.h>
#include <unordered_map>

namespace lum {

#define LUM_SYM_APPLY(Name, CStr, ...) \
  static constexpr auto kConstStr_##Name = ConstStrCreate(CStr); \
  static constexpr auto kConstSym_##Name = ConstSymCreate(&kConstStr_##Name); \
  const Str const* kStr_##Name; \
  const Sym const* kSym_##Name;
#include "sym-defs.h"
#undef LUM_SYM_APPLY

#define LUM_SYM_APPLY_ONLY_CORE 1
#define LUM_SYM_APPLY(Name, CStr, ...) \
  static constexpr auto kCoreConstStr_##Name = ConstStrCreate("core/" CStr); \
  static constexpr auto kCoreConstSym_##Name = ConstSym2Create( \
    &kConstStr_core, &kConstStr_##Name, &kCoreConstStr_##Name); \
  const Str const* kCoreStr_##Name; \
  const Sym const* kCoreSym_##Name;
#include "sym-defs.h"
#undef LUM_SYM_APPLY
#undef LUM_SYM_APPLY_ONLY_CORE

// ---------------------------------------------------------------------------

Sym* Sym::create(const Str* ns, const Str* name) {
  Sym* sym = (Sym*)malloc(sizeof(Sym));
  sym->ns = ns;
  sym->name = name;
  sym->_str = 0;
  sym->hash = hash::combine(name->hash, ns == 0 ? 0 : ns->hash);
  return sym;
}

void Sym::free(Sym* sym) {
  if (sym->_str != 0 && sym->_str != sym->name) {
    Str::free(const_cast<Str*>(sym->_str));
    sym->_str = 0;
  }
  std::free(sym);
}

static const Str* intern_str_nscat(const Str* ns, const Str* name);

const char* Sym::c_str() {
  if (_str == 0) {
    const Str* s = intern_str_nscat(ns, name);
    if (!LumAtomicBoolCmpSwap(&_str, 0, s)) {
      // Another thread finished the race before us and already assigned
      // a Str to _str.
    }
  }
  return _str->c_str();
}

// ---------------------------------------------------------------------------
// Global string and symbol interning
struct CStrCmp {
  inline bool operator()(const char* const a, const char* const b) const {
    return std::strcmp(a, b) == 0; }};
struct CStrHash {
  constexpr inline size_t operator()(const char* const a) const {
    return hash::fnv1a(a); }};

typedef
  std::unordered_map<const char*, const Str*, CStrHash, CStrCmp>
  StrMap;

static StrMap   str_intern_map;
static Spinlock str_intern_map_lock = LUM_SPINLOCK_INIT;

typedef
  std::unordered_map<const Str const*, const Sym*, Str::Hasher, Str::Comparator>
  SymMap;

static SymMap   sym_intern_map;
static Spinlock sym_intern_map_lock = LUM_SPINLOCK_INIT;

const Str* intern_str(const char* cstr) {
  const Str* obj = 0;
  spinlock_lock(str_intern_map_lock);
  // Create a new entry in the intern map, but only if the string doesn't
  // already exist.
  auto P = str_intern_map.emplace(cstr, obj);
  if (P.second) {
    P.first->second = obj = Str::create(cstr);
  } else {
    obj = P.first->second;
  }
  spinlock_unlock(str_intern_map_lock);
  return obj;
}

// (0, "bar") -> "bar"
// ("foo", "bar") -> "foo/bar"
const Str* intern_str_nscat(const Str* ns, const Str* name) {
  if (ns == 0) { return name; }

  size_t len = ns->length + 1 + name->length;
  Str* obj = (Str*)malloc(sizeof(Str) + len + 1);
  char* buf = const_cast<char*>(obj->bytes());
  memcpy((void*)buf, (const void*)ns->bytes(), ns->length);
  buf[ns->length] = '/';
  memcpy((void*)(buf + ns->length + 1), (const void*)name->bytes(),
    name->length);
  buf[len] = '\0';
  
  obj->hash = hash::fnv1a(buf);
  obj->length = len;

  spinlock_lock(str_intern_map_lock);
  const Str* actual = str_intern_map.emplace(buf, obj).first->second;
  spinlock_unlock(str_intern_map_lock);

  if (actual != obj) {
    // already existed
    std::free(obj);
  }
  return actual;
}

const Sym* intern_sym(const Str* name) {
  const Sym* obj = 0;
  spinlock_lock(sym_intern_map_lock);
  // Create a new entry in the intern map, but only if the symbol doesn't
  // already exist.
  auto P = sym_intern_map.emplace(name, obj);
  if (P.second) {
    P.first->second = obj = Sym::create(0, name);
  } else {
    obj = P.first->second;
  }
  spinlock_unlock(sym_intern_map_lock);
  return obj;
}

const Sym* intern_sym(const Str* ns, const Str* name) {
  if (ns == 0) { return intern_sym(name); }
  const Str* nsname = intern_str_nscat(ns, name);
  const Sym* obj = 0;
  spinlock_lock(sym_intern_map_lock);
  // Create a new entry in the intern map, but only if the symbol doesn't
  // already exist.
  auto P = sym_intern_map.emplace(nsname, obj);
  if (P.second) {
    Sym* sym = Sym::create(ns, name);
    sym->_str = nsname;
    P.first->second = obj = sym;
  } else {
    obj = P.first->second;
  }
  spinlock_unlock(sym_intern_map_lock);
  return obj;
}

// ---------------------------------------------------------------------------
// Initialize global symbols

static void LUM_UNUSED _dump_sym_intern_map() {
  std::cout << "sym_intern_map => {\n";
  for (auto entry : sym_intern_map) {
    std::cout << "  '" << entry.second << "'\n";
  }
  std::cout << "}\n";
}

static volatile const struct _SymInitializer {
  _SymInitializer() {
    // Initialize exported pointers to internal constants and fill intern maps
    #define LUM_SYM_APPLY(Name, ...) \
      kStr_##Name = (const Str const*)&kConstStr_##Name; \
      kSym_##Name = (const Sym const*)&kConstSym_##Name; \
      str_intern_map.emplace((kConstStr_##Name).c_str(), kStr_##Name); \
      sym_intern_map.emplace(kStr_##Name, kSym_##Name);
    #include <lum/sym-defs.h>
    #undef LUM_SYM_APPLY

    #define LUM_SYM_APPLY_ONLY_CORE 1
    #define LUM_SYM_APPLY(Name, ...) \
      kCoreStr_##Name = (const Str const*)&kCoreConstStr_##Name; \
      kCoreSym_##Name = (const Sym const*)&kCoreConstSym_##Name; \
      str_intern_map.emplace( \
        (kCoreConstStr_##Name).c_str(), kCoreStr_##Name); \
      sym_intern_map.emplace(kCoreStr_##Name, kCoreSym_##Name);
    #include <lum/sym-defs.h>
    #undef LUM_SYM_APPLY
    #undef LUM_SYM_APPLY_ONLY_CORE

    //_dump_sym_intern_map();
  }
} _sym_initializer;

} // namespace lum
