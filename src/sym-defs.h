#ifdef LUM_SYM_APPLY

#if !LUM_SYM_APPLY_ONLY_CORE
// Other symbols (which do not represent cells in the core namespace)
// These will get the following public definitions:
//   const Str* kStr_Name     -> "Value"
//   const Sym* kSym_Name     -> <Value>
//
//            Name   Value
LUM_SYM_APPLY(core,  "core", 0)

#endif // !LUM_SYM_APPLY_ONLY_CORE

// Symbols which represent cells in the core namespace.
// These will get the following public definitions:
//   const Str* kStr_Name     -> "Value"
//   const Sym* kSym_Name     -> <Value>
//   const Sym* kCoreSym_Name -> <core/Value>
// In addition, the core namespace will be populated.
//
//            Name   Value    Cell creation method
LUM_SYM_APPLY(nil,   "nil",   Cell::createSym(kSym_nil) )
LUM_SYM_APPLY(true,  "true",  Cell::createBool(true) )
LUM_SYM_APPLY(false, "false", Cell::createBool(false) )

// Export BIFs like so:
// LUM_SYM_APPLY(sum, "+",    Cell::createBif(kBif_sum))
#define LUM_BIF_APPLY(Name, CStr) \
LUM_SYM_APPLY(Name,   CStr,   Cell::createBif(kBif_##Name))
#include <lum/bif-defs.h>
#undef LUM_BIF_APPLY

#endif // LUM_SYM_APPLY
