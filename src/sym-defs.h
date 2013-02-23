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
LUM_SYM_APPLY(sum,   "+",     LUM_MCAT(Cell::createBIF(BIF_numop<BIF_sumI, BIF_sumF>)) )
LUM_SYM_APPLY(sub,   "-",     LUM_MCAT(Cell::createBIF(BIF_numop<BIF_subI, BIF_subF>)) )
LUM_SYM_APPLY(mul,   "*",     LUM_MCAT(Cell::createBIF(BIF_numop<BIF_mulI, BIF_mulF>)) )
LUM_SYM_APPLY(div,   "/",     LUM_MCAT(Cell::createBIF(BIF_numop<BIF_divI, BIF_divF>)) )
LUM_SYM_APPLY(rem,   "rem",   LUM_MCAT(Cell::createBIF(BIF_numop<BIF_remI, BIF_remF>)) )
LUM_SYM_APPLY(cons,  "cons",  Cell::createBIF(BIF_cons) )
LUM_SYM_APPLY(in_ns, "in-ns", Cell::createBIF(BIF_in_ns) )
LUM_SYM_APPLY(def,   "def",   Cell::createBIF(BIF_def) )
LUM_SYM_APPLY(eq,    "=",     Cell::createBIF(BIF_eq) )
LUM_SYM_APPLY(fn,    "fn",    Cell::createBIF(BIF_fn) )

#endif // LUM_SYM_APPLY
