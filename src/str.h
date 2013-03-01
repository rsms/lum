#ifndef _LUM_STR_H_
#define _LUM_STR_H_

#include <lum/common.h>
#include <lum/hash.h>

namespace lum {

struct Str {
  static Str* create(const char* str, uint32_t length_hint=0);
  static void free(Str* str);

  const char* c_str() const { return _cstr; }
  const char* bytes() const { return _cstr; }
  constexpr bool equals(const Str* const other) const;
  bool ends_with(const char* bytes, size_t len) const;
  bool ends_with(const char* cstr) const;

  struct Comparator {
    inline bool operator()(const Str* const a, const Str* const b) const {
      return a->equals(b); }};

  struct Hasher {
    inline size_t operator()(const Str* const a) const { return a->hash; }};

  uint32_t hash;
  uint32_t length;
private:
  const char _cstr[];
};


constexpr inline bool Str::equals(const Str* const other) const {
  return (other != 0) &&
         (hash == other->hash) &&
         (length == other->length) &&
         (std::memcmp(bytes(), other->bytes(), length) == 0);
}

inline bool Str::ends_with(const char* bytes, size_t len) const {
  return length >= len &&
         memcmp((const void*)(((const char*)_cstr)+(length-len)),
                (const void*)bytes,
                len) == 0;
}

inline bool Str::ends_with(const char* cstr) const {
  return ends_with(cstr, strlen(cstr));
}

inline static std::ostream& operator<< (std::ostream& os, const Str* s) {
  return os << s->c_str();
}

// A constant-expression initializable string which is bridge-free
// compatible with a Str.
template <size_t N> struct ConstStr;

// Create a ConstStr from a constant c-string.
template<size_t N> constexpr ConstStr<N> ConstStrCreate(const char(&s)[N]);

template<size_t ... Indices>
struct indices_holder {};

template<size_t index_to_add, typename Indices=indices_holder<> >
struct make_indices_impl;

template<size_t index_to_add, size_t...existing_indices>
struct make_indices_impl<index_to_add,indices_holder<existing_indices...> > {
  typedef typename make_indices_impl<
    index_to_add-1,
    indices_holder<index_to_add-1,existing_indices...> >::type type;
};

template<size_t... existing_indices>
struct make_indices_impl<0,indices_holder<existing_indices...> > {
  typedef indices_holder<existing_indices...>  type;
};

template<size_t max_index>
constexpr typename make_indices_impl<max_index>::type make_indices() {
  return typename make_indices_impl<max_index>::type();
}

template <size_t N>
struct ConstStr {
  uint32_t hash;
  uint32_t length;
  char _cstr[N];

  constexpr const char* c_str() const { return _cstr; }

  template<size_t... Indexes>
  static constexpr ConstStr
  create(const char(&s)[N], indices_holder<Indexes...>) {
    return ConstStr{
      hash::fnv1a(s),
      uint32_t(N-1),
      {(Indexes < N-1 ? s[Indexes] : char())...}
    };
  }
};

template<size_t N>
constexpr inline ConstStr<N> ConstStrCreate(const char(&s)[N]) {
  return ConstStr<N>::create(s, make_indices<N>());
}

template<size_t N>
inline static std::ostream& operator<< (
    std::ostream& os, const ConstStr<N> const* s)
{
  return os << s->_cstr;
}

} // namespace lum
#endif // _LUM_STR_H_
