#include <lum/bif.h>
#include <lum/cell.h>
#include <lum/fn.h>
#include <lum/env.h>
#include <lum/eval.h>
#include <lum/print.h>

namespace lum {

template <NumOpI IOP, NumOpF FOP>
Cell* BIF_numop(Env* env, Cell* args) {
  typedef union { void* p; int64_t i; double f; } NumVal;
  NumVal sum;
  sum.p = 0;
  bool is_int = true;
  bool is_inited = false;
  Cell* arg = args;

  while (arg != 0) {
    args = arg->rest; // to avoid linking into the result of eval_cons
    bool did_expand_arg = false;
    consider_arg:
    switch (arg->type) {
      case Type::INT: {
        if (is_int) {
          if (is_inited) {
            IOP(sum.i, arg->value.i);
          } else {
            sum.i = arg->value.i;
          }
        } else {
          int64_t v = arg->value.i;
          if (v > 9007199254740992 || v < -9007199254740992) {
            fprintf(stderr, "BIF/add: "
              "error precision loss in int-to-float conversion\n");
          }
          FOP(sum.f, (double)v);
        }
        break;
      }
      case Type::FLOAT: {
        if (is_int) {
          is_int = false;
          if (sum.i != 0) {
            // move sum.i to sum.f
            int64_t v = sum.i;
            if (v > 9007199254740992 || v < -9007199254740992) {
              fprintf(stderr, "BIF/add: "
                "error precision loss in int-to-float conversion\n");
            }
            sum.f = (double)v;
          }
        }
        if (is_inited) {
          FOP(sum.f, arg->value.f);
        } else {
          sum.f = arg->value.f;
        }
        break;
      }
      default: {
        // Note: This is a form of optimization. We could as well just do
        // `eval` on every argument before this switch, but by checking for
        // atom types like INT before `eval`ing, we save ourselves one switch
        // statement.
        if (did_expand_arg) {
          goto bad_arg;
        } else {
          did_expand_arg = true;
          arg = eval(env, arg);
          goto consider_arg;
        }
      }
    } // switch arg->type
    arg = args;
    is_inited = true;
  } // while (arg != 0)

  return is_int ? Cell::createInt(sum.i) : Cell::createFloat(sum.f);

  bad_arg:
  std::cerr << "bad argument to built-in function: ";
  print1(std::cerr, arg) << '\n';
  return 0;
}


template Cell* BIF_numop<BIF_sumI, BIF_sumF>(Env* env, Cell* args);
template Cell* BIF_numop<BIF_subI, BIF_subF>(Env* env, Cell* args);
template Cell* BIF_numop<BIF_mulI, BIF_mulF>(Env* env, Cell* args);
template Cell* BIF_numop<BIF_divI, BIF_divF>(Env* env, Cell* args);
template Cell* BIF_numop<BIF_remI, BIF_remF>(Env* env, Cell* args);


Cell* BIF_eq(Env* env, Cell* args) {
  if (args == 0) {
    std::cerr << "too few arguments given to built-in function '='\n";
    return 0;
  }
  Cell* a = eval(env, args);
  Cell* b;
  while ((b = a->rest) != 0) {
    b = eval(env, b);
    if (a->type != b->type) {
      goto return_false;
    } else {
      // same type
      switch (b->type) {
        case Type::BOOL:
        case Type::INT: {
          if (a->value.i != b->value.i) { goto return_false; }
          break;
        }
        case Type::FLOAT: {
          if (a->value.f != b->value.f) { goto return_false; }
          break;
        }
        case Type::SYM: case Type::KEYWORD: {
          if (a->value.p != b->value.p ||
              !((Sym*)a->value.p)->equals((Sym*)b->value.p) )
          {
            goto return_false;
          }
          break;
        }
        default: {
          if (a->value.p != b->value.p) { goto return_false; }
          break;
        }
      }
    }
    a = b;
  }
  return Cell::createSym(kSym_true);
  return_false:
  return Cell::createSym(kSym_false);
}


Cell* BIF_cons(Env* env, Cell* args) {
  if (args == 0) {
    std::cerr << "too few arguments given to built-in function 'cons'\n";
    return 0;
  }

  // Now, evaluate any arguments
  Cell* rest = args->rest;
  if (rest != 0) {
    if (rest->rest != 0) {
      std::cerr << "too many arguments given to built-in function 'cons'\n";
      return 0;
    }
    rest = eval(env, rest);
    if (rest->type != Type::CONS) {
      std::cerr <<
        "second argument to built-in function 'cons' is not a list\n";
      return 0;
    } else if (env->results.top() == rest) {
      // steal
      std::cout << "cons: stealing newly created 'rest'\n";
      env->results.pop();
    } else {
      // copy
      std::cout << "cons: copying existing 'rest'\n";
      rest = Cell::copy(rest);
    }
  } else {
    rest = Cell::createCons(0);
  }

  // First, evaluate `first`
  Cell* first = eval(env, args);
  if (env->results.top() == first) {
    std::cout << "cons: stealing newly created 'first'\n";
    // steal, since first was just created by eval, so we know for sure
    // that no one is referencing this `first` cell.
    env->results.pop();
  } else {
    std::cout << "cons: copying existing 'first'\n";
    // copy, as this is a value by reference and other's might reference it
    first = Cell::copy(first);
  }

  // 1. first->rest = first(cons)
  // 2. set-first(cons, first)
  first->rest = (Cell*)rest->value.p;
  rest->value.p = (void*)first;

  return rest;
}


Cell* BIF_in_ns(Env* env, Cell* arg) {
  if (arg == 0 || arg->rest != 0) {
    std::cerr << "built-in function 'in-ns' takes exactly one argument\n";
    return 0;
  }
  if (arg->type != Type::SYM) {
    std::cerr << "first argument to 'in-ns' must be a symbol\n";
    return 0;
  }
  const Str* name = ((Sym*)arg->value.p)->name;
  Namespace* ns = env->ns_set_current(name);
  return Cell::createNS(ns);
}


Cell* BIF_def(Env* env, Cell* arg) {
  if (arg == 0 || arg->rest == 0 || arg->rest->rest != 0) {
    std::cerr << "built-in function 'def' takes exactly one argument\n";
    return 0;
  }
  if (arg->type != Type::SYM) {
    std::cerr << "first argument to 'def' must be a symbol\n";
    return 0;
  }
  Sym* sym = (Sym*)arg->value.p;
  if (sym->ns != 0) {
    std::cerr << "namespace qualified symbol can not be used for "
                 "built-in function 'def'\n";
    return 0;
  }
  Var* v = env->define(sym, arg->rest);
  return Cell::createVar(v);
}


Cell* BIF_fn(Env* env, Cell* args) {
  // (fn (arg0 ...argN) body0 ...bodyN)
  if (args == 0 || args->rest == 0) {
    std::cerr << "built-in function 'fn' requires at least two arguments\n";
    return 0;
  }
  if (args->type != Type::CONS) {
    std::cerr << "first argument to 'fn' must be a list\n";
    return 0;
  }
  Fn* fn = Fn::create(env, args);
  if (fn == 0) {
    // error
    Cell::free(args);
    return 0;
  }
  return Cell::createFn(fn);
}


} // namespace lum
