#ifndef _LUM_PRINT_H_
#define _LUM_PRINT_H_

#include <lum/common.h>

namespace lum {

struct Cell;

std::ostream& printchain(std::ostream& s, const Cell* c);
std::ostream& print1(std::ostream& s, const Cell* c);

// ---- impl ----
std::ostream& _print(std::ostream& s, const Cell* c, bool rest);
inline std::ostream& printchain(std::ostream& s, const Cell* c) {
  return _print(s, c, true); }
inline std::ostream& print1(std::ostream& s, const Cell* c) {
  return _print(s, c, false); }

inline std::ostream& operator<< (std::ostream& os, const Cell* c) {
  return print1(os, c); }

} // namespace lum
#endif // _LUM_PRINT_H_
