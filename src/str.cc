#include <lum/str.h>
#include <unordered_map>

namespace lum {

Str* Str::create(const char* cstr, uint32_t length) {
  if (length == 0) {
    size_t len = strlen(cstr);
    assert(len <= UINT32_MAX);
    length = (uint32_t)len;
  }
  Str* obj = (Str*)malloc(sizeof(Str) + length + 1);
  if (obj == 0) {
    fprintf(stderr, "out of memory - unable to allocate string\n");
  } else {
    obj->hash = hash::fnv1a(cstr, length);
    obj->length = length;
    memcpy((void*)&obj->_cstr, (const void*)cstr, length + 1);
  }
  return obj;
}

void Str::free(Str* str) {
  std::free(str);
}

} // namespace lum
