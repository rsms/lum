#include "test.h"
#include <lum/str.h>

using namespace lum;

int main(int argc, const char** argv) {
  Str* s = Str::create("lolcat");
  assert(std::strcmp("lolcat", s->c_str()) == 0);

  return 0;
}
