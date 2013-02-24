#include <lum/cell.h>

namespace lum {

const char* Cell::type_name(Type type) {
  switch (type) {
    case Type::UNKNOWN: { return "unknown"; }
    case Type::BOOL:    { return "bool"; }
    case Type::INT:     { return "int"; }
    case Type::FLOAT:   { return "float"; }
    case Type::BIF:     { return "bif"; }
    case Type::FN:      { return "fn"; }
    case Type::SYM:     { return "sym"; }
    case Type::KEYWORD: { return "keyword"; }
    case Type::VAR:     { return "var"; }
    case Type::LOCAL:   { return "local"; }
    case Type::QUOTE:   { return "quote"; }
    case Type::LIST:    { return "list"; }
    case Type::NS:      { return "ns"; }
  }
}

// #define LUM_DEBUG_CELL_MEMORY 1

Cell* Cell::alloc() {
#if LUM_DEBUG_CELL_MEMORY
  Cell* c = (Cell*)std::malloc(sizeof(Cell));
  std::cout << "Cell::alloc " << (void*)c << " [" << sizeof(Cell) << "B]\n";
  return c;
#else
  return (Cell*)std::malloc(sizeof(Cell));
#endif // LUM_DEBUG_CELL_MEMORY
}

void Cell::free(Cell* c) {
  // TODO some serious work... Yikes
#if LUM_DEBUG_CELL_MEMORY
  std::cout << "Cell::free " << (void*)c << " '";
  print1(std::cout,c) << "'\n";
#endif // LUM_DEBUG_CELL_MEMORY
  std::free(c);
}

} // namespace lum
