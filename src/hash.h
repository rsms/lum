#ifndef _LUM_HASH_H_
#define _LUM_HASH_H_

#include <lum/common.h>
#include <cassert>

namespace lum { namespace hash {

constexpr uint32_t fnv1a(const char *const cstr);
constexpr uint32_t fnv1a(const char *const bytes, const size_t len);
constexpr uint32_t combine(uint32_t a, uint32_t b);

const uint32_t FNV1A_PRIME = 0x01000193;
const uint32_t FNV1A_SEED = 0x811c9dc5;

// ---- impl ----
// inspired by https://gist.github.com/filsinger/1255697
constexpr inline uint32_t _fnv1a(
    const char *const str,
    const size_t len,
    const uint32_t v)
{
  return (len == 0) ? v :
    _fnv1a(str + 1, len - 1, ( v ^ uint32_t(str[0]) ) * FNV1A_PRIME);
}
constexpr inline uint32_t fnv1a(const char *const str, const size_t len) {
  return _fnv1a(str, len, FNV1A_SEED);
}

constexpr inline uint32_t _fnv1a(
    const char *const str,
    const uint32_t v)
{
  return (str[0] == '\0') ? v :
    _fnv1a(&str[1], ( v ^ uint32_t(str[0]) ) * FNV1A_PRIME);
}
constexpr inline uint32_t fnv1a(const char *const str) {
  return _fnv1a(str, FNV1A_SEED);
}

constexpr inline uint32_t combine(uint32_t a, uint32_t b) {
  return a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
}

// inline uint32_t LUM_UNUSED fnv1a(const char* s) {
//   assert(s != 0);
//   uint32_t hash = FNV1A_SEED;
//   const unsigned char* ptr = (const unsigned char*)s;
//   while (*ptr) {
//     // hash = (*ptr++ ^ hash) * FNV1A_PRIME;
//     hash ^= (uint32_t)s[i++];
//     hash += (hash<<1) + (hash<<4) + (hash<<7) + (hash<<8) + (hash<<24);
//   }
//   return hash;
// }

// inline uint32_t LUM_UNUSED fnv1a(
//     const char* s,
//     const size_t length,
//     uint32_t hash = FNV1A_SEED)
// {
//   assert(s != 0);
//   size_t i = 0;
//   while (i != length) {
//     hash ^= (uint32_t)s[i++];
//     hash += (hash<<1) + (hash<<4) + (hash<<7) + (hash<<8) + (hash<<24);
//   }
//   return hash;
// }

}} // namespace lum::hash

#endif // _LUM_HASH_H_
