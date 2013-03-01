#include "lum.h"
#include <iostream>

// Since we do not use exceptions...
namespace boost {
void throw_exception(std::exception const & e) {
  std::cerr << "Fatal: boost exception: " << e.what() << '\n';
  abort();
}
}
