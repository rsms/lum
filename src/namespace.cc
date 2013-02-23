#include <lum/namespace.h>
#include <lum/cell.h>
#include <lum/bif.h>

namespace lum {

Namespace::Namespace(const Str* name)
    : _name(name)

    // A new namespace is always prepopulated with core symbols
    , mappings({ // SymVarMap::SMap
      // const Sym* unqual_sym => Var{const Sym* qual_sym, const Cell* val}
      #define LUM_SYM_APPLY_ONLY_CORE 1
      #define LUM_SYM_APPLY(Name, _cstr, CreateCell) \
        {kStr_##Name, {kCoreSym_##Name, CreateCell}},
      #include <lum/sym-defs.h>
      #undef LUM_SYM_APPLY
      #undef LUM_SYM_APPLY_ONLY_CORE
    })
{}


// constexpr Cell kCell_BIF_def{BIF_def};
// const Cell kCell_nil{kSym_nil};
// constexpr Cell kCell_true{true};
// constexpr Cell kCell_false{false};
// Namespace Namespace::core = {
//   (const Str*)&kStr_core,
//   { // SymVarMap
//     { // SymVarMap::SMap
//       // const Sym* => Var{const Sym*, const Cell*}
//       {kSym_def, {kSym_def, &kCell_BIF_def}},
//       {kSym_nil, {kSym_nil, &kCell_nil}},
//       {kSym_true, {kSym_true, &kCell_true}},
//       {kSym_false, {kSym_false, &kCell_false}},
//     }
//   }
// };

} // namespace lum
