#include <lum/str.h>
#include "test.h"

using namespace lum;

int main(int argc, const char** argv) {
  // Dynamic string
  Str* s = Str::create("lolcat");
  assert_eq(std::strcmp("lolcat", s->c_str()), 0);
  assert_eq(s->length, 6);
  assert_true(s->ends_with("t"));
  assert_true(s->ends_with("cat"));
  assert_false(s->ends_with("ocat"));

  // Constant string
  constexpr ConstStr<7> cs = ConstStrCreate("lolcat");
  const Str* s2 = (const Str*)&cs;
  assert_eq(std::strcmp("lolcat", s2->c_str()), 0);
  assert_eq(s2->length, 6);
  assert_true(s2->ends_with("t"));
  assert_true(s2->ends_with("cat"));
  assert_false(s2->ends_with("ocat"));

  // Comparing dynamic and constant string for equality
  assert_eq(s->hash, s2->hash);
  assert_true(s->equals(s2));
  assert_true(s2->equals(s));

  return 0;
}
