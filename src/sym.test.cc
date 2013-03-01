#include <lum/sym.h>
#include "test.h"

using namespace lum;

int main(int argc, const char** argv) {
  // String interning
  { const Str* s = intern_str("lolcat");
    assert_eq_cstr("lolcat", s->c_str());
    assert_eq(s->length, 6);
    for (int i = 10; --i; ) {
      // Same string should be returned every time
      assert_eq(s, intern_str("lolcat"));
    }
  }
  
  // Dynamic unqualified symbols
  Sym* s = Sym::create(0, intern_str("lol"));
  assert_null(s->ns);
  assert_eq_cstr("lol", s->name->c_str());
  Sym* s2 = Sym::create(0, intern_str("lol"));
  assert_eq(s->hash, s2->hash);
  assert_true(s->equals(s2));
  assert_true(s2->equals(s));
  Sym::free(s2);

  // Constant unqualified symbols
  static constexpr ConstStr<4> conststr_cat = ConstStrCreate("cat");
  static constexpr ConstSym<4> constsym = ConstSymCreate(&conststr_cat);
  const Sym* cs = (const Sym*)&constsym;
  assert_null(cs->ns);
  assert_eq_cstr("cat", cs->name->c_str());
  s2 = Sym::create(0, intern_str("cat"));
  assert_eq(cs->hash, s2->hash);
  assert_true(cs->equals(s2));
  assert_true(s2->equals(cs));
  Sym::free(s2);

  // Constant qualified symbols
  static constexpr ConstStr<8> conststr_foobar = ConstStrCreate("foo.bar");
  static constexpr ConstStr<12> conststr_foobar_cat =
    ConstStrCreate("foo.bar/cat");
  static constexpr ConstSym2<8,4> constsym2 =
    ConstSym2Create(&conststr_foobar, &conststr_cat, &conststr_foobar_cat);
  cs = (const Sym*)&constsym2;
  assert_eq_cstr("foo.bar",     cs->ns->c_str());
  assert_eq_cstr("cat",         cs->name->c_str());
  assert_eq_cstr("foo.bar/cat", cs->c_str());

  // Dynamic qualified symbols
  const Str* istr_foobar = intern_str("foo.bar");
  const Str* istr_dog = intern_str("dog");
  Sym* qs = Sym::create(istr_foobar, istr_dog);
  assert_eq_cstr("foo.bar",     qs->ns->c_str());
  assert_eq_cstr("dog",         qs->name->c_str());
  assert_eq_cstr("foo.bar/dog", qs->c_str());
  assert_not_eq(qs->ns, cs->ns); // b/c qs->ns is dynamically allocated
  assert_not_eq(qs->name, cs->name);
  assert_true(qs->ns->equals(cs->ns));
  assert_false(qs->name->equals(cs->name));

  // Symbol interning with Str object
  cs = intern_sym(istr_foobar, istr_dog);
  assert_eq_cstr("foo.bar",     qs->ns->c_str());
  assert_eq_cstr("dog",         qs->name->c_str());
  assert_eq_cstr("foo.bar/dog", qs->c_str());
  assert_true(qs->equals(cs));
  assert_true(cs->equals(qs));
  for (int i = 10; --i; ) {
    // Same symbol should be returned every time
    assert_eq(cs, intern_sym(istr_foobar, istr_dog));
  }

  // Symbol interning with c-strings
  assert_eq(intern_sym("dog"), intern_sym(istr_dog));

  return 0;
}
